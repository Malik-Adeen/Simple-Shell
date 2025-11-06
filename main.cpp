#include "SHELL.h"

std::vector<Job> jobs_list;
struct termios orig_termios;
std::vector<std::string> command_history;
int history_index = 0;

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode); // Ensure we restore terminal on exit

    struct termios raw = orig_termios;
    // Disable:
    // ECHO: Don't print characters automatically (we will do it manually)
    // ICANON: Turn off canonical mode (read byte-by-byte, not line-by-line)
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
//

int get_next_jid()
{
  int max_jid = 0;
  for (const auto &job : jobs_list)
  {
    if (job.jid > max_jid)
    {
      max_jid = job.jid;
    }
  }
  return max_jid + 1;
}

void handle_sigchld(int sig)
{
  (void)sig; // Suppress unused parameter warning
  int status;
  pid_t pid;

  // --- REPLACE YOUR OLD FUNCTION WITH THIS ---
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    // A child process finished
    std::string cmd_str = "Unknown";
    bool found = false;

    // Find the job in our list
    for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it)
    {
      if (it->pid == pid)
      {
        cmd_str = it->command;
        jobs_list.erase(it); // Remove from list
        found = true;
        break;
      }
    }

    if (found)
    {
      // Print the "Done" message with the command name
      std::cout << std::endl
                << BLUE << "[" << "Done" << "] " << cmd_str << RESET << std::endl;
    }
  }
}

std::string get_input(void)
{
    std::string cwd(1024, '\0');
    if (NULL == getcwd(cwd.data(), cwd.size()))
    {
        std::cerr << RED << "Error getting current working directory" << RESET
                  << std::endl;
    }
    std::cout << GREEN << cwd << " $ " << RESET << std::flush; // Use flush instead of endl

    enable_raw_mode();

    std::string cmd_buffer;
    int cursor_pos = 0;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        // Check for Enter key (\r or \n)
        if (c == '\r' || c == '\n')
        {
            std::cout << std::endl; // Print a real newline to move to the next line
            break;
        }
        // Check for Ctrl+C (byte value 3)
        else if (c == 3) 
        {
            std::cout << "^C" << std::endl;
            cmd_buffer.clear(); // Clear whatever was typed
            disable_raw_mode(); // Must disable raw mode before returning!
            return "";          // Return empty string to show new prompt
        }
        // Check for Ctrl+D (byte value 4, End of Transmission)
        else if (c == 4)
        {
            if (cmd_buffer.empty())
            {
                std::cout << "exit" << std::endl;
                disable_raw_mode();
                exit(0); // Exit shell on empty Ctrl+D
            }
        }
        else if (c == 127 || c == 8)
        {
           if (cursor_pos > 0)
            {
                // Remove char BEFORE cursor
                cmd_buffer.erase(cursor_pos - 1, 1);
                cursor_pos--;
                
                // Move cursor back one space
                std::cout << "\b" << std::flush;
                
                // Re-print the rest of the line from the new cursor position
                std::cout << cmd_buffer.substr(cursor_pos) << " " << std::flush;
                
                // Move cursor back to its correct position
                // We printed (len - pos) characters, plus one space.
                for (int i = cursor_pos; i < (int)cmd_buffer.length() + 1; i++)
                {
                     std::cout << "\b";
                }
                std::cout << std::flush;
            }
        }
        // ... inside while(read(...)) ...

        // Check for Escape Sequence (Arrow Keys start with \x1b)
        else if (c == 27)
        {
            char seq[2];
            // Try to read the next 2 bytes
            if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
                read(STDIN_FILENO, &seq[1], 1) == 1)
            {
                if (seq[0] == '[')
                {
                    switch (seq[1])
                    {
                       case 'A': // Up Arrow
                            if (history_index > 0)
                            {
                                history_index--;
                                // 1. Erase current line from screen
                                while (cmd_buffer.length() > 0)
                                {
                                    std::cout << "\b \b" << std::flush;
                                    cmd_buffer.pop_back();
                                }
                                // 2. Load command from history
                                cmd_buffer = command_history[history_index];
                                // 3. Print it to screen
                                std::cout << cmd_buffer << std::flush;

                                cursor_pos = cmd_buffer.length();
                            }
                            break;
                        case 'B': // Down Arrow
                            if (history_index < (int)command_history.size())
                            {
                                history_index++;
                                // 1. Erase current line
                                while (cmd_buffer.length() > 0)
                                {
                                    std::cout << "\b \b" << std::flush;
                                    cmd_buffer.pop_back();
                                }
                                // 2. Load command (or empty if at the end)
                                if (history_index < (int)command_history.size())
                                {
                                    cmd_buffer = command_history[history_index];
                                    std::cout << cmd_buffer << std::flush;
                                }
                                cursor_pos = cmd_buffer.length();
                            }
                            break;
                       case 'C': // Right Arrow
                            if (cursor_pos < (int)cmd_buffer.length())
                            {
                                cursor_pos++;
                                std::cout << "\033[C" << std::flush; // ANSI escape to move right
                            }
                            break;
                        case 'D': // Left Arrow
                            if (cursor_pos > 0)
                            {
                                cursor_pos--;
                                std::cout << "\b" << std::flush; // Move cursor back
                            }
                            break;
                    }
                }
            }
        }
        // Handle normal printable characters
        else if (!iscntrl(c)) 
        {
            if (cursor_pos == (int)cmd_buffer.length())
            {
                // Normal append at the end
                cmd_buffer += c;
                cursor_pos++;
                std::cout << c << std::flush;
            }
            else
            {
                // Insert in the middle
                cmd_buffer.insert(cursor_pos, 1, c);
                
                // We have to reprint the WHOLE rest of the line
                std::cout << cmd_buffer.substr(cursor_pos) << std::flush;
                
                // Now we have to move the cursor BACK to where it should be
                // because printing pushed it all the way to the end.
                cursor_pos++;
                for (int i = cursor_pos; i < (int)cmd_buffer.length(); i++)
                {
                     std::cout << "\b";
                }
                std::cout << std::flush;
            }
        }
    }
    disable_raw_mode();
    return cmd_buffer;
}

void shell_launch(std::vector<char *> args)
{

  if (args.size() == 1) // empty input
    return;

  if (fork() == 0)
  {
    if (execvp(args[0], args.data()) == -1)
    {
      std::cerr << RED << "Error executing command: " << args[0] << RESET
                << std::endl;
    }
    exit(EXIT_FAILURE);
  }
  else
  {
    wait(NULL);
  }
}

int main(void)
{
  std::string input;
  print_banner_R();

  // Put shell in its own process group
  if (setpgid(getpid(), getpid()) < 0)
  {
    perror("setpgid");
    exit(EXIT_FAILURE);
  }
  // Take control of the terminal
  if (tcsetpgrp(STDIN_FILENO, getpid()) < 0)
  {
    perror("tcsetpgrp");
    exit(EXIT_FAILURE);
  }

  // Ignore Ctrl+C in the main shell
  signal(SIGINT, SIG_IGN);
  // Ignore terminal write signals (for background processes)
  signal(SIGTTOU, SIG_IGN);
  // Ignore Ctrl+Z (SIGTSTP) in the parent shell
  signal(SIGTSTP, SIG_IGN);

  struct sigaction sa;
  sa.sa_handler = &handle_sigchld; // Set the handler function
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Restart syscalls, don't stop for SIGCHLD
  if (sigaction(SIGCHLD, &sa, 0) == -1)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    input = get_input();
    if (input.empty())
      continue;

    command_history.push_back(input);
    history_index = command_history.size();

    // Outer loop: splits by "&&"
    std::vector<std::string> logical_commands = split_commands(input);
    bool success = true;

    for (auto &cmd_group : logical_commands)
    {
      if (!success)
        break; // Stop processing '&&' chain if a command fails

      // Check if the whole '&&' group ends with &
      bool group_has_trailing_amp = false;
      std::string trimmed_group = trim(cmd_group);
      if (!trimmed_group.empty() && trimmed_group.back() == '&')
      {
        group_has_trailing_amp = true;
      }

      // Inner loop: splits the group by "&"
      std::vector<std::string> bg_commands = split_by_ampersand(cmd_group);

      for (size_t i = 0; i < bg_commands.size(); ++i)
      {
        std::string cmd = bg_commands[i];
        if (cmd.empty())
          continue;

        bool is_background = true; // Assume background since it was split by '&'
        if (i == bg_commands.size() - 1 && !group_has_trailing_amp)
        {
          is_background = false;
        }

        // --- This is your original execution logic ---
        if (cmd.find('|') != std::string::npos)
        {
          execute_pipes(cmd, is_background);
          continue;
        }

        std::vector<char *> args = tokenize_input(cmd);

        if (handle_builtin(args))
        {
          for (char *arg : args)
          {
            delete[] arg;
          }
          continue;
        }

        pid_t pid = fork();

        if (pid < 0) // failure in forking
        {
          std::cerr << RED << "Error forking" << RESET << std::endl;
          success = false;
          break; // Exit inner loop
        }

        if (pid == 0) // --- CHILD PROCESS ---
        {
          setpgid(0, 0);            // this will put the child in its own process group
          signal(SIGINT, SIG_DFL);  // Reset Ctrl+C to default
          signal(SIGTSTP, SIG_DFL); // Reset Ctrl+Z to default
          if (!is_background)
          {
            // Give terminal control to the new foreground job
            tcsetpgrp(STDIN_FILENO, getpid());
          }
          if (is_background)
          {
            // Redirect stdin, stdout, stderr to /dev/null
            int devNullIn = open("/dev/null", O_RDONLY);
            int devNullOut = open("/dev/null", O_WRONLY);
            if (devNullIn != -1)
            {
              dup2(devNullIn, STDIN_FILENO);
              close(devNullIn);
            }
            if (devNullOut != -1)
            {
              dup2(devNullOut, STDOUT_FILENO);
              dup2(devNullOut, STDERR_FILENO);
              close(devNullOut);
            }
          }

          handle_redirection(cmd);

          std::vector<char *> child_args = tokenize_input(cmd);
          if (child_args.empty() || child_args[0] == NULL)
          {
            for (char *arg : child_args)
              delete[] arg;
            exit(EXIT_SUCCESS);
          }
          if (execvp(child_args[0], child_args.data()) == -1)
          {
            std::cerr << RED << "Error executing: " << child_args[0] << RESET
                      << std::endl;
            for (char *arg : child_args)
            {
              delete[] arg;
            }
            exit(EXIT_FAILURE);
          }
        }
        else // --- PARENT PROCESS ---
        {
          if (is_background)
          {
            Job new_job;
            new_job.pid = pid;
            new_job.jid = get_next_jid();
            new_job.command = cmd; // The command string
            new_job.status = RUNNING;
            jobs_list.push_back(new_job);

            // Print [jid] pid
            std::cout << BLUE << "[" << new_job.jid << "] " << new_job.pid << RESET << std::endl;
            success = true; // Allow '&&' chain to continue
          }
          else
          {
            // Foreground job: Wait for it to finish
            int status;
            waitpid(pid, &status, WUNTRACED); // <-- Add WUNTRACED
            success = WIFEXITED(status) && WEXITSTATUS(status) == 0;

            // Take back terminal control
            tcsetpgrp(STDIN_FILENO, getpid());

            if (WIFSTOPPED(status))
            {
              // The job was stopped (Ctrl+Z)
              std::cout << std::endl;
              Job new_job;
              new_job.pid = pid;
              new_job.jid = get_next_jid();
              new_job.command = cmd;
              new_job.status = STOPPED; // <-- Set status
              jobs_list.push_back(new_job);
              std::cout << "[" << new_job.jid << "] Stopped\t" << new_job.command << std::endl;
            }
          }
          for (char *arg : args)
          {
            delete[] arg;
          }
        }

        if (!is_background && !success)
        {
          // The foreground job failed, so stop processing
          // the rest of this '&&' group.
          break;
        }
      } // End of inner 'for' loop (bg_commands)
    } // End of outer 'for' loop (logical_commands)
  } // End of while(1)
  return EXIT_SUCCESS;
}