#include "../drivers/screen.h"
#include "../libc/memory.h"

void main() {
    clear();
    for(int i = 0; i < 24; ++i){
    	char str[255];
	int_to_ascii(i, str);
	krnl_print_at(str, 0, i, 0x0f);
    }
   	krnl_print_at("some text\n", 60, 24, 0x0f);
	krnl_print("another some text");
}
