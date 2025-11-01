#include "SHELL.h"

void handle_sigchld(int sig)
{
  (void)sig; // Suppress unused parameter warning
  int status;
  pid_t pid;

  // Reap all terminated children without blocking
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    // Using cout in a signal handler is not strictly safe,
    // but it's common for simple shells.
    std::cout << std::endl
              << BLUE << "[+] Background job finished: " << pid << RESET << std::endl;
  }
}

std::string get_input(void)
{
  std::string buff = "", cwd(1024, '\0');
  if (NULL == getcwd(cwd.data(), cwd.size()))
  {
    std::cerr << RED << "Error getting current working directory" << RESET
              << std::endl;
  }
  std::cout << GREEN << cwd << " $ " << RESET;
  if (!getline(std::cin, buff))
  {
    // Check if getline was interrupted by a signal (like Ctrl+C)
    if (errno == EINTR)
    {
      errno = 0;              // Reset the error flag
      std::cout << std::endl; // Print a newline to clean up the ^C
      return "";              // Return empty string to loop again
    }
    std::cerr << RED << "Error reading input" << RESET << std::endl;
  }
  return buff;
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

  signal(SIGINT, SIG_IGN); // Ignore Ctrl+C in the main shell

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
          if (!is_background)
          {
            signal(SIGINT, SIG_DFL); // Reset Ctrl+C to default
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
            // Background job: Print PID and DO NOT wait
            std::cout << BLUE << "[+] Background job started: " << pid << RESET << std::endl;
            success = true; // '&&' chain considers this a "success"
          }
          else
          {
            // Foreground job: Wait for it to finish
            int status;
            waitpid(pid, &status, 0); // Use waitpid
            success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
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