# Foxy Shell

**Foxy** is a lightweight, feature-rich Unix-like shell for Windows, written in C. It provides a familiar command-line environment with modern productivity features.

## Features

### Core Shell Features
*   **Command Execution**: Run standard Windows commands (`ping`, `dir`, `python`, etc.).
*   **Pipelines**: Chain commands using pipes (`ls | sort | more`).
*   **Redirection**: Redirect I/O using standard operators (`>`, `>>`, `<`).
*   **Logical Operators**: Chain commands with `&&` (AND) and `||` (OR).
*   **Command Sequencing**: Run multiple commands sequentially with `;`.
*   **Quoting**: Supports single (`'`) and double (`"`) quotes for arguments with spaces.
*   **Comments**: Lines starting with `#` are ignored.

### Advanced Productivity
*   **Tab Completion**: Auto-complete filenames and built-in commands.
*   **Command History**: Navigate previous commands with **Up/Down** arrow keys.
*   **Reverse Search**: Press **Ctrl+R** to search your command history.
*   **Persistent History**: History is saved to `.foxy_history` and loaded on startup.
*   **Job Control**:
    *   Run jobs in the background with `&`.
    *   List active jobs with `jobs`.
    *   Bring jobs to the foreground with `fg %id`.
*   **Aliases**: Create shortcuts with `alias name="value"`.
*   **Environment Variables**: usage `$VAR`. Set variables with `export VAR=val`.
*   **Custom Prompt**: Customize your prompt using `prompt` command (supports `$CWD`).
*   **Configuration**: Automatically loads commands from `.foxyrc` at startup.

## Built-in Commands

| Command | Description | Usage |
| :--- | :--- | :--- |
| `cd` | Change directory | `cd <path>` |
| `exit` | Quit the shell | `exit` |
| `help` | Show help message | `help` |
| `echo` | Print arguments | `echo <text>` |
| `prompt`| Set custom prompt | `prompt "$CWD> "` |
| `jobs` | List background jobs | `jobs` |
| `fg` | Foreground a job | `fg %1` |
| `alias` | Define/List alias | `alias ll="ls -l"` |
| `unalias`| Remove alias | `unalias ll` |
| `export`| Set env variable | `export PATH=...` |

## Compilation

Foxy is designed for **Windows** (MinGW/GCC).

```bash
# Compile using make
make

# Or manually with gcc
gcc -Wall -Wextra -std=gnu11 -o foxy src/main.c src/lexer.c src/builtins.c src/parser.c src/exec.c src/jobs.c src/interaction.c src/alias.c
```

## Configuration (`.foxyrc`)

Create a `.foxyrc` file in the same directory as the executable to run startup commands:

```bash
# Example .foxyrc
echo Welcome to Foxy Shell!
prompt "Foxy $CWD $ "
alias ll="ls -l"
alias ga="git add ."
export MY_PROJECT="E:\code\Project"
```

## Usage Examples

```bash
# Pining Google in background
ping google.com &

# Using Pipes
dir /b | sort

# Logic
gcc main.c && ./a.exe || echo "Build Failed"

# History Search
# Press Ctrl+R and type "gcc" to find the last compile command.
```

## Project Structure

*   `src/main.c`: Main REPL loop and initialization.
*   `src/lexer.c`: Tokenizer (handles quotes, operators).
*   `src/parser.c`: Recursive descent parser (builds AST).
*   `src/exec.c`: Executor (process spawning, pipes, redirection).
*   `src/builtins.c`: Implementation of internal commands.
*   `src/jobs.c`: Job control logic.
*   `src/interaction.c`: Line editing, history, and auto-completion.
*   `src/alias.c`: Alias management.
