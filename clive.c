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


void show_error_and_exit(const char *msg)
{
    perror(msg);
    exit(1);
}
//restore setting
void restore_terminal_settings()
{
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&original_terminal_settings) == -1)
    {
        show_error_and_exit("tcsetattr");
    }
}

void setup_raw_terminal()
{
    if(tcgetattr(STDIN_FILENO,&original_terminal_settings) == -1)
    {
        show_error_and_exit("tcgetattr");
    }
    atexit(restore_terminal_settings);
    struct termios raw_settings = original_terminal_settings;
    raw_settings.c_iflag &= ~(ICRNL | IXON);
    raw_settings.c_oflag &= ~(OPOST); // post process output handling
    raw_settings.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw_settings.c_cc[VMIN] = 0; // minimum bytes to return
    raw_settings.c_cc[VTIME] = 1; //timeout : 100ms
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw_settings) == -1)
    {
        show_error_and_exit("tcsetattr");
    }
}

//ICRNL : prevent enter key to convert to new line
//IXON : disable ctrl + s
//OPOST : disable output  post processing
//ECHO : prevent character echoing
//ICANON : disable canonical mode
//ISIG : disable signal generation
//IEXTEN : disable extended input processing



//special keys for up,down,left,right
#define arrow_up 1000
#define arrow_down 1001
#define arrow_right 1002
#define arrow_left 1003
int read_user_input()
{
    int bytes_read;
    char input_char;
    while((bytes_read = read(STDIN_FILENO,&input_char,1)) != 1)
    {
        if(bytes_read == -1 && errno != EAGAIN)
        {
            show_error_and_exit("read");
        }
    }
    if(input_char == '\x1b')
    {
        char escape_sequence[3];
        if(read(STDIN_FILENO,&escape_sequence[0],1) != 1)return '\x1b';
        if(read(STDIN_FILENO,&escape_sequence[1],1) != 1)return '\x1b';
        if(escape_sequence[0] == '[')
        {
            switch(escape_sequence[1])
            {
                case 'A' : return arrow_up;
                case 'B' : return arrow_down;
                case 'C' : return arrow_right;
                case 'D' : return arrow_left;
            }
        }
        return '\x1b';
    }
    return input_char;
}