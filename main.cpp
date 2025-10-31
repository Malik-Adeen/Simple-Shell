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

    std::vector<std::string> commands = split_commands(input);
    bool success = true;

    for (auto &cmd : commands)
    {
      if (!success)
        break;

      bool is_background = false;
      std::string trimmed_cmd = trim(cmd);
      if (!trimmed_cmd.empty() && trimmed_cmd.back() == '&')
      {
        is_background = true;
        cmd = trim(trimmed_cmd.substr(0, trimmed_cmd.length() - 1));
      }

      if (cmd.find('|') != std::string::npos)
      {
        execute_pipes(cmd, is_background);
        // Note: 'success' is not correctly updated by execute_pipes
        // for the '&&' chain. We'll assume success for now.
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
      }

      if (pid == 0)
      {

        if (is_background)
        {
          // Redirect stdin from /dev/null
          int devNullIn = open("/dev/null", O_RDONLY);
          if (devNullIn != -1)
          {
            dup2(devNullIn, STDIN_FILENO);
            close(devNullIn);
          }

          // Redirect stdout and stderr to /dev/null
          int devNullOut = open("/dev/null", O_WRONLY);
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
      else
      {
        if (is_background)
        {
          // Background job: Print PID and DO NOT wait
          std::cout << BLUE << "[+] Background job started: " << pid << RESET << std::endl;
          success = true; // Allow '&&' chain to continue
        }
        else
        {
          // Foreground job: Wait for it to finish
          int status;
          wait(&status);
          success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
        for (char *arg : args)
        {
          delete[] arg;
        }
      }
    }
  }
  return EXIT_SUCCESS;
}