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
### Built-in Commands
- `cd <dir>` — Change the current working directory  
  - Supports `cd -` to return to the previous directory  
  - Supports `cd ~` to go to the home directory  
- `exit` — Exit the shell  
- `help` — Display available commands and usage  
- `export VAR=value` — Set environment variables for the shell session  

### Command Execution
- Execute system commands using `execvp`  
- Supports multiple commands sequentially with `&&`  
- Handles empty commands gracefully  

### Pipes
- Supports single and multiple chained pipes using `|`  
- Example: `ls | grep cpp | wc -l`  

### Environment Variable Expansion
- Supports `$VAR` in commands  
- Example:  
```bash
export MYVAR=Hello
echo $MYVAR
```
### Input/Output Redirection 
- Redirect output using > and >>
``` bash
echo "Hello" > file.txt
echo "World" >> file.txt
cat < file.txt
```
## Build Instructions
```bash
g++ main.cpp shell.cpp -o shell
./shell
```