#define VIDEO_ADDRESS (char*)0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80

#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

//kernel api
void clear();
void krnl_print_at(char *msg, int col, int row);
void krnl_print(char *msg);
