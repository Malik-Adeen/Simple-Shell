#ifndef SHELL_h
#define SHELL_h
// Library includes
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>

// COLORS for terminal output
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"

// variables
static std::string old_pwd = "";

std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split_commands(std::string input)
{
    std::vector<std::string> commands;
    size_t pos = 0;

    while ((pos = input.find("&&")) != std::string::npos)
    {
        std::string cmd = trim(input.substr(0, pos));
        if (!cmd.empty())
            commands.push_back(cmd);
        input.erase(0, pos + 2); // remove up to and including &&
    }

    input = trim(input);
    if (!input.empty())
        commands.push_back(input);

    return commands;
}

std::vector<char *> tokenize_input(const std::string &input)
{
    std::vector<char *> tokens;
    char *token;
    char *input_cstr = new char[input.length() + 1];
    std::strcpy(input_cstr, input.c_str());
    token = std::strtok(input_cstr, " ");
    while (token != NULL)
    {
        tokens.push_back(token);
        token = std::strtok(NULL, " ");
    }
    tokens.push_back(NULL); // for execvp compatibility
    return tokens;
}

bool handle_builtin(std::vector<char *> &args)
{
    if (args.empty() || args[0] == nullptr)
        return false;

    std::string cmd = args[0];

    if (cmd == "exit")
    {
        std::cout << YELLOW << "Exiting shell..." << RESET << std::endl;
        exit(0);
    }

    else if (cmd == "cd")
    {
        std::string path;

        if (args[1] == NULL)
        {
            // No argument → go to home directory
            const char *home = getenv("HOME");
            if (!home)
            {
                std::cerr << RED << "cd: HOME not set" << RESET << std::endl;
                return true;
            }
            path = std::string(home);
            chdir(path.c_str());
            return true;
        }
        else
        {
            std::string arg = args[1];

            if (arg == "-")
            {
                if (old_pwd.empty())
                {
                    std::cerr << RED << "cd: OLDPWD not set" << RESET << std::endl;
                    return true;
                }
                path = old_pwd;
                std::cout << path << std::endl;
            }
            else if (arg == "~")
            {
                const char *home = getenv("HOME");
                if (!home)
                {
                    std::cerr << RED << "cd: HOME not set" << RESET << std::endl;
                    return true;
                }

                // Replace ~ with $HOME
                path = std::string(home) + arg.substr(1);
            }
            else
            {
                path = arg;
            }
            std::string current_dir(1024, '\0');
            if (getcwd(current_dir.data(), current_dir.size()) != NULL)
                old_pwd = current_dir;

            if (chdir(path.c_str()) != 0)
            {
                std::cerr << RED << "cd: " << "Invalid directory" << RESET << std::endl;
            }
            return true;
        }
    }

    else if (cmd == "help")
    {
        std::cout << CYAN << "Simple Shell Commands:\n"
                  << "  cd <dir>     - Change directory\n"
                  << "  exit         - Exit the shell\n"
                  << "  help         - Show this help menu\n"
                  << "  command && command - Execute sequentially\n"
                  << RESET;
        return true;
    }

    return false; // not a built-in
}

void print_banner_R(void)
{
    std::cout << RED << R"(
  █████████  █████       ████████  ████  ████ 
 ███░░░░░███░░███       ███░░░░███░░███ ░░███ 
░███    ░░░  ░███████  ░░░    ░███ ░███  ░███ 
░░█████████  ░███░░███    ██████░  ░███  ░███ 
 ░░░░░░░░███ ░███ ░███   ░░░░░░███ ░███  ░███ 
 ███    ░███ ░███ ░███  ███   ░███ ░███  ░███ 
░░█████████  ████ █████░░████████  █████ █████
 ░░░░░░░░░  ░░░░ ░░░░░  ░░░░░░░░  ░░░░░ ░░░░░ 
)" << RESET << std::endl;
}

#endif