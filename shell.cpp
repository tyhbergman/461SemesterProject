#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

//define structure to hold parsed input
struct ParsedInput {
    std::vector<std::string> tokens; //command and arguments
    std::string input_file; // input file for redirection
    std::string output_file; //output file for redirection
};

//function to parse the input command line
ParsedInput parse_input(const std::string &input) {
    std::istringstream iss(input);
    std::string token;
    ParsedInput parsed_input;

    //loop through each token in the input string
    while (iss >> token) {
        //if token is '<', the next token isinput file for redirection
        if (token == "<") {
            iss >> parsed_input.input_file;
        } 
        //if token is '>', the next token is output file for redirection
        else if (token == ">") {
            iss >> parsed_input.output_file;
        } 
        //if token is neither '<' nor '>', it is part of command or its arguments
        else {
            parsed_input.tokens.push_back(token);
        }
    }
    return parsed_input;
};

//function to execute command without pipes
void execute_command(const ParsedInput &parsed_input) {
    //fork new process
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed" << std::endl;
        return;
    }

    //child
    if (pid == 0) {
        if (!parsed_input.input_file.empty()) {
            
            int input_fd = open(parsed_input.input_file.c_str(), O_RDONLY);
            //if file opening failed print error message
            if (input_fd < 0) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
            //redirect stdin to the input file
            dup2(input_fd, STDIN_FILENO);
            //close the input file descriptor
            close(input_fd);
        }

        // If there is an output file for redirection
        if (!parsed_input.output_file.empty()) {

            int output_fd = open(parsed_input.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            //if file opening failed print error message
            if (output_fd < 0) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            //redirect stdout to the output file
            dup2(output_fd, STDOUT_FILENO);
            //close the output file descriptor
            close(output_fd);
        }

        //convert the command and arguments to work with execvp
        std::vector<char *> argv(parsed_input.tokens.size() + 1);
        for (size_t i = 0; i < parsed_input.tokens.size(); ++i) {
            argv[i] = const_cast<char *>(parsed_input.tokens[i].c_str());
        }
        //last element must be null pointer
        argv[parsed_input.tokens.size()] = nullptr;

                //execute
        if (execvp(argv[0], &argv[0]) == -1) {
            std::cerr << "Error executing command" << std::endl;
        }
        //error handling
        exit(EXIT_FAILURE);
    } else {
        // parent process: wait for child to finish
        waitpid(pid, nullptr, 0);
    }
}

// Function to execute a series of commands with pipes
void execute_command_with_pipe(std::vector<std::vector<std::string>> commands) {
    int fd[2]; 
    size_t num_cmds = commands.size(); // # of commands to execute
    int saved_stdin = dup(STDIN_FILENO);

    //iterate through commands
    for (size_t i = 0; i < num_cmds; ++i) {
        pipe(fd);
        pid_t pid = fork(); // new forl

        if (pid < 0) {
            std::cerr << "Fork failed" << std::endl;
            return;
        }

        //child
        if (pid == 0) {
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }
            //close the read end of the pipes
            close(fd[0]);
            close(fd[1]);

            //convert the command and arguments work with execvp
            std::vector<char *> argv(commands[i].size() + 1);
            for (size_t j = 0; j < commands[i].size(); ++j) {
                argv[j] = const_cast<char *>(commands[i][j].c_str());
            }
            //last element must be null pointer
            argv[commands[i].size()] = nullptr;

            if (execvp(argv[0], &argv[0]) == -1) {
                std::cerr << "Error executing command" << std::endl;
                exit(EXIT_FAILURE);
            }
        } else {
            waitpid(pid, nullptr, 0);
            // Redirect stdin to the read end of the pipe
            dup2(fd[0], STDIN_FILENO);
            //close the read end of the pipes
            close(fd[0]);
            close(fd[1]);
        }
    }
    //restore the original stdin file descriptor
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
}

int main() {
    std::string input;

    while (true) {
        //promptuser for input
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        ParsedInput parsed_input = parse_input(input);

        if (parsed_input.tokens.empty()) {
            continue;
        }

        if (parsed_input.tokens[0] == "exit") {
            break;
        } 
        // look for change directory
        else if (parsed_input.tokens[0] == "cd") {
            if (parsed_input.tokens.size() == 2) { 
                if (chdir(parsed_input.tokens[1].c_str()) != 0) { 
                    perror("Error changing directory");
                }
            } else {
                std::cerr << "Usage: cd <directory>" << std::endl;
            }
        } 

        else if (parsed_input.tokens[0] == "pwd") {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                std::cout << cwd << std::endl;
            } else {
                perror("Error getting current directory");
            }
        } 
        // look for mkdir
        else if (parsed_input.tokens[0] == "mkdir" && parsed_input.tokens.size() == 2) {
            if (mkdir(parsed_input.tokens[1].c_str(), 0777) == -1) {
                perror("Error creating directory");
            }
        } 
        // look for echo
        else if (parsed_input.tokens[0] == "echo") {
            for (size_t i = 1; i < parsed_input.tokens.size(); ++i) {
                printf("%s ", parsed_input.tokens[i].c_str());
            }
            printf("\n");
        } 

        else {
            std::vector<std::vector<std::string>> piped_commands;
            std::vector<std::string> command;
            for (const auto& token : parsed_input.tokens) {
                if (token == "|") {
                    piped_commands.push_back(command);
                    command.clear();
                } else {
                    command.push_back(token);
                }
            }
            piped_commands.push_back(command);

            //look for multiple commands
            if (piped_commands.size() > 1) {
                execute_command_with_pipe(piped_commands);
            } 
            // or execute single command
            else {
                execute_command(parsed_input);
            }
        }
    }

    return 0;
}
