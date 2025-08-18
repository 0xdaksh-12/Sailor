#include "tls_client.hpp"

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

TlsClientContext::TlsClientContext(std::string ca_cert_path,
                                   std::string client_cert_path,
                                   std::string client_key_path)
    : ca_cert_path_(std::move(ca_cert_path)),
      client_cert_path_(std::move(client_cert_path)),
      client_key_path_(std::move(client_key_path)) {}

TlsClientContext::~TlsClientContext() {
  if (ctx_) {
    SSL_CTX_free(ctx_);
    ctx_ = nullptr;
  }
}

bool TlsClientContext::initialize() {
  const SSL_METHOD* method = TLS_client_method();
  ctx_ = SSL_CTX_new(method);
  if (!ctx_) {
    ERR_print_errors_fp(stderr);
    return false;
  }

  SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

  // 1. Verify the Server
  std::string resolved_ca = resolveFilePath(ca_cert_path_);
  if (!resolved_ca.empty() && fs::exists(resolved_ca)) {
    SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, nullptr);
    if (SSL_CTX_load_verify_locations(ctx_, resolved_ca.c_str(), nullptr) <=
        0) {
      std::cerr << "Failed to load trusted server certificate: " << resolved_ca
                << std::endl;
      return false;
    }
  } else {
    SSL_CTX_set_verify(ctx_, SSL_VERIFY_NONE, nullptr);
  }

  // 2. Present Client Identity to Server (mTLS)
  std::string client_cert = resolveFilePath(client_cert_path_);
  std::string client_key = resolveFilePath(client_key_path_);

  if (fs::exists(client_cert) && fs::exists(client_key)) {
    if (SSL_CTX_use_certificate_file(ctx_, client_cert.c_str(),
                                     SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx_, client_key.c_str(),
                                    SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      return false;
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
      std::cerr << "Client private key does not match client certificate"
                << std::endl;
      return false;
    }
  }

  return true;
}
