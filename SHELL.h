#ifndef SHELL_h
#define SHELL_h
// Library includes
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sstream>
// COLORS for terminal output
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"

// variables
static std::string old_pwd = "";

// prototypes
bool handle_builtin(std::vector<char *> &args);
std::string trim(const std::string &s);
std::vector<std::string> split_commands(std::string input);
std::vector<std::string> split_pipes(const std::string &input);
std::vector<char *> tokenize_input(const std::string &input);
std::vector<std::string> split_by_ampersand(const std::string &input);
void execute_pipes(const std::string &input, bool is_background);
bool handle_builtin(std::vector<char *> &args);
void handle_redirection(std::string &cmd);
void print_banner_R(void);
#endif