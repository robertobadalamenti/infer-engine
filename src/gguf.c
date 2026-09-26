
#include "gguf.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int gguf_map_file(const char *file_path, gguf_file_t *f) {
  struct stat st;
  f->fd = open(file_path, O_RDONLY);
  if (f->fd == -1) {
    return GGUF_FILE_OPEN_ERROR;
  }
  int fstat_fd_return = fstat(f->fd, &st);

  if (fstat_fd_return == -1) {
    close(f->fd);
    return GGUF_FILE_STATS_ERROR;
  }
  f->size = (size_t)st.st_size;

  f->base = mmap(NULL, f->size, PROT_READ, MAP_PRIVATE, f->fd, 0);
  if (f->base == MAP_FAILED) {
    close(f->fd);
    return GGUF_FILE_MMAP_ERROR;
  }
  return GGUF_SUCCESS;
}

int gguf_parse_header(const gguf_file_t *f, gguf_header_t *h) {
  if (f->size < 24) {
    return GGUF_PARSE_ERROR_TOO_SMALL;
  }
  const uint8_t *data = (const uint8_t *)f->base;
  memcpy(&h->magic, data, sizeof(h->magic));
  if (memcmp(data, "GGUF", 4) != 0) {
    return GGUF_PARSE_ERROR_MAGIC_INVALID;
  }
  memcpy(&h->version, data + 4, sizeof(h->version));
  if (h->version != 3) {
    return GGUF_PARSE_ERROR_UNSUPPORTED_VERSION;
  }
  memcpy(&h->tensor_count, data + 8, sizeof(h->tensor_count));
  memcpy(&h->metadata_kv_count, data + 16, sizeof(h->metadata_kv_count));

  return GGUF_SUCCESS;
}

int gguf_close_and_unmap_file(const gguf_file_t *f) {
  munmap(f->base, f->size);
  close(f->fd);
  return GGUF_SUCCESS;
}

const char *gguf_strerror(int outcome) {
  switch (outcome) {
  case GGUF_FILE_OPEN_ERROR:
    return "Cannot open file";
  case GGUF_FILE_STATS_ERROR:
    return "Cannot read file size";
  case GGUF_FILE_MMAP_ERROR:
    return "Cannot map file into memory";
  case GGUF_PARSE_ERROR_MAGIC_INVALID:
    return "Not a GGUF file (bad magic)";
  case GGUF_PARSE_ERROR_TOO_SMALL:
    return "File too small to be a GGUF file";
  case GGUF_PARSE_ERROR_UNSUPPORTED_VERSION:
    return "Unsupported GGUF version";
  case GGUF_SUCCESS:
    return "Success";
  default:
    return "Unknown error";
  }
}