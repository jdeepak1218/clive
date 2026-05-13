# Clive - C Live Editor

A minimal, lightweight text editor built from scratch in C, inspired by Vim's modal editing philosophy. Clive runs directly in your terminal and provides a distraction-free editing experience with just the essentials.

## What is Clive?

Clive is a terminal-based text editor that implements core Vim-like functionality without the complexity of a full-featured editor. It operates in raw terminal mode, giving you direct control over every keystroke and screen update. The name "Clive" is a play on "C Live" - a live editor written in C.

## Why Build This?

This project was born from a desire to understand how text editors work at a fundamental level. By building Clive, I learned:

- How terminal control works through escape sequences and raw mode
- The intricacies of cursor positioning and screen rendering
- Modal editing patterns and state management
- File I/O operations and buffer management
- Low-level input handling and keyboard event processing

Building a text editor from scratch strips away all abstractions and forces you to think about every detail - from how backspace works to how lines are stored in memory. It's an exercise in understanding the tools we use every day.

## Features

### Modal Editing
- **Normal Mode**: Navigate and manipulate text with single-key commands
- **Insert Mode**: Type and edit text naturally
- **Command Mode**: Execute editor commands like save and quit

### Core Functionality
- Open and edit existing files or create new ones
- Line-based text buffer supporting up to 1000 lines
- Cursor movement with arrow keys in all modes
- Line numbers displayed alongside your text
- Status bar showing filename, modification status, and cursor position
- Unsaved changes indicator to prevent accidental data loss

### Normal Mode Commands
- `i` - Enter insert mode at cursor position
- `o` - Open a new line below and enter insert mode
- `x` - Delete character under cursor
- `:` - Enter command mode
- Arrow keys - Move cursor

### Insert Mode Commands
- `Esc` - Return to normal mode
- `Enter` - Create new line
- `Backspace` - Delete character before cursor
- Arrow keys - Navigate while editing
- Any printable character - Insert at cursor

### Command Mode
- `:w` - Save current file
- `:w filename` - Save as new filename
- `:q` - Quit (blocked if unsaved changes exist)
- `:q!` - Force quit without saving
- `:wq` - Save and quit
- `Esc` - Cancel command and return to normal mode

## How to Build and Run

### Prerequisites
- GCC or any C compiler
- Unix-like operating system (Linux, macOS, WSL)
- Terminal emulator

### Building
```bash
gcc -o clive clive.c -Wall -Wextra
```

Or create a Makefile:
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = clive

$(TARGET): clive.c
	$(CC) $(CFLAGS) -o $(TARGET) clive.c

clean:
	rm -f $(TARGET)
```

Then build with:
```bash
make
```

### Running
```bash
# Create new file or edit existing
./clive filename.txt

# Start with empty buffer
./clive
```

## Usage Tips

1. Start in normal mode - you cannot type immediately
2. Press `i` to enter insert mode and start typing
3. Press `Esc` to return to normal mode
4. Use `:w` frequently to save your work
5. The status bar shows `[+]` when you have unsaved changes
6. Line numbers help you track your position in the file

## Current Limitations

- Maximum 1000 lines per file
- Maximum 256 characters per line
- No syntax highlighting
- No undo/redo functionality
- No search and replace
- No visual selection mode
- No copy/paste between lines
- No split windows or tabs
- No configuration file

## Future Enhancements

### High Priority
- **Undo/Redo**: Implement a command history stack to reverse changes
- **Search**: Add `/` command to search for text patterns
- **Copy/Paste**: Implement yank and put operations (y, p, d, dd)
- **Visual Mode**: Select and manipulate blocks of text
- **Line Operations**: Delete line (dd), change line (cc), duplicate line

### Medium Priority
- **Syntax Highlighting**: Color coding for common programming languages
- **Multiple Buffers**: Edit multiple files in one session
- **Configuration**: Read settings from a .cliverc file
- **Better Error Messages**: Display helpful messages in the status bar
- **Word Movement**: Jump by words (w, b, e) instead of characters
- **Jump to Line**: Go to specific line number (:123)

### Nice to Have
- **Auto-indentation**: Maintain indentation on new lines
- **Line Wrapping**: Handle lines longer than terminal width
- **Mouse Support**: Click to position cursor
- **Split Windows**: View multiple files side by side
- **Macros**: Record and replay command sequences
- **Plugin System**: Extend functionality without modifying core
- **UTF-8 Support**: Handle international characters properly
- **Incremental Search**: Highlight matches as you type

### Technical Improvements
- **Dynamic Buffer**: Remove line count and length limits
- **Efficient Rendering**: Only redraw changed portions of screen
- **Gap Buffer**: More efficient text insertion and deletion
- **Crash Recovery**: Auto-save and recover from unexpected exits
- **Memory Optimization**: Handle large files without loading entirely into RAM

## Technical Details

Clive uses POSIX terminal control to achieve raw mode input handling. It disables canonical mode, echo, and signal generation to capture every keystroke. Screen updates are performed using ANSI escape sequences for cursor positioning and screen clearing.

The editor maintains a simple array-based buffer where each line is a fixed-size character array. While not the most memory-efficient approach, it keeps the implementation straightforward and easy to understand.

## Contributing

This is a learning project, but suggestions and improvements are welcome. The code prioritizes readability and educational value over performance optimization.

## License

Free to use, modify, and learn from. No warranties provided.
