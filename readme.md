<p align="center">
<pre>
       ░█▀▀░█░█░█▀▀░▀█▀░█▀█░█▄█░░░█▀▀░█░█░█▀▀░█░░░█░░░░░▀█▀░█▀█░░░█▀▀░█▀█░█▀█
       ░█░░░█░█░▀▀█░░█░░█░█░█░█░░░▀▀█░█▀█░█▀▀░█░░░█░░░░░░█░░█░█░░░█░░░█▀▀░█▀▀
       ░▀▀▀░▀▀▀░▀▀▀░░▀░░▀▀▀░▀░▀░░░▀▀▀░▀░▀░▀▀▀░▀▀▀░▀▀▀░░░▀▀▀░▀░▀░░░▀▀▀░▀░░░▀░░
</pre>
</p>

<p align="center">
<b>A Simple Unix-like Shell built in C++</b>
</p>

### 🚀 Features

### Command Execution

  * Execute system commands using `execvp`.
  * Supports multiple commands sequentially with `&&`.
  * Handles empty commands gracefully.

### 🧠 Smart Parsing & Variable Expansion

  * Handles arguments with spaces using both **single (`'`)** and **double (`"`)** quotes.
  ```bash
  echo "This is one argument"
  touch 'a file with spaces.txt'
  ```
  * Supports **variable expansion (`$VAR`)**.
    * Variables are expanded inside `"` (double quotes).
    * Variables are **not** expanded inside `'` (single quotes), matching standard shell behavior.
  <!-- end list -->
  ```bash
  export MY_VAR="World"
  echo "Hello $MY_VAR"   # Prints "Hello World"
  echo 'Hello $MY_VAR'   # Prints "Hello $MY_VAR"
  ```

### Pipes & Redirection

  * Supports single and multiple chained pipes using `|`.
  ```bash
  ls | grep cpp | wc -l
  ```
  * Redirect output using `>` (overwrite) and `>>` (append).
  * Redirect input using `<`.
  ```bash
  echo "Hello" > file.txt
  echo "World" >> file.txt
  cat < file.txt
  ```

### 🔧 Full Job Control

The shell provides a complete job control system, allowing for true multitasking.

  * **Background Execution (`&`):** Run any command in the background. `&` is supported as both a suffix and a command separator.
  ```bash
  sleep 10 &
  sleep 1 & sleep 2 &
  ```
  * **Signal Handling:**
    * `Ctrl+C` (`SIGINT`): Terminates the current **foreground job** without exiting the shell.
    * `Ctrl+Z` (`SIGTSTP`): Stops the current **foreground job** and moves it to the background.
  * **Job Management Commands:**
    * `jobs`: List all jobs (Running or Stopped) with their job ID (JID).
    * `fg %<jid>`: Bring a job to the **foreground**.
    * `bg %<jid>`: Resume a *stopped* job in the **background**.

### Built-in Commands

  * `cd <dir>` — Change the current working directory.
    * Supports `cd -` (previous directory) and `cd ~` (home directory).
  * `exit` — Exit the shell.
  * `help` — Display available commands and usage.
  * `export VAR=value` — Set environment variables for the session.
  * `jobs` — List all active background and stopped jobs.
  * `fg %<jid>` — Bring a job to the foreground.
  * `bg %<jid>` — Resume a stopped job in the background.

## Build Instructions

```bash
g++ main.cpp shell.cpp -o shell
./shell
```