#pragma once

#ifndef LINE_INFO_CB
#define LINE_INFO_CB
typedef int(*line_info_cb)(
  unsigned long address,
  unsigned char op_index,
  char *filename,
  unsigned int line,
  unsigned int column,
  unsigned int discriminator,
  int end_sequence);
#endif //LINE_INFO_CB

#ifdef __cplusplus

extern "C" {
#endif
    int resolve_addr(int argc, char **argv);
    int enum_file_addresses(const char *file_name, line_info_cb cb);
#ifdef __cplusplus
}
#endif
