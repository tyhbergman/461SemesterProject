/* Ty Bergman
03/24/2023
CS 461
ps3 - Simple Shell with I/O Redirection */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // Include fcntl.h for open() function

// Struct to more easily handle command strings
struct ParsedInput {
    std::vector<std::string> tokens;
    std::string input_file;
    std::string output_file;
};

// Function to parse user input into command and arguments
ParsedInput parse_input(const std::string &input) {
    // Create an input string stream from input
    std::istringstream iss(input);

    // String variable to hold individual tokens
    std::string token;

    // Struct to store all tokens
    ParsedInput parsed_input;

    // Parse the input and store tokens in the struct
    while (iss >> token) {
        if (token == "<") {
            iss >> parsed_input.input_file;
        } else if (token == ">") {
            iss >> parsed_input.output_file;
        } else {
            parsed_input.tokens.push_back(token);
        }
    }

    return parsed_input;
};

// Function to execute a command with its arguments
void execute_command(const ParsedInput &parsed_input) {
    // Fork a new process
    pid_t pid = fork();

    // Check if the fork failed
    if (pid < 0) {
        std::cerr << "Fork failed" << std::endl;
        return;
    }

    if (pid == 0) { // Child process
        // Handle input redirection
        if (!parsed_input.input_file.empty()) {
            //Create file descriptor
            int input_fd = open(parsed_input.input_file.c_str(), O_RDONLY);
            if (input_fd < 0) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
            //dup2 to create copy
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        // Handle output redirection
        if (!parsed_input.output_file.empty()) {
            //Create file descriptor with appropriate tags to handle if file already exists or not, and with read/write permission
            int output_fd = open(parsed_input.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd < 0) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            //dup2 to create copy
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Convert the tokens from std::string to char* for the execvp function
        std::vector<char *> argv(parsed_input.tokens.size() + 1);
        for (size_t i = 0; i < parsed_input.tokens.size(); ++i) {
            argv[i] = const_cast<char *>(parsed_input.tokens[i].c_str());
        }
        //Set last element to null for null termination
        argv[parsed_input.tokens.size()] = nullptr;

        // Execute the command
        if (execvp(argv[0], &argv[0]) == -1) {
            std::cerr << "Error executing command" << std::endl;
        }
        exit(EXIT_FAILURE);
    } else { // Parent process
        // Wait for the child process to finish
        waitpid(pid, nullptr, 0);
    }
}


//Main function
int main() {
    // String to hold user input
    std::string input;

    // Infinite loop to keep reading input from the user
    while (true) {
        // Print the shell prompt
        std::cout << "> ";
        // Read a line of input from the user
        std::getline(std::cin, input);

        // If the input is empty, continue to the next iteration
        if (input.empty()) {
            continue;
        }

        // Parse the input into command and arguments using ParsedInput struct
        ParsedInput parsed_input = parse_input(input);

        if (parsed_input.tokens.empty()) {
            continue;
        }

        if (parsed_input.tokens[0] == "exit") {
            break;
        }

        // Check if the command is 'cd' and handle it separately
        if (parsed_input.tokens[0] == "cd") {
            if (parsed_input.tokens.size() == 2) {
                if (chdir(parsed_input.tokens[1].c_str()) != 0) {
                    perror("Error changing directory");
                }
            } else {
                std::cerr << "Usage: cd <directory>" << std::endl;
            }
        } else {
            execute_command(parsed_input);
        }
    }

    return 0;
}

