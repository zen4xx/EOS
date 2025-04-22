#include "../drivers/screen.h"

void main() {
    clear();
    krnl_print_at("Hello world\n", -1, -1, 0x12);
}
