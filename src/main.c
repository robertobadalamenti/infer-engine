#include "gguf.h"
#include <stdio.h>

int main(void) {
  gguf_file_t file = {.fd = -1};
  gguf_header_t gguf_header = {0};
  int outcome_map_file;
  outcome_map_file =
      gguf_map_file("models/Llama-3.2-1B-Instruct-f16.gguf", &file);
  if (outcome_map_file == GGUF_SUCCESS) {
    printf("File mapped in memory successfully \n");
  } else {
    fprintf(stderr, "%s\n", gguf_strerror(outcome_map_file));
    return 1;
  }
  int outcome_parse_header;
  outcome_parse_header = gguf_parse_header(&file, &gguf_header);
  if (outcome_parse_header == GGUF_SUCCESS) {
    printf("GGUF parsed successfully \n");
  } else {
    fprintf(stderr, "%s\n", gguf_strerror(outcome_parse_header));
    gguf_close_and_unmap_file(&file);
    return 2;
  }
  printf("magic: 0x%08x \n", gguf_header.magic);
  printf("version: %u \n", gguf_header.version);
  printf("tensor_count: %llu \n", gguf_header.tensor_count);
  printf("metadata_kv_count: %llu \n", gguf_header.metadata_kv_count);
  gguf_close_and_unmap_file(&file);
  return 0;
}
