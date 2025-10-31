// Library includes
#include "SHELL.h"

// prototypes
bool handle_builtin(std::vector<char *> &args);

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

std::vector<std::string> split_pipes(const std::string &input)
{
    std::vector<std::string> commands;
    size_t pos = 0;
    std::string s = input;

    while ((pos = s.find('|')) != std::string::npos)
    {
        commands.push_back(trim(s.substr(0, pos)));
        s.erase(0, pos + 1);
    }
    s = trim(s);
    if (!s.empty())
        commands.push_back(s);

    return commands;
}

std::vector<char *> tokenize_input(const std::string &input)
{
    std::vector<char *> tokens;
    char *input_cstr = new char[input.length() + 1];
    std::strcpy(input_cstr, input.c_str());

    char *token = std::strtok(input_cstr, " ");
    while (token != NULL)
    {
        std::string t(token);

        // Expand environment variables
        if (!t.empty() && t[0] == '$')
        {
            const char *val = getenv(t.c_str() + 1); // skip the $
            if (val)
                t = val;
            else
                t = ""; // undefined variable becomes empty
        }

        // Convert back to char* for execvp
        char *tok_cstr = new char[t.length() + 1];
        std::strcpy(tok_cstr, t.c_str());
        tokens.push_back(tok_cstr);

        token = std::strtok(NULL, " ");
    }
    tokens.push_back(NULL); // for execvp compatibility
    delete[] input_cstr;
    return tokens;
}

void execute_pipes(const std::string &input, bool is_background)
{
    std::vector<std::string> pipe_cmds = split_pipes(input);
    int prev_fd = -1; // previous pipe read end
    std::vector<pid_t> pids;

    for (size_t i = 0; i < pipe_cmds.size(); ++i)
    {
        int pipefd[2];
        if (i != pipe_cmds.size() - 1)
            pipe(pipefd); // create pipe except for last command

        pid_t pid = fork();
        if (pid == 0) // child
        {
            if (is_background)
            {
                // Redirect stdin for the *first* command
                if (i == 0)
                {
                    int devNullIn = open("/dev/null", O_RDONLY);
                    if (devNullIn != -1)
                    {
                        dup2(devNullIn, STDIN_FILENO);
                        close(devNullIn);
                    }
                }

                // Redirect stdout/stderr for the *last* command
                if (i == pipe_cmds.size() - 1)
                {
                    int devNullOut = open("/dev/null", O_WRONLY);
                    if (devNullOut != -1)
                    {
                        dup2(devNullOut, STDOUT_FILENO);
                        dup2(devNullOut, STDERR_FILENO);
                        close(devNullOut);
                    }
                }
            }

            if (prev_fd != -1)
            {
                dup2(prev_fd, STDIN_FILENO); // read from previous pipe
                close(prev_fd);
            }

            if (i != pipe_cmds.size() - 1)
            {
                close(pipefd[0]);               // close read end
                dup2(pipefd[1], STDOUT_FILENO); // write to pipe
                close(pipefd[1]);
            }

            handle_redirection(pipe_cmds[i]);

            std::vector<char *> args = tokenize_input(pipe_cmds[i]);

            if (args.empty() || args[0] == NULL)
            {
                // Clean up tokens before exiting
                for (char *arg : args)
                {
                    delete[] arg;
                }
                exit(EXIT_SUCCESS);
            }

            if (!handle_builtin(args))
                execvp(args[0], args.data());

            perror("execvp");
            for (char *arg : args)
            {
                delete[] arg;
            }
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            pids.push_back(pid);

            if (prev_fd != -1)
                close(prev_fd); // close previous read end

            if (i != pipe_cmds.size() - 1)
            {
                close(pipefd[1]);    // close write end
                prev_fd = pipefd[0]; // save read end for next command
            }

            // wait(NULL);
        }
        else
        {
            perror("fork");
        }
    }
    // --- AFTER THE LOOP ---
    // Parent waits for all children ONLY if it's a foreground job
    if (!is_background)
    {
        int status;
        for (pid_t p : pids)
        {
            waitpid(p, &status, 0);
            // Note: 'success' bool for '&&' is not updated here,
            // but this correctly waits for the pipeline.
        }
    }
    else
    {
        // For a background job, just print the PID of the last command
        if (!pids.empty())
        {
            std::cout << BLUE << "[+] Background pipeline started: " << pids.back() << RESET << std::endl;
        }
    }
}

void handle_redirection(std::string &cmd)
{
    int fd;
    size_t pos;

    if ((pos = cmd.find(">>")) != std::string::npos)
    {
        std::string filename = trim(cmd.substr(pos + 2));
        cmd = trim(cmd.substr(0, pos));
        fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            std::cerr << RED << "Error opening file for appending: " << filename << RESET << std::endl;
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    else if ((pos = cmd.find(">")) != std::string::npos)
    {
        std::string filename = trim(cmd.substr(pos + 1));
        cmd = trim(cmd.substr(0, pos));
        fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            std::cerr << RED << "Error opening file for writing: " << filename << RESET << std::endl;
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    else if ((pos = cmd.find("<")) != std::string::npos)
    {
        std::string filename = trim(cmd.substr(pos + 1));
        cmd = trim(cmd.substr(0, pos));
        fd = open(filename.c_str(), O_RDONLY);
        if (fd < 0)
        {
            std::cerr << RED << "Error opening file for reading: " << filename << RESET << std::endl;
            return;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
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

    // export VAR=value
    else if (cmd == "export")
    {
        if (args.size() < 2)
        {
            std::cerr << RED << "export: Invalid arguments" << RESET << std::endl;
            return true;
        }

        std::string assignment(args[1]);
        size_t eq_pos = assignment.find('=');

        if (eq_pos == std::string::npos)
        {
            std::cerr << RED << "export: Invalid format, use VAR=value" << RESET << std::endl;
            return true;
        }

        std::string var = assignment.substr(0, eq_pos);
        std::string value = assignment.substr(eq_pos + 1);

        if (setenv(var.c_str(), value.c_str(), 1) != 0)
            std::cerr << RED << "export: Failed to set variable" << RESET << std::endl;

        return true;
    }

    return false; // not a built-in
}

void print_banner_R(void)
{
    std::cout << R"(
░█▀▀░█░█░▀█▀░█░░░█░░░░░▀█▀░█▀▀░█▀▀░█░█░█▀▀░░░█
░▀▀█░█▀▄░░█░░█░░░█░░░░░░█░░▀▀█░▀▀█░█░█░█▀▀░░░▀
░▀▀▀░▀░▀░▀▀▀░▀▀▀░▀▀▀░░░▀▀▀░▀▀▀░▀▀▀░▀▀▀░▀▀▀░░░▀
)" << std::endl;
}
