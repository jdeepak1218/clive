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
#define MAX_UNDO 100
#define ctrl_key(k) ((k) & 0x1f) //for ctrl q , ctrl s

static int undo_suppress = 0;

typedef enum{
	OP_NONE,
	OP_INSERT_CHAR,
	OP_DELETE_CHAR,
	OP_SPLIT_LINE,
	OP_JOIN_LINE,
	OP_DELETE_LINE,
	OP_PASTE_LINE,
	OP_OPEN_LINE,
}op_type;

typedef struct
{
	op_type type;
	int x;
	int y;
	char data[256];
}undo_entry;


#ifndef CLIVE_VERSION
#define CLIVE_VERSION "1.0.0"
#endif

typedef enum {normal_mode,command_mode,insert_mode} editor_mode;

typedef struct {
	char text_lines[max_lines][max_line_length];
	int total_lines;
	int cursor_x,cursor_y; //mouse positions
	editor_mode current_mode;
	char filename[256];
	char command_buffer[256];
	int command_length;
	int has_unsaved_changes; //flag for file 0 = saved 1 = modified
	char clipboard[max_line_length];
	int has_clipboard;  //(0 -> empty 1 -> has something)
	char last_search[256];
	char status_message[512];
	int clear_status_on_next_key;
	undo_entry undo_stack[MAX_UNDO];
	int undo_count;
	undo_entry redo_stack[MAX_UNDO];
	int redo_count;
} text_editor;

struct termios original_terminal_settings;
text_editor editor;

void clear_screen_and_exit();
void search_forward_from(int start_y, int start_x);
void search_backward_from(int start_y, int start_x);
void push_undo(op_type type, int y, int x, const char *data);
void clear_redo();
void undo();
void redo();
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
			case 'A' :
				return arrow_up;
			case 'B' :
				return arrow_down;
			case 'C' :
				return arrow_right;
			case 'D' :
				return arrow_left;
			}
		}
		return '\x1b';
	}
	return input_char;
}

void initialize_editor()
{
	editor.total_lines = 1;
	editor.cursor_x = 0;
	editor.cursor_y = 0;
	editor.current_mode = normal_mode;
	editor.command_length = 0;
	editor.has_unsaved_changes = 0;
	strcpy(editor.filename,"");
	strcpy(editor.text_lines[0],"");
	editor.has_clipboard = 0;
	editor.last_search[0] = '\0';
	editor.status_message[0] = '\0';
	editor.clear_status_on_next_key = 0;
	editor.undo_count = 0;
	editor.redo_count = 0;
}

void load_file_into_editor(const char *filename)
{
	strcpy(editor.filename,filename);
	FILE *file_pointer = fopen(filename,"r"); // open in read mode
	if(!file_pointer)
	{
		strcpy(editor.text_lines[0],"");
		editor.total_lines = 1;
		return;
	}
	editor.total_lines = 0;
	while(fgets(editor.text_lines[editor.total_lines],max_line_length,file_pointer) && editor.total_lines < max_lines)
	{
		size_t line_length = strlen(editor.text_lines[editor.total_lines]);
		if(line_length > 0 && editor.text_lines[editor.total_lines][line_length - 1] == '\n')
		{
			editor.text_lines[editor.total_lines][line_length - 1] = '\0';
		}
		editor.total_lines++;
	}
	if(editor.total_lines == 0)
	{
		strcpy(editor.text_lines[0],"");
		editor.total_lines = 1;
	}
	fclose(file_pointer);
}

int save_current_file()
{
	if(strlen(editor.filename) == 0)
	{
		return -1;
	}
	FILE *file_pointer = fopen(editor.filename,"w"); // open file in write mode
	if(!file_pointer)
	{
		return -1;
	}
	for(int line_number = 0; line_number < editor.total_lines ; line_number++)
	{
		fprintf(file_pointer,"%s\n",editor.text_lines[line_number]);
	}
	fclose(file_pointer);
	editor.has_unsaved_changes = 0;
	return 0;
}


void insert_character_at_cursor(char character)
{
	char ch_str[2] = {character, '\0'};
	push_undo(OP_INSERT_CHAR, editor.cursor_y, editor.cursor_x, ch_str);
	clear_redo();
	int current_line_length = strlen(editor.text_lines[editor.cursor_y]);
	if(current_line_length >= max_line_length - 1)return;
	memmove(&editor.text_lines[editor.cursor_y][editor.cursor_x + 1],&editor.text_lines[editor.cursor_y][editor.cursor_x],current_line_length - editor.cursor_x + 1);
	editor.text_lines[editor.cursor_y][editor.cursor_x] = character;
	editor.cursor_x++;
	editor.has_unsaved_changes = 1;
}

void split_line_at_cursor()
{
	if(editor.total_lines >= max_lines)return;
	push_undo(OP_SPLIT_LINE, editor.cursor_y, editor.cursor_x, &editor.text_lines[editor.cursor_y][editor.cursor_x]);
	clear_redo();
	for(int line_number = editor.total_lines ; line_number > editor.cursor_y  + 1; line_number--)
	{
		strcpy(editor.text_lines[line_number],editor.text_lines[line_number - 1]);
	}
	strcpy(editor.text_lines[editor.cursor_y + 1],&editor.text_lines[editor.cursor_y][editor.cursor_x]);
	editor.text_lines[editor.cursor_y][editor.cursor_x] = '\0';
	editor.cursor_y++;
	editor.cursor_x = 0;
	editor.total_lines++;
	editor.has_unsaved_changes = 1;
}

void delete_character_before_cursor()
{
	if(editor.cursor_x == 0 && editor.cursor_y == 0)return;
	if(editor.cursor_x > 0)
	{
		char deleted = editor.text_lines[editor.cursor_y][editor.cursor_x - 1];
		char ch_str[2] = {deleted, '\0'};
		push_undo(OP_DELETE_CHAR, editor.cursor_y, editor.cursor_x - 1, ch_str);
		clear_redo();
		int current_line_length = strlen(editor.text_lines[editor.cursor_y]);
		memmove(&editor.text_lines[editor.cursor_y][editor.cursor_x - 1],&editor.text_lines[editor.cursor_y][editor.cursor_x],current_line_length - editor.cursor_x + 1);
		editor.cursor_x--;
	}
	else
	{
		push_undo(OP_JOIN_LINE, editor.cursor_y - 1, strlen(editor.text_lines[editor.cursor_y - 1]), editor.text_lines[editor.cursor_y]);
		clear_redo();
		editor.cursor_y--;
		editor.cursor_x = strlen(editor.text_lines[editor.cursor_y]);
		strcat(editor.text_lines[editor.cursor_y],editor.text_lines[editor.cursor_y + 1]);
		for(int line_number = editor.cursor_y + 1 ; line_number < editor.total_lines - 1; line_number++)
		{
			strcpy(editor.text_lines[line_number],editor.text_lines[line_number + 1]);
		}
		editor.total_lines--;
	}
	editor.has_unsaved_changes = 1;
}

void handle_cursor_movement(int direction_key)
{
	int current_line_length = strlen(editor.text_lines[editor.cursor_y]);
	switch(direction_key)
	{
	case arrow_left:
		if(editor.cursor_x > 0)editor.cursor_x--;
		break;
	case arrow_down:
		if(editor.cursor_y < editor.total_lines - 1)
		{
			editor.cursor_y++;
			current_line_length = strlen(editor.text_lines[editor.cursor_y]);
			if(editor.cursor_x > current_line_length) editor.cursor_x = current_line_length;
		}
		break;
	case arrow_right:
		if(editor.cursor_x < current_line_length)editor.cursor_x++;
		break;
	case arrow_up:
		if(editor.cursor_y > 0)
		{
			editor.cursor_y--;
			current_line_length = strlen(editor.text_lines[editor.cursor_y]);
			if(editor.cursor_x > current_line_length) editor.cursor_x = current_line_length;
		}
		break;
	}
}

void clear_screen_and_exit()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);  // Clear screen
	write(STDOUT_FILENO, "\x1b[H", 3);
	exit(0);
}

void execute_command()
{
	editor.command_buffer[editor.command_length] = '\0';
	if (editor.command_buffer[0] == '/' || editor.command_buffer[0] == '?')
	{
		char *query = editor.command_buffer + 1;
		int is_backward = (editor.command_buffer[0] == '?');
		if (strlen(query) == 0)
		{
			editor.current_mode = normal_mode;
			editor.command_length = 0;
			return;
		}
		strcpy(editor.last_search, query);
		if (is_backward)
		{
			search_backward_from(editor.cursor_y, editor.cursor_x);
		}
		else
		{
			search_forward_from(editor.cursor_y, editor.cursor_x);
		}
		editor.current_mode = normal_mode;
		editor.command_length = 0;
		return;
	}
	// Jump to line: if command is a number, go to that line
	if (editor.command_length > 0)
	{
		int is_number = 1;
		for (int i = 0; i < editor.command_length; i++)
		{
			if (!isdigit(editor.command_buffer[i]))
			{
				is_number = 0;
				break;
			}
		}
		if (is_number)
		{
			int line_number = atoi(editor.command_buffer);
			if (line_number >= 1 && line_number <= editor.total_lines)
			{
				editor.cursor_y = line_number - 1;
				int line_len = strlen(editor.text_lines[editor.cursor_y]);
				if (editor.cursor_x > line_len)
					editor.cursor_x = line_len;
			}
			editor.current_mode = normal_mode;
			editor.command_length = 0;
			return;
		}
	}
	if(strcmp(editor.command_buffer,"q") == 0)
	{
		if(editor.has_unsaved_changes)
		{
			editor.current_mode = normal_mode;
			editor.command_length = 0;
			return;
		}
		clear_screen_and_exit();
	}
	else if(strcmp(editor.command_buffer,"q!") == 0) // force quit
	{
		clear_screen_and_exit();
	}
	else if(strcmp(editor.command_buffer,"w") == 0)  //save
	{
		if(strlen(editor.filename) == 0)
		{
			snprintf(editor.status_message, sizeof(editor.status_message), "No file name");
		}
		else if (save_current_file() != 0)
		{
			snprintf(editor.status_message, sizeof(editor.status_message),
			         "Can't open \"%s\" for writing", editor.filename);
		}
		else
		{
			snprintf(editor.status_message, sizeof(editor.status_message),
			         "\"%s\" saved", editor.filename);
		}
		editor.clear_status_on_next_key = 1;
	}
	else if(strcmp(editor.command_buffer,"wq") == 0)
	{
		if(strlen(editor.filename) == 0)
		{
			snprintf(editor.status_message, sizeof(editor.status_message), "No file name");
			editor.clear_status_on_next_key = 1;
		}
		else if (save_current_file() != 0)
		{
			snprintf(editor.status_message, sizeof(editor.status_message),
			         "Can't open \"%s\" for writing", editor.filename);
			editor.clear_status_on_next_key = 1;
		}
		else
		{
			clear_screen_and_exit();
		}
	}
	else if(strncmp(editor.command_buffer,"w ",2) == 0)
	{
		strcpy(editor.filename,editor.command_buffer + 2);
		if (save_current_file() != 0)
		{
			snprintf(editor.status_message, sizeof(editor.status_message),
			         "Can't open \"%s\" for writing", editor.filename);
		}
		else
		{
			snprintf(editor.status_message, sizeof(editor.status_message),
			         "\"%s\" saved", editor.filename);
		}
		editor.clear_status_on_next_key = 1;
	}
	editor.current_mode = normal_mode;
	editor.command_length = 0;
}

void get_terminal_dimensions(int *rows,int *columns)
{
	struct winsize window_size;
	if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&window_size) == -1 || window_size.ws_col == 0)
	{
		*rows = 24;
		*columns = 80;
	}
	else
	{
		*rows = window_size.ws_row;
		*columns = window_size.ws_col;
	}
}

void refresh_display()
{
	int terminal_rows,terminal_columns;
	get_terminal_dimensions(&terminal_rows,&terminal_columns);
	write(STDOUT_FILENO,"\x1b[2J",4); // clear screen
	write(STDOUT_FILENO,"\x1b[H",3); // move cursor to top most
	int visible_rows = terminal_rows - 2;
	for(int row = 0 ; row < visible_rows ; row++)
	{
		if(row < editor.total_lines)
		{
			char line_info[32];
			int info_length = snprintf(line_info,sizeof(line_info),"%4d ",row + 1);
			write(STDOUT_FILENO,line_info,info_length);
			write(STDOUT_FILENO,editor.text_lines[row],strlen(editor.text_lines[row]));
		}
		else
		{
			write(STDOUT_FILENO,"   ~",4);
		}
		write(STDOUT_FILENO,"\x1b[K",3);
		if(row < visible_rows - 1)
		{
			write(STDOUT_FILENO,"\r\n",2);
		}
	}
	char position_buffer[32];
	snprintf(position_buffer,sizeof(position_buffer),"\x1b[%d;1H",terminal_rows - 1);
	write(STDOUT_FILENO,position_buffer,strlen(position_buffer));
	write(STDOUT_FILENO,"\x1b[7m",4); // invert color
	char status_line[512];
	int status_length = snprintf(status_line,sizeof(status_line)," %s %s",strlen(editor.filename) > 0 ? editor.filename : "[No name]",editor.has_unsaved_changes ? "[+]" : "");
	char position_info[32];
	snprintf(position_info,sizeof(position_info),"%d,%d ",editor.cursor_y + 1,editor.cursor_x + 1); // added comma and trailing space
	int padding = terminal_columns - status_length - strlen(position_info);
	if(padding < 0)padding = 0;
	write(STDOUT_FILENO,status_line,status_length);
	for(int i = 0 ; i < padding ; i++)
	{
		write(STDOUT_FILENO," ",1);
	}
	write(STDOUT_FILENO,position_info,strlen(position_info));
	write(STDOUT_FILENO,"\x1b[m",3); // normal colors
	snprintf(position_buffer,sizeof(position_buffer),"\x1b[%d;1H",terminal_rows);
	write(STDOUT_FILENO,position_buffer,strlen(position_buffer));
	if (strlen(editor.status_message) > 0)
	{
		write(STDOUT_FILENO, editor.status_message, strlen(editor.status_message));
	}
	else if(editor.current_mode == command_mode)
	{
		write(STDOUT_FILENO,":",1);
		write(STDOUT_FILENO,editor.command_buffer,editor.command_length);
	}
	else if(editor.current_mode == insert_mode)
	{
		write(STDOUT_FILENO,"-- INSERT --",12);
	}
	write(STDOUT_FILENO,"\x1b[K",3);
	char cursor_positions[32];
	snprintf(cursor_positions,sizeof(cursor_positions),"\x1b[%d;%dH",editor.cursor_y + 1,editor.cursor_x + 6);
	write(STDOUT_FILENO,cursor_positions,strlen(cursor_positions));
}

void search_forward_from(int start_y, int start_x)
{
	for (int y = start_y; y < editor.total_lines; y++)
	{
		char *match;
		if (y == start_y)
		{
			match = strstr(editor.text_lines[y] + start_x + 1, editor.last_search);
		}
		else
		{
			match = strstr(editor.text_lines[y], editor.last_search);
		}
		if (match != NULL)
		{
			editor.cursor_y = y;
			editor.cursor_x = match - editor.text_lines[y];
			return;
		}
	}
}
void search_backward_from(int start_y, int start_x)
{
	for (int y = start_y; y >= 0; y--)
	{
		char *match = NULL;
		if (y == start_y)
		{
			char temp[max_line_length];
			strncpy(temp, editor.text_lines[y], start_x);
			temp[start_x] = '\0';
			char *last_match = NULL;
			char *current = temp;
			while ((match = strstr(current, editor.last_search)) != NULL)
			{
				last_match = match;
				current = match + 1;
			}
			match = last_match;
		}
		else
		{
			char *last_match = NULL;
			char *current = editor.text_lines[y];
			while ((match = strstr(current, editor.last_search)) != NULL)
			{
				last_match = match;
				current = match + 1;
			}
			match = last_match;
		}
		if (match != NULL)
		{
			editor.cursor_y = y;
			editor.cursor_x = match - editor.text_lines[y];
			return;
		}
	}
}
void print_version()
{
	printf("Clive version %s\n", CLIVE_VERSION);
}
int is_word_char(char c)
{
	return isalnum(c) || c == '_';
}

void move_word_forward()
{
	char *line = editor.text_lines[editor.cursor_y];
	int len = strlen(line);
	if (len == 0)
	{
		if (editor.cursor_y < editor.total_lines - 1)
		{
			editor.cursor_y++;
			editor.cursor_x = 0;
			line = editor.text_lines[editor.cursor_y];
			len = strlen(line);
			while (editor.cursor_x < len && isspace(line[editor.cursor_x]))
				editor.cursor_x++;
		}
		return;
	}
	if (is_word_char(line[editor.cursor_x]))
	{
		while (editor.cursor_x < len && is_word_char(line[editor.cursor_x]))
			editor.cursor_x++;
	}
	else if (!isspace(line[editor.cursor_x]))
	{
		while (editor.cursor_x < len && !is_word_char(line[editor.cursor_x]) && !isspace(line[editor.cursor_x]))
			editor.cursor_x++;
	}
	while (editor.cursor_x < len && isspace(line[editor.cursor_x]))
		editor.cursor_x++;
	if (editor.cursor_x >= len && editor.cursor_y < editor.total_lines - 1)
	{
		editor.cursor_y++;
		editor.cursor_x = 0;
		line = editor.text_lines[editor.cursor_y];
		len = strlen(line);
		while (editor.cursor_x < len && isspace(line[editor.cursor_x]))
			editor.cursor_x++;
	}
}

void move_word_backward()
{
	char *line = editor.text_lines[editor.cursor_y];
	int len = strlen(line);
	if (editor.cursor_x > 0)
	{
		editor.cursor_x--;
	}
	else if (editor.cursor_y > 0)
	{
		editor.cursor_y--;
		line = editor.text_lines[editor.cursor_y];
		len = strlen(line);
		editor.cursor_x = len;
		if (editor.cursor_x == 0) return;
		editor.cursor_x--;
	}
	else return;
	while (editor.cursor_x > 0 && !is_word_char(line[editor.cursor_x]))
	{
		editor.cursor_x--;
	}
	while (editor.cursor_x > 0 && is_word_char(line[editor.cursor_x - 1]))
	{
		editor.cursor_x--;
	}
}

void move_word_end()
{
	char *line = editor.text_lines[editor.cursor_y];
	int len = strlen(line);
	if (editor.cursor_x < len - 1)
	{
		editor.cursor_x++;
	}
	else if (editor.cursor_y < editor.total_lines - 1)
	{
		editor.cursor_y++;
		editor.cursor_x = 0;
		line = editor.text_lines[editor.cursor_y];
		len = strlen(line);
		if (len == 0) return;
	}
	else
	{
		return;
	}
	while (editor.cursor_x < len && !is_word_char(line[editor.cursor_x]))
	{
		editor.cursor_x++;
	}
	while (editor.cursor_x < len - 1 && is_word_char(line[editor.cursor_x + 1]))
		editor.cursor_x++;
}
void print_usage()
{
	print_version();
	printf("Usage: clive [filename] [options]\n\n");
	printf("Options:\n");
	printf("  -v, --version     Show version information\n");
	printf("  -h, --help        Show this help message\n");
	printf("  -u, --update      Update Clive to the latest version\n\n");
	printf("A minimal Vim-inspired terminal text editor.\n");
}

void push_undo(op_type type, int y, int x, const char *data)
{
	if(undo_suppress) return;
	if(editor.undo_count == MAX_UNDO)
	{
		// Stack full — shift everything left, dropping the oldest (index 0)
		for(int i = 0; i < MAX_UNDO - 1; i++)
		{
			editor.undo_stack[i] = editor.undo_stack[i + 1];
		}
		editor.undo_count--;
	}
	editor.undo_stack[editor.undo_count].type = type;
	editor.undo_stack[editor.undo_count].y = y;
	editor.undo_stack[editor.undo_count].x = x;
	if(data != NULL) strcpy(editor.undo_stack[editor.undo_count].data, data);
	else editor.undo_stack[editor.undo_count].data[0] = '\0';
	editor.undo_count++;
}

void clear_redo()
{
	if(undo_suppress) return;
	editor.redo_count = 0;
}

void push_redo(op_type type, int y, int x, const char *data)
{
	if(editor.redo_count == MAX_UNDO)
	{
		for(int i = 0; i < MAX_UNDO - 1; i++)
		{
			editor.redo_stack[i] = editor.redo_stack[i + 1];
		}
		editor.redo_count--;
	}
	editor.redo_stack[editor.redo_count].type = type;
	editor.redo_stack[editor.redo_count].y = y;
	editor.redo_stack[editor.redo_count].x = x;
	if(data != NULL) strcpy(editor.redo_stack[editor.redo_count].data, data);
	else editor.redo_stack[editor.redo_count].data[0] = '\0';
	editor.redo_count++;
}

void undo()
{
	if(editor.undo_count == 0) return;
	editor.undo_count--;
	undo_entry *op = &editor.undo_stack[editor.undo_count];
	int len;

	switch(op->type)
	{
	case OP_INSERT_CHAR:
	{
		// Reverse: delete the char that was inserted at (op->y, op->x)
		// For redo: save the char we're about to delete
		char c = editor.text_lines[op->y][op->x];
		char ch_str[2] = {c, '\0'};
		push_redo(OP_INSERT_CHAR, op->y, op->x, ch_str);

		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		len = strlen(editor.text_lines[editor.cursor_y]);
		if(editor.cursor_x < len)
		{
			memmove(&editor.text_lines[editor.cursor_y][editor.cursor_x],
				&editor.text_lines[editor.cursor_y][editor.cursor_x + 1],
				len - editor.cursor_x);
			editor.has_unsaved_changes = 1;
		}
		break;
	}
	case OP_DELETE_CHAR:
	{
		// Reverse: re-insert the deleted char at (op->y, op->x)
		char ch_str[2];
		// For redo: save whatever char is currently at this position
		if(op->x < (int)strlen(editor.text_lines[op->y]))
		{
			ch_str[0] = editor.text_lines[op->y][op->x];
			ch_str[1] = '\0';
		}
		else
		{
			ch_str[0] = '\0';
		}
		push_redo(OP_DELETE_CHAR, op->y, op->x, ch_str);

		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		len = strlen(editor.text_lines[editor.cursor_y]);
		if(len < max_line_length - 1)
		{
			memmove(&editor.text_lines[editor.cursor_y][editor.cursor_x + 1],
				&editor.text_lines[editor.cursor_y][editor.cursor_x],
				len - editor.cursor_x + 1);
			editor.text_lines[editor.cursor_y][editor.cursor_x] = op->data[0];
			editor.cursor_x++;
			editor.has_unsaved_changes = 1;
		}
		break;
	}
	case OP_SPLIT_LINE:
	{
		// Reverse: join the two lines back together
		// For redo: save the text that will move to next line
		push_redo(OP_SPLIT_LINE, op->y, op->x,
			&editor.text_lines[op->y][op->x]);

		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		// Restore current line: cut at cursor_x, append saved data
		editor.text_lines[editor.cursor_y][editor.cursor_x] = '\0';
		strcat(editor.text_lines[editor.cursor_y], op->data);
		// Remove the next line (shift everything up)
		for(int i = editor.cursor_y + 1; i < editor.total_lines - 1; i++)
			strcpy(editor.text_lines[i], editor.text_lines[i + 1]);
		editor.total_lines--;
		editor.has_unsaved_changes = 1;
		break;
	}
	case OP_JOIN_LINE:
	{
		// Reverse: split the line back at (op->y, op->x)
		// For redo: save the next line content that will be joined
		push_redo(OP_JOIN_LINE, op->y, op->x,
			editor.text_lines[op->y + 1]);

		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		// Split at cursor_x — move text after cursor to new line
		for(int i = editor.total_lines; i > editor.cursor_y + 1; i--)
			strcpy(editor.text_lines[i], editor.text_lines[i - 1]);
		strcpy(editor.text_lines[editor.cursor_y + 1],
			&editor.text_lines[editor.cursor_y][editor.cursor_x]);
		editor.text_lines[editor.cursor_y][editor.cursor_x] = '\0';
		editor.total_lines++;
		editor.cursor_y++;
		editor.cursor_x = 0;
		editor.has_unsaved_changes = 1;
		break;
	}
	case OP_DELETE_LINE:
	{
		// Reverse: re-insert the deleted line at op->y
		// For redo: save whatever line is currently at op->y
		push_redo(OP_DELETE_LINE, op->y, 0,
			editor.text_lines[op->y]);

		editor.cursor_y = op->y;
		for(int i = editor.total_lines; i > editor.cursor_y; i--)
			strcpy(editor.text_lines[i], editor.text_lines[i - 1]);
		strcpy(editor.text_lines[editor.cursor_y], op->data);
		editor.total_lines++;
		editor.cursor_x = 0;
		editor.has_unsaved_changes = 1;
		break;
	}
	case OP_PASTE_LINE:
	{
		// Reverse: delete the pasted line at op->y
		// For redo: save the line we're about to delete
		push_redo(OP_PASTE_LINE, op->y, 0,
			editor.text_lines[op->y]);

		editor.cursor_y = op->y;
		for(int i = editor.cursor_y; i < editor.total_lines - 1; i++)
			strcpy(editor.text_lines[i], editor.text_lines[i + 1]);
		editor.total_lines--;
		if(editor.cursor_y >= editor.total_lines)
			editor.cursor_y = editor.total_lines - 1;
		editor.cursor_x = 0;
		editor.has_unsaved_changes = 1;
		break;
	}
	case OP_OPEN_LINE:
	{
		// Reverse: delete the empty line that was created
		// For redo: save the line we're about to delete
		push_redo(OP_OPEN_LINE, op->y, 0,
			editor.text_lines[op->y]);

		editor.cursor_y = op->y;
		for(int i = editor.cursor_y; i < editor.total_lines - 1; i++)
			strcpy(editor.text_lines[i], editor.text_lines[i + 1]);
		editor.total_lines--;
		if(editor.cursor_y >= editor.total_lines)
			editor.cursor_y = editor.total_lines - 1;
		editor.cursor_x = 0;
		editor.has_unsaved_changes = 1;
		break;
	}
	default:
		break;
	}
}

void redo()
{
	if(editor.redo_count == 0) return;
	editor.redo_count--;
	undo_entry *op = &editor.redo_stack[editor.redo_count];
	int len;

	switch(op->type)
	{
	case OP_INSERT_CHAR:
	{
		// Re-apply: insert char at (op->y, op->x)
		push_undo(OP_INSERT_CHAR, op->y, op->x, op->data);
		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		undo_suppress = 1;
		insert_character_at_cursor(op->data[0]);
		undo_suppress = 0;
		break;
	}
	case OP_DELETE_CHAR:
	{
		push_undo(OP_DELETE_CHAR, op->y, op->x, op->data);
		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		len = strlen(editor.text_lines[editor.cursor_y]);
		if(editor.cursor_x < len)
		{
			memmove(&editor.text_lines[editor.cursor_y][editor.cursor_x],
				&editor.text_lines[editor.cursor_y][editor.cursor_x + 1],
				len - editor.cursor_x);
			editor.has_unsaved_changes = 1;
		}
		break;
	}
	case OP_SPLIT_LINE:
	{
		push_undo(OP_SPLIT_LINE, op->y, op->x, op->data);
		editor.cursor_y = op->y;
		editor.cursor_x = op->x;
		undo_suppress = 1;
		split_line_at_cursor();
		undo_suppress = 0;
		break;
	}
	case OP_JOIN_LINE:
	{
		push_undo(OP_JOIN_LINE, op->y, op->x, op->data);
		editor.cursor_y = op->y + 1;
		editor.cursor_x = 0;
		undo_suppress = 1;
		delete_character_before_cursor();
		undo_suppress = 0;
		break;
	}
	case OP_DELETE_LINE:
	{
		push_undo(OP_DELETE_LINE, op->y, 0, op->data);
		editor.cursor_y = op->y;
		for(int i = editor.cursor_y; i < editor.total_lines - 1; i++)
			strcpy(editor.text_lines[i], editor.text_lines[i + 1]);
		editor.total_lines--;
		if(editor.cursor_y >= editor.total_lines)
			editor.cursor_y = editor.total_lines - 1;
		editor.has_unsaved_changes = 1;
		break;
	}
	case OP_PASTE_LINE:
	{
		push_undo(OP_PASTE_LINE, op->y, 0, op->data);
		editor.cursor_y = op->y;
		for(int i = editor.total_lines; i > editor.cursor_y; i--)
			strcpy(editor.text_lines[i], editor.text_lines[i - 1]);
		strcpy(editor.text_lines[editor.cursor_y], op->data);
		editor.total_lines++;
		editor.has_unsaved_changes = 1;
		break;
	}
	case OP_OPEN_LINE:
	{
		push_undo(OP_OPEN_LINE, op->y, 0, op->data);
		editor.cursor_y = op->y;
		for(int i = editor.total_lines; i > editor.cursor_y; i--)
			strcpy(editor.text_lines[i], editor.text_lines[i - 1]);
		strcpy(editor.text_lines[editor.cursor_y], "");
		editor.total_lines++;
		editor.has_unsaved_changes = 1;
		break;
	}
	default:
		break;
	}
}

int main(int argument_count, char *argument_values[]) {
	// Handle flags before terminal setup (no raw mode needed)
	if (argument_count >= 2)
	{
		if (strcmp(argument_values[1], "--version") == 0 || strcmp(argument_values[1], "-v") == 0)
		{
			print_version();
			return 0;
		}
		if (strcmp(argument_values[1], "--help") == 0 || strcmp(argument_values[1], "-h") == 0)
		{
			print_usage();
			return 0;
		}
		if (strcmp(argument_values[1], "--update") == 0 || strcmp(argument_values[1], "-u") == 0)
		{
			printf("Updating Clive...\n\n");
			fflush(stdout);
			const char *update_cmd =
			    "curl -fsSL https://raw.githubusercontent.com/jdeepak1218/clive/main/install.sh | bash";
			int result = system(update_cmd);
			if (result == 0)
			{
				printf("\nClive has been updated to the latest version!\n");
			}
			else
			{
				printf("\nUpdate failed. Please try manually:\n");
				printf("  curl -fsSL https://raw.githubusercontent.com/jdeepak1218/clive/main/install.sh | bash\n");
			}
			return result;
		}
	}

	setup_raw_terminal();
	initialize_editor();
	if (argument_count >= 2)
	{
		if (argument_values[1][0] != '-')
		{
			load_file_into_editor(argument_values[1]);
		}
	}
	while (1)
	{
		refresh_display();
		int user_input = read_user_input();

		if (editor.current_mode == normal_mode)
		{
			if (editor.clear_status_on_next_key)
			{
				editor.status_message[0] = '\0';
				editor.clear_status_on_next_key = 0;
			}
			if (user_input == 'i')
			{
				editor.current_mode = insert_mode;
			}
			else if (user_input == '/')
			{
				editor.current_mode = command_mode;
				editor.command_buffer[0] = '/';
				editor.command_length = 1;
			}
			else if (user_input == '?')  // ← ADD THIS
			{
				editor.current_mode = command_mode;
				editor.command_buffer[0] = '?';
				editor.command_length = 1;
			}
			else if (user_input == ':')
			{
				editor.current_mode = command_mode;
				editor.command_length = 0;
			}

			else if (user_input == arrow_left || user_input == 'h')
			{
				handle_cursor_movement(arrow_left);
			}
			else if (user_input == arrow_down || user_input == 'j')
			{
				handle_cursor_movement(arrow_down);
			}
			else if (user_input == arrow_up || user_input == 'k')
			{
				handle_cursor_movement(arrow_up);
			}
			else if (user_input == arrow_right || user_input == 'l')
			{
				handle_cursor_movement(arrow_right);
			}
			else if (user_input == '0')
			{
				editor.cursor_x = 0;
			}
			else if (user_input == '$')
			{
				editor.cursor_x = strlen(editor.text_lines[editor.cursor_y]);
			}
			else if(user_input == 'w')
			{
				move_word_forward();
			}
			else if(user_input == 'b')
			{
				move_word_backward();
			}
			else if(user_input == 'e')
			{
				move_word_end();
			}
			else if (user_input == 'O')
			{
				if (editor.total_lines < max_lines)
				{
					push_undo(OP_OPEN_LINE, editor.cursor_y, 0, "");
					clear_redo();
					for (int line_number = editor.total_lines; line_number > editor.cursor_y; line_number--)
					{
						strcpy(editor.text_lines[line_number], editor.text_lines[line_number - 1]);
					}
					strcpy(editor.text_lines[editor.cursor_y], "");
					editor.cursor_x = 0;
					editor.total_lines++;
					editor.current_mode = insert_mode;
					editor.has_unsaved_changes = 1;
				}
			}
			else if (user_input == 'n')
			{
				if (strlen(editor.last_search) > 0)
				{
					search_forward_from(editor.cursor_y, editor.cursor_x);
				}
			}
			else if (user_input == 'N')
			{
				if (strlen(editor.last_search) > 0)
				{
					search_backward_from(editor.cursor_y, editor.cursor_x);
				}
			}
			else if (user_input == 'u')
			{
				undo();
			}
			else if (user_input == ctrl_key('r'))
			{
				redo();
			}
			else if (user_input == 'd')
			{
				push_undo(OP_DELETE_LINE, editor.cursor_y, 0, editor.text_lines[editor.cursor_y]);
				clear_redo();
				if (editor.total_lines > 1)
				{
					for (int line_number = editor.cursor_y; line_number < editor.total_lines - 1; line_number++)
					{
						strcpy(editor.text_lines[line_number], editor.text_lines[line_number + 1]);
					}
					editor.total_lines--;
					if (editor.cursor_y >= editor.total_lines)
					{
						editor.cursor_y = editor.total_lines - 1;
					}
					int new_line_length = strlen(editor.text_lines[editor.cursor_y]);
					if (editor.cursor_x > new_line_length)
					{
						editor.cursor_x = new_line_length;
					}

					editor.has_unsaved_changes = 1;
				}
				else
				{
					strcpy(editor.text_lines[0], "");
					editor.cursor_x = 0;
					editor.cursor_y = 0;
					editor.has_unsaved_changes = 1;
				}
			}
			else if (user_input == 'y')
			{
				strcpy(editor.clipboard, editor.text_lines[editor.cursor_y]);
				editor.has_clipboard = 1;
			}
			else if (user_input == 'p')
			{
				if (editor.has_clipboard && editor.total_lines < max_lines)
				{
					push_undo(OP_PASTE_LINE, editor.cursor_y + 1, 0, editor.clipboard);
					clear_redo();
					for (int line_number = editor.total_lines; line_number > editor.cursor_y + 1; line_number--)
					{
						strcpy(editor.text_lines[line_number], editor.text_lines[line_number - 1]);
					}
					strcpy(editor.text_lines[editor.cursor_y + 1], editor.clipboard);
					editor.total_lines++;
					editor.cursor_y++;
					editor.cursor_x = 0;
					editor.has_unsaved_changes = 1;
				}
			}
			else if (user_input == 'x')
			{
				if (editor.cursor_x < (int)strlen(editor.text_lines[editor.cursor_y]))
				{
					char deleted = editor.text_lines[editor.cursor_y][editor.cursor_x];
					char ch_str[2] = {deleted, '\0'};
					push_undo(OP_DELETE_CHAR, editor.cursor_y, editor.cursor_x, ch_str);
					clear_redo();
					memmove(&editor.text_lines[editor.cursor_y][editor.cursor_x],
					        &editor.text_lines[editor.cursor_y][editor.cursor_x + 1],
					        strlen(editor.text_lines[editor.cursor_y]) - editor.cursor_x);
					editor.has_unsaved_changes = 1;
				}
			}
			else if (user_input == 'o')
			{
				if (editor.total_lines < max_lines)
				{
					push_undo(OP_OPEN_LINE, editor.cursor_y + 1, 0, "");
					clear_redo();
					for (int line_number = editor.total_lines; line_number > editor.cursor_y + 1; line_number--)
					{
						strcpy(editor.text_lines[line_number], editor.text_lines[line_number - 1]);
					}
					strcpy(editor.text_lines[editor.cursor_y + 1], "");
					editor.cursor_y++;
					editor.cursor_x = 0;
					editor.total_lines++;
					editor.current_mode = insert_mode;
					editor.has_unsaved_changes = 1;
				}
			}
		}
		else if (editor.current_mode == insert_mode)
		{
			if (editor.clear_status_on_next_key)
			{
				editor.status_message[0] = '\0';
				editor.clear_status_on_next_key = 0;
			}
			if (user_input == '\x1b')
			{
				editor.current_mode = normal_mode;
				if (editor.cursor_x > 0) editor.cursor_x--;
			}
			else if (user_input == '\r')
			{
				split_line_at_cursor();
			}
			else if (user_input == 127 || user_input == ctrl_key('h'))
			{
				delete_character_before_cursor();
			}
			else if (user_input == arrow_left || user_input == arrow_down || user_input == arrow_up || user_input == arrow_right)
			{
				handle_cursor_movement(user_input);
			}
			else if (!iscntrl(user_input))
			{
				insert_character_at_cursor(user_input);
			}
		}
		else if (editor.current_mode == command_mode)
		{
			if (editor.clear_status_on_next_key)
			{
				editor.status_message[0] = '\0';
				editor.clear_status_on_next_key = 0;
			}
			if (user_input == '\x1b')
			{
				editor.current_mode = normal_mode;
				editor.command_length = 0;
			}
			else if (user_input == '\r')
			{
				execute_command();
			}
			else if (user_input == 127 || user_input == ctrl_key('h'))
			{
				if (editor.command_length > 0) editor.command_length--;
			}
			else if (!iscntrl(user_input) && editor.command_length < (int)sizeof(editor.command_buffer) - 1)
			{
				editor.command_buffer[editor.command_length++] = user_input;
			}
		}
	}
	return 0;
}
