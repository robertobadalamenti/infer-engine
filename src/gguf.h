
#ifndef GGUF_H
#define GGUF_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
  int fd;
  size_t size;
  void *base;
} gguf_file_t;

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint64_t tensor_count;
  uint64_t metadata_kv_count;
} gguf_header_t;

enum gguf_return {
  GGUF_SUCCESS = 0,
  GGUF_FILE_OPEN_ERROR = 1,
  GGUF_FILE_STATS_ERROR = 2,
  GGUF_FILE_MMAP_ERROR = 3,
  GGUF_PARSE_ERROR_MAGIC_INVALID = 4,
  GGUF_PARSE_ERROR_TOO_SMALL = 5,
  GGUF_PARSE_ERROR_UNSUPPORTED_VERSION = 6,
};

int gguf_map_file(const char *file_path, gguf_file_t *f);
int gguf_parse_header(const gguf_file_t *f, gguf_header_t *h);
int gguf_close_and_unmap_file(const gguf_file_t *f);
const char *gguf_strerror(int outcome);

#endif