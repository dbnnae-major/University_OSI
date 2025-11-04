#include "args.h"
#include "io.h"

int main(int argc, char** argv) {
  Config parsed_config;
  if (parse_args(argc, argv, &parsed_config) != 0) {
    return 1;
  }
  int run_result_code = run_io(&parsed_config);
  if (run_result_code != 0) {
    return run_result_code;
  }
  return 0;
}
