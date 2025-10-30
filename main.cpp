#include "SHELL.h"

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

      if (cmd.find('|') != std::string::npos)
      {
        execute_pipes(cmd);
        continue;
      }

      std::vector<char *> args = tokenize_input(cmd);

      if (handle_builtin(args))
        continue;

      if (fork() == 0)
      {
        if (execvp(args[0], args.data()) == -1)
        {
          std::cerr << RED << "Error executing: " << args[0] << RESET
                    << std::endl;
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        int status;
        wait(&status);
        success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
      }
    }
  }
  return EXIT_SUCCESS;
}