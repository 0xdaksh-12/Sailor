#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef SAILOR_EXPORTS
#define SAILOR_API __declspec(dllexport)
#else
#define SAILOR_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define SAILOR_API __attribute__((visibility("default")))
#else
#define SAILOR_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SAILOR_OK = 0,
  SAILOR_CONNECTION_FAILED = -1,
  SAILOR_AUTH_FAILED = -2,
  SAILOR_FILE_NOT_FOUND = -3,
  SAILOR_PERMISSION_DENIED = -4,
  SAILOR_INVALID_PATH = -5,
  SAILOR_INTERNAL_ERROR = -6,
  SAILOR_NOT_CONNECTED = -7,
  SAILOR_OPERATION_FAILED = -8
} SailorStatus;

typedef struct {
  const char* name;
  uint8_t is_directory;
  uint64_t size;
  uint64_t modified_time;
} SailorEntry;

typedef struct {
  SailorEntry* entries;
  uint64_t count;
} SailorListResult;

typedef void (*SailorProgressCallback)(uint64_t transferred, uint64_t total);
typedef void (*SailorLogCallback)(const char* message);

// Lifecycle & Configuration
SAILOR_API void sailor_set_progress_callback(SailorProgressCallback callback);
SAILOR_API void sailor_set_log_callback(SailorLogCallback callback);

SAILOR_API int32_t sailor_connect(const char* host, uint16_t port,
                                  const char* username, const char* password);
SAILOR_API int32_t sailor_disconnect(void);
SAILOR_API int32_t sailor_is_connected(void);

// Directory Operations
SAILOR_API SailorListResult* sailor_list(const char* path);
SAILOR_API void sailor_free_list(SailorListResult* result);
SAILOR_API int32_t sailor_mkdir(const char* path);
SAILOR_API int32_t sailor_rmdir(const char* path);

// File Operations
SAILOR_API int32_t sailor_upload(const char* local_path,
                                 const char* remote_dir);
SAILOR_API int32_t sailor_download(const char* remote_path,
                                   const char* local_path);
SAILOR_API int32_t sailor_delete(const char* remote_path);
SAILOR_API int32_t sailor_rename(const char* source, const char* destination);

#ifdef __cplusplus
}
#endif
