/* Ty Bergman
03/20/2023
CS 461
ps2 - Simple Shell */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Function to parse user input into command and arguments
std::vector<std::string> parse_input(const std::string &input) {
    // Create an input string stream from input
    std::istringstream iss(input);
    // String variable to hold individual tokens
    std::string token;
    // Vector to store all tokens
    std::vector<std::string> tokens;

    // Extract tokens from input string stream and store them in the vector
    while (iss >> token) {
        tokens.push_back(token);
    }

    // Return the vector of tokens
    return tokens;
}

// Function to execute a command with its arguments
void execute_command(const std::vector<std::string> &tokens) {
    // Fork a new process
    pid_t pid = fork();

    // Check if the fork failed
    if (pid < 0) {
        std::cerr << "Fork failed" << std::endl;
        return;
    }

    // Child process
    if (pid == 0) {
        // Convert the vector of strings into a vector of char pointers
        std::vector<char *> argv(tokens.size() + 1);

        // Fill the argv vector with pointers to the tokens
        for (size_t i = 0; i < tokens.size(); ++i) {
            argv[i] = &tokens[i][0];
        }
        // Set the last element of argv to nullptr
        argv[tokens.size()] = nullptr;

        // Execute the command with its arguments
        if (execvp(argv[0], &argv[0]) == -1) {
            std::cerr << "Error executing command" << std::endl;
        }
        // Exit the child process if the command execution fails
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

        // Parse the input into command and arguments
        std::vector<std::string> tokens = parse_input(input);

        // If the command is "exit", break the loop and exit the program
        if (tokens[0] == "exit") {
            break;
        }

        // Check if the command is 'cd' and handle it separately
        if (tokens[0] == "cd") {
            // Ensure that the command has the correct number of arguments
            if (tokens.size() == 2) {
                // Attempt to change the directory and check for errors
                if (chdir(tokens[1].c_str()) != 0) {
                    // Print an error message if the directory change failed
                    perror("Error changing directory");
                }
            } else {
                // Print a usage message if the command has the wrong number of arguments
                std::cerr << "Usage: cd <directory>" << std::endl;
            }
        } else {
            // If the command is not 'cd', execute the command with its arguments
            execute_command(tokens);
        }
    }

    // Exit the program
    return 0;
}

