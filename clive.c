#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<termios.h>
#include<unistd.h>
#include<ctype.h>
#include<errno.h>
#include<sys/ioctl.h>

#define max_lines 1000
#define max_line_length 256
#define ctrl_key(k) ((k) & 0x1f) //for ctrl q , ctrl s

typedef enum{normal_mode,command_mode,insert_mode}editor_mode;

typedef struct{
    char text_lines[max_lines][max_line_length];
    int total_lines;
    int cursor_x,cursor_y; //mouse positions
    editor_mode current_mode;
    char filename[256];
    char command_buffer[256];
    int command_length;
    int has_unsaved_changes; //flag for file 0 = saved 1 = modified
}text_editor;

struct termios original_terminal_settings;
text_editor editor;
