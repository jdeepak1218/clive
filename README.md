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
- Multiple cursor movement options (arrow keys and Vim-style hjkl)
- Line numbers displayed alongside your text
- Status bar showing filename, modification status, and cursor position
- Unsaved changes indicator to prevent accidental data loss
- Clipboard support for copying and pasting lines
- Bidirectional text search with pattern matching
- Quick line navigation (jump to start/end of line, or jump to specific line via `:number`)

### Normal Mode Commands

#### Navigation
- `h` or `←` - Move cursor left
- `j` or `↓` - Move cursor down
- `k` or `↑` - Move cursor up
- `l` or `→` - Move cursor right
- `0` - Jump to beginning of line
- `$` - Jump to end of line

#### Editing
- `i` - Enter insert mode at cursor position
- `o` - Open a new line below current line and enter insert mode
- `O` - Open a new line above current line and enter insert mode
- `x` - Delete character under cursor
- `d` - Delete current line

#### Copy and Paste
- `y` - Yank (copy) current line to clipboard
- `p` - Put (paste) clipboard content below current line

#### Search
- `/` - Search forward (type search term and press Enter)
- `?` - Search backward (type search term and press Enter)
- `n` - Jump to next search match (forward)
- `N` - Jump to previous search match (backward)

#### Mode Switching
- `:` - Enter command mode

### Insert Mode Commands
- `Esc` - Return to normal mode
- `Enter` - Create new line at cursor position
- `Backspace` - Delete character before cursor
- `h/j/k/l` or Arrow keys - Navigate while editing
- Any printable character - Insert at cursor

### Command Mode

#### File Operations
- `:w` - Save current file
- `:w filename` - Save as new filename
- `:wq` - Save and quit
- `:q` - Quit (blocked if unsaved changes exist)
- `:q!` - Force quit without saving

#### Search Operations
- `/searchterm` - Search forward for "searchterm"
- `?searchterm` - Search backward for "searchterm"

#### General
- `Esc` - Cancel command and return to normal mode
- `Backspace` - Delete character in command buffer

## Quick Install (One-Liner)

Install the latest version of Clive in one command — no compiler needed, works on Linux and macOS:

```bash
curl -fsSL https://raw.githubusercontent.com/jdeepak1218/clive/main/install.sh | bash
```

After installation, you can use `clive` from anywhere:
```bash
clive myfile.txt
```

### What it does
1. Detects your OS (Linux/macOS) and architecture (x86_64/ARM64)
2. Downloads the matching pre-built binary from GitHub Releases
3. Installs it to `/usr/local/bin`

### Checking your version
```bash
clive --version
# Clive version v1.0.0
```

### Updating to the latest version
```bash
clive --update
# or manually re-run the install:
curl -fsSL https://raw.githubusercontent.com/jdeepak1218/clive/main/install.sh | bash
```

### Getting help
```bash
clive --help
```

## How to Build and Run

### Prerequisites
- GCC or any C compiler
- Unix-like operating system (Linux, macOS, WSL)
- Terminal emulator

### Building from Source
```bash
gcc -o clive clive.c -Wall -Wextra
```

Or use the provided Makefile:
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

## Complete Command Reference

### Normal Mode

| Command | Action | Example Use Case |
|---------|--------|------------------|
| `h` | Move left | Navigate character by character |
| `j` | Move down | Navigate line by line |
| `k` | Move up | Navigate line by line |
| `l` | Move right | Navigate character by character |
| `←↓↑→` | Arrow key movement | Alternative to hjkl |
| `0` | Jump to line start | Quick navigation to beginning |
| `$` | Jump to line end | Quick navigation to end |
| `i` | Enter insert mode | Start typing at cursor |
| `o` | Open line below | Add new line and start typing |
| `O` | Open line above | Insert line before current |
| `x` | Delete character | Remove single character |
| `d` | Delete line | Remove entire line |
| `y` | Yank (copy) line | Copy line to clipboard |
| `p` | Put (paste) line | Paste clipboard below |
| `/text` | Search forward | Find "text" going down |
| `?text` | Search backward | Find "text" going up |
| `n` | Next match | Jump to next search result |
| `N` | Previous match | Jump to previous search result |
| `:` | Command mode | Execute editor commands |

### Insert Mode

| Command | Action |
|---------|--------|
| `Esc` | Exit to normal mode |
| `Enter` | New line at cursor |
| `Backspace` | Delete previous character |
| `Arrow keys` | Navigate while inserting |
| `h/j/k/l` | Navigate while inserting |
| Any character | Insert at cursor position |

### Command Mode

| Command | Action | Notes |
|---------|--------|-------|
| `:w` | Write (save) file | Requires filename |
| `:w filename` | Save as filename | Sets new filename |
| `:q` | Quit editor | Blocked if unsaved changes |
| `:q!` | Force quit | Discards unsaved changes |
| `:wq` | Write and quit | Save then exit |
| `:number` | Jump to line number | E.g., `:42` jumps to line 42 |
| `/pattern` | Search forward | Moves cursor to match |
| `?pattern` | Search backward | Moves cursor to match |
| `Esc` | Cancel command | Return to normal mode |

### CLI Flags

| Flag | Action |
|------|--------|
| `clive --version` / `-v` | Show installed version |
| `clive --help` / `-h` | Show help and usage |
| `clive --update` / `-u` | Update to the latest version |

## Usage Tips

1. Start in normal mode - you cannot type immediately
2. Press `i` to enter insert mode and start typing
3. Press `Esc` to return to normal mode from any other mode
4. Use `:w` frequently to save your work
5. The status bar shows `[+]` when you have unsaved changes
6. Line numbers help you track your position in the file
7. Use `h`, `j`, `k`, `l` for Vim-style navigation or arrow keys if you prefer
8. Search with `/` for forward search or `?` for backward search
9. After searching, use `n` to jump to next match or `N` for previous match
10. Copy a line with `y` and paste it below with `p`
11. Delete entire lines quickly with `d`
12. Use `0` and `$` to jump to line boundaries instantly
13. Jump to any line instantly with `:42` (replace 42 with desired line number)

## Common Workflows

### Editing a File
```bash
./clive myfile.txt    # Open file
i                     # Enter insert mode
# Type your content
Esc                   # Return to normal mode
:w                    # Save
:q                    # Quit
```

### Jumping to a Specific Line
```bash
:42                   # Jump directly to line 42
:1                    # Jump to the first line
:$                    # Jump to the last line (note: $ is not yet supported)
```

### Searching and Replacing Text
```bash
/searchterm           # Find text forward
n                     # Jump to next occurrence
i                     # Enter insert mode to edit
# Make your changes
Esc                   # Back to normal mode
n                     # Continue to next match
```

### Copying and Rearranging Lines
```bash
y                     # Yank (copy) current line
j                     # Move down
p                     # Paste below current line
d                     # Delete a line you don't need
```

### Quick Navigation
```bash
0                     # Jump to start of line
$                     # Jump to end of line
k k k                 # Move up 3 lines
j j                   # Move down 2 lines
```

## Current Limitations

- Maximum 1000 lines per file
- Maximum 256 characters per line
- No syntax highlighting
- No undo/redo functionality
- Clipboard only holds one line at a time
- No visual selection mode
- No multi-line copy/paste operations
- No split windows or tabs
- No configuration file
- Search wraps around but doesn't indicate when it does

## Future Enhancements

### High Priority
- **Undo/Redo**: Implement a command history stack to reverse changes
- **Search and Replace**: Add `:s/old/new/` command for text substitution
- **Visual Mode**: Select and manipulate blocks of text (v, V commands)
- **Multi-line Clipboard**: Copy and paste multiple lines (dd, yy with counts)
- **Search Wrap Indicator**: Show message when search wraps around file
- **Regex Search**: Support regular expressions in search patterns

### Medium Priority
- **Syntax Highlighting**: Color coding for common programming languages
- **Multiple Buffers**: Edit multiple files in one session
- **Configuration**: Read settings from a .cliverc file
- **Better Error Messages**: Display helpful messages in the status bar
- **Word Movement**: Jump by words (w, b, e) instead of characters


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
