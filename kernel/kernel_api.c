#include "kernel_api.h"

char input_msg[INPUT_BUF_SIZE] = "";

void raw_keyboard_input(char c) { _current_char = c; };
