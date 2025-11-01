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

// This new function splits a command string by '&'
// It assumes '&&' has already been handled.
std::vector<std::string> split_by_ampersand(const std::string &input)
{
    std::vector<std::string> commands;
    size_t pos = 0;
    std::string s = input;

    while ((pos = s.find('&')) != std::string::npos)
    {
        std::string cmd = trim(s.substr(0, pos));
        if (!cmd.empty())
            commands.push_back(cmd);
        s.erase(0, pos + 1);
    }

    std::string remaining = trim(s);
    if (!remaining.empty())
    {
        commands.push_back(remaining);
    }

    return commands;
}

std::vector<char *> tokenize_input(const std::string &input)
{
    std::vector<char *> tokens;
    std::stringstream ss(input);
    std::string token_str;
    char c;

    while (ss.get(c))
    {
        if (std::isspace(c))
            continue;

        token_str.clear();

        if (c == '\"')
        {
            while (ss.get(c) && c != '\"')
            {
                token_str += c;
            }
        }
        else if (c == '\'') // Handle single-quoted string
        {
            while (ss.get(c) && c != '\'')
            {
                token_str += c;
            }
        }
        else
        {
            token_str += c;
            while (ss.peek() != EOF && !std::isspace(ss.peek()))
            {
                ss.get(c);
                token_str += c;
            }
        }

        if (!token_str.empty() && token_str[0] == '$')
        {
            const char *val = getenv(token_str.c_str() + 1);
            if (val)
                token_str = val;
            else
                token_str = "";
        }

        if (!token_str.empty())
        {
            char *tok_cstr = new char[token_str.length() + 1];
            std::strcpy(tok_cstr, token_str.c_str());
            tokens.push_back(tok_cstr);
        }
    }

    tokens.push_back(NULL);
    return tokens;
}

void handle_fg(int jid)
{
    pid_t pid = -1;
    std::string cmd_str = "";
    Job *job_to_fg = nullptr;

    // 1. Find the job in the list
    for (auto &job : jobs_list)
    {
        if (job.jid == jid)
        {
            pid = job.pid;
            cmd_str = job.command;
            job_to_fg = &job;
            break;
        }
    }

    if (pid == -1)
    {
        std::cerr << RED << "fg: job not found: %" << jid << RESET << std::endl;
        return;
    }

    // 2. Give terminal control to the job's process group
    //    We use -pid because 'pid' is the process group ID (since we did setpgid(0,0))
    if (tcsetpgrp(STDIN_FILENO, pid) < 0)
    {
        perror("tcsetpgrp");
        return;
    }

    // 3. Send a "continue" signal (SIGCONT) in case it was stopped
    if (kill(-pid, SIGCONT) < 0)
    {
        perror("kill (SIGCONT)");
        return;
    }

    // 4. Wait for the job to finish (just like a normal foreground job)
    int status;
    waitpid(pid, &status, WUNTRACED); // We'll add WUNTRACED later for Ctrl+Z

    // 5. Take back terminal control
    tcsetpgrp(STDIN_FILENO, getpid());

    // 6. Check how it exited (we'll expand this later)
    if (WIFEXITED(status) || WIFSIGNALED(status))
    {
        // Job finished (normally or by signal), so remove it
        for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it)
        {
            if (it->jid == jid)
            {
                jobs_list.erase(it);
                break;
            }
        }
    }
    else if (WIFSTOPPED(status))
    {
        // Job was stopped again! Update its status.
        if (job_to_fg != nullptr)
        {
            job_to_fg->status = STOPPED;
            std::cout << std::endl
                      << "[" << job_to_fg->jid << "] Stopped\t" << job_to_fg->command << std::endl;
        }
    }
}

void handle_bg(int jid)
{
    pid_t pid = -1;
    Job *job_to_bg = nullptr; // Use a pointer

    // 1. Find the job in the list
    for (auto &job : jobs_list)
    {
        if (job.jid == jid)
        {
            pid = job.pid;
            job_to_bg = &job; // Store a pointer to the job
            break;
        }
    }

    if (pid == -1 || job_to_bg == nullptr)
    {
        std::cerr << RED << "bg: job not found: %" << jid << RESET << std::endl;
        return;
    }

    // 2. Check if the job is actually stopped
    if (job_to_bg->status == RUNNING)
    {
        std::cerr << RED << "bg: job %" << jid << " is already running" << RESET << std::endl;
        return;
    }

    // 3. Send a "continue" signal (SIGCONT)
    if (kill(-pid, SIGCONT) < 0)
    {
        perror("kill (SIGCONT)");
        return;
    }

    // 4. Update the job's status
    job_to_bg->status = RUNNING;
    std::cout << "[" << job_to_bg->jid << "] " << job_to_bg->command << " &" << std::endl;
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
            setpgid(0, 0); // put child in its own process group
            if (!is_background)
            {
                signal(SIGINT, SIG_DFL);  // Reset Ctrl+C to default
                signal(SIGTSTP, SIG_DFL); // Reset Ctrl+Z to default
                if (i == 0)
                {
                    // Give terminal control to the new foreground process group
                    tcsetpgrp(STDIN_FILENO, getpid());
                }
            }
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

        // Take back terminal control
        tcsetpgrp(STDIN_FILENO, getpid());
    }
    else
    {
        // For a background job, just print the PID of the last command
        if (!pids.empty())
        {
            Job new_job;
            new_job.pid = pids.back(); // Use last PID as the representative
            new_job.jid = get_next_jid();
            new_job.command = input; // The whole pipe string
            new_job.status = RUNNING;
            jobs_list.push_back(new_job);

            std::cout << BLUE << "[" << new_job.jid << "] " << new_job.pid << RESET << std::endl;
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
    else if (cmd == "jobs")
    {
        for (const auto &job : jobs_list)
        {
            std::cout << "[" << job.jid << "] "
                      << (job.status == RUNNING ? "Running " : "Stopped ")
                      << "\t" << job.command << std::endl;
        }
        return true;
    }
    else if (cmd == "fg")
    {
        if (args[1] == NULL)
        {
            std::cerr << RED << "fg: expected job ID (e.g., %1)" << RESET << std::endl;
            return true;
        }

        std::string jid_str = args[1];
        if (jid_str[0] == '%') // Remove the '%'
        {
            jid_str = jid_str.substr(1);
        }

        int jid = std::stoi(jid_str);

        // This helper function (which we'll write next) does all the work
        handle_fg(jid);
        return true;
    }
    else if (cmd == "bg")
    {
        if (args[1] == NULL)
        {
            std::cerr << RED << "bg: expected job ID (e.g., %1)" << RESET << std::endl;
            return true;
        }

        std::string jid_str = args[1];
        if (jid_str[0] == '%') // Remove the '%'
        {
            jid_str = jid_str.substr(1);
        }

        int jid = std::stoi(jid_str);

        // This helper function (which we'll write next) does all the work
        handle_bg(jid);
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
