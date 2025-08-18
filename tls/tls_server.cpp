#include "tls_server.hpp"

#include <openssl/err.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

static std::string resolveFilePath(const std::string& path) {
  if (path.empty()) {
    return path;
  }
  if (fs::exists(path)) {
    return path;
  }
  if (fs::exists("../" + path)) {
    return "../" + path;
  }
  try {
    fs::path exe_dir = fs::canonical("/proc/self/exe").parent_path();
    if (fs::exists(exe_dir / path)) {
      return (exe_dir / path).string();
    }
    if (fs::exists(exe_dir.parent_path() / path)) {
      return (exe_dir.parent_path() / path).string();
    }
  } catch (...) {
    // Fallback to original path
  }
  return path;
}

TlsServerContext::TlsServerContext(std::string cert_path, std::string key_path,
                                   std::string client_ca_path)
    : cert_path_(std::move(cert_path)),
      key_path_(std::move(key_path)),
      client_ca_path_(std::move(client_ca_path)) {}

TlsServerContext::~TlsServerContext() {
  if (ctx_) {
    SSL_CTX_free(ctx_);
    ctx_ = nullptr;
  }
}

bool TlsServerContext::initialize() {
  const SSL_METHOD* method = TLS_server_method();
  ctx_ = SSL_CTX_new(method);
  if (!ctx_) {
    ERR_print_errors_fp(stderr);
    return false;
  }

  // Set modern TLS versions (TLS 1.2 and TLS 1.3)
  SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

  std::string resolved_cert = resolveFilePath(cert_path_);
  std::string resolved_key = resolveFilePath(key_path_);

  if (SSL_CTX_use_certificate_file(ctx_, resolved_cert.c_str(),
                                   SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (SSL_CTX_use_PrivateKey_file(ctx_, resolved_key.c_str(),
                                  SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (!SSL_CTX_check_private_key(ctx_)) {
    std::cerr << "Private key does not match public certificate" << std::endl;
    return false;
  }

  // Enable Mutual TLS: Require client to present a valid certificate
  SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                     nullptr);

  // Load trusted CA to verify the client's certificate
  std::string trusted_client_ca = resolveFilePath(client_ca_path_);
  if (SSL_CTX_load_verify_locations(ctx_, trusted_client_ca.c_str(), nullptr) <=
      0) {
    std::cerr << "Failed to configure client CA verification path: "
              << trusted_client_ca << std::endl;
    ERR_print_errors_fp(stderr);
    return false;
  }

  return true;
}
