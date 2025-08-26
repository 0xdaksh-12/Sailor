#include <iomanip>
#include <iostream>
#include <string>

#include "sailor.h"

void printProgressBar(uint64_t sent, uint64_t total) {
  if (total == 0) return;
  double ratio = static_cast<double>(sent) / static_cast<double>(total);
  int percent = static_cast<int>(ratio * 100.0);
  int bar_width = 40;

  std::cout << "\r[";
  int pos = static_cast<int>(bar_width * ratio);
  for (int i = 0; i < bar_width; ++i) {
    if (i < pos)
      std::cout << "=";
    else if (i == pos)
      std::cout << ">";
    else
      std::cout << " ";
  }
  std::cout << "] " << std::setw(3) << percent << "% (" << (sent / 1024) << "/"
            << (total / 1024) << " KB)" << std::flush;
}

int main(int argc, char* argv[]) {
  std::string host = "127.0.0.1";
  int port = 9000;
  std::string user = "admin";
  std::string pass = "password123";

  std::string command = (argc > 1) ? argv[1] : "list";

  sailor_set_progress_callback(printProgressBar);

  int32_t conn_res = sailor_connect(host.c_str(), port, user.c_str(), pass.c_str());
  if (conn_res != SAILOR_OK) {
    std::cerr << "Failed to connect and authenticate (Status code: " << conn_res
              << ")" << std::endl;
    return 1;
  }

  if (command == "upload") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " upload <local_file> [remote_dir]"
                << std::endl;
      sailor_disconnect();
      return 1;
    }

    std::string local_file = argv[2];
    std::string remote_dir = (argc > 3) ? argv[3] : "/";

    std::cout << "Starting upload: " << local_file << " -> " << remote_dir
              << std::endl;
    int32_t ok = sailor_upload(local_file.c_str(), remote_dir.c_str());
    std::cout << (ok == SAILOR_OK ? "\nUpload complete." : "\nUpload failed.")
              << std::endl;
  } else if (command == "download") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " download <remote_path> [local_dest]"
                << std::endl;
      sailor_disconnect();
      return 1;
    }

    std::string remote_path = argv[2];
    std::string local_dest = (argc > 3) ? argv[3] : ".";

    std::cout << "Starting download: " << remote_path << " -> " << local_dest
              << std::endl;
    int32_t ok = sailor_download(remote_path.c_str(), local_dest.c_str());
    std::cout << (ok == SAILOR_OK ? "\nDownload complete." : "\nDownload failed.")
              << std::endl;
  } else if (command == "delete") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " delete <remote_path>"
                << std::endl;
      sailor_disconnect();
      return 1;
    }

    std::string remote_path = argv[2];
    std::cout << "Deleting remote path: " << remote_path << std::endl;
    int32_t ok = sailor_delete(remote_path.c_str());
    if (ok == SAILOR_OK) {
      std::cout << "Delete SUCCESS" << std::endl;
    } else {
      std::cerr << "Delete FAILED (Code: " << ok << ")" << std::endl;
    }
  } else if (command == "rename" || command == "move") {
    if (argc < 4) {
      std::cerr << "Usage: " << argv[0] << " " << command
                << " <source_path> <dest_path>" << std::endl;
      sailor_disconnect();
      return 1;
    }

    std::string src = argv[2];
    std::string dst = argv[3];
    std::cout << "Renaming/Moving: " << src << " -> " << dst << std::endl;
    int32_t ok = sailor_rename(src.c_str(), dst.c_str());
    if (ok == SAILOR_OK) {
      std::cout << "Rename SUCCESS" << std::endl;
    } else {
      std::cerr << "Rename FAILED (Code: " << ok << ")" << std::endl;
    }
  } else if (command == "mkdir") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " mkdir <path>" << std::endl;
      sailor_disconnect();
      return 1;
    }

    std::string path = argv[2];
    std::cout << "Creating directory: " << path << std::endl;
    int32_t ok = sailor_mkdir(path.c_str());
    if (ok == SAILOR_OK) {
      std::cout << "MKDIR SUCCESS" << std::endl;
    } else {
      std::cerr << "MKDIR FAILED (Code: " << ok << ")" << std::endl;
    }
  } else if (command == "rmdir") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " rmdir <path>" << std::endl;
      sailor_disconnect();
      return 1;
    }

    std::string path = argv[2];
    std::cout << "Removing directory: " << path << std::endl;
    int32_t ok = sailor_rmdir(path.c_str());
    if (ok == SAILOR_OK) {
      std::cout << "RMDIR SUCCESS" << std::endl;
    } else {
      std::cerr << "RMDIR FAILED (Code: " << ok << ")" << std::endl;
    }
  } else {
    std::string path = (argc > 2) ? argv[2] : "/";
    std::cout << "\nListing directory: " << path << "\n" << std::endl;
    SailorListResult* result = sailor_list(path.c_str());

    if (result) {
      for (uint64_t i = 0; i < result->count; ++i) {
        const auto& item = result->entries[i];
        if (item.is_directory) {
          std::cout << " [D] " << item.name << "/" << std::endl;
        } else {
          std::cout << " [F] " << std::left << std::setw(24) << item.name
                    << " (" << item.size << " bytes)" << std::endl;
        }
      }
      sailor_free_list(result);
    } else {
      std::cerr << "Failed to list directory" << std::endl;
    }
  }

  sailor_disconnect();
  return 0;
}
