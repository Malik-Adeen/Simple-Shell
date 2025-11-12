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
#include <algorithm>
#include <dirent.h>
#include <sstream>
#include <cerrno>
#include <vector>
#include <termios.h> // to handle raw input from the terminal

// COLORS for terminal output
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"

enum JobStatus
{
    RUNNING,
    STOPPED // For later when we add Ctrl+Z
};

struct Job
{
    int jid;
    pid_t pid;
    std::string command;
    JobStatus status;
};

// variables
extern std::vector<Job> jobs_list;
static std::string old_pwd = "";

// prototypes
bool handle_builtin(std::vector<char *> &args);
int get_next_jid();
std::string trim(const std::string &s);
std::vector<std::string> split_commands(std::string input);
std::vector<std::string> split_pipes(const std::string &input);
std::vector<char *> tokenize_input(const std::string &input);
std::vector<std::string> split_by_ampersand(const std::string &input);
void enable_raw_mode();
void handle_tab_completion(std::string& cmd_buffer, int& cursor_pos);  
void disable_raw_mode(); 
void handle_fg(int jid);
void handle_bg(int jid);
void execute_pipes(const std::string &input, bool is_background);
bool handle_builtin(std::vector<char *> &args);
void handle_redirection(std::string &cmd);
void print_banner_R(void);
#endif