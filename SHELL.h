#ifndef SHELL_h
# define SHELL_h

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>

//COLORS for terminal output
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[1;31m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"


std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
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



void print_banner_R(void)
{
    std::cout << RED << R"(
███╗   ███╗██╗   ██╗███████╗██╗  ██╗███████╗██╗     ██╗         ██╗   ██╗ ██╗
████╗ ████║╚██╗ ██╔╝██╔════╝██║  ██║██╔════╝██║     ██║         ██║   ██║███║
██╔████╔██║ ╚████╔╝ ███████╗███████║█████╗  ██║     ██║         ██║   ██║╚██║
██║╚██╔╝██║  ╚██╔╝  ╚════██║██╔══██║██╔══╝  ██║     ██║         ╚██╗ ██╔╝ ██║
██║ ╚═╝ ██║   ██║   ███████║██║  ██║███████╗███████╗███████╗     ╚████╔╝  ██║
╚═╝     ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝      ╚═══╝   ╚═╝
)" << RESET << std::endl;
}


#endif