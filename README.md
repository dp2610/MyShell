# Name: Divy Patel
# NetID: dp1206

# My Shell Project

## Overview

My shell is a custom shell that replicates fundamental functionalities of traditional Unix shells. This project supports both interactive and batch modes, offering a variety of shell features essential for command line interactions.

## Features

- Interactive Mode: Enables direct user interaction with the shell, where commands are entered and executed one at a time.
- Batch Mode: Executes a series of commands from a specified file, automating the command execution process.
- Built-in Commands: Implements essential shell commands like cd for changing directories, pwd for displaying the current directory
    exit for terminating the shell, and which for locating a command.
- Redirection: Facilitates input (<) and output (>) redirection, allowing users to direct the output of commands to files or use files
    as input.
- Pipes: Supports the use of pipes (|) to pass the output of one command as input to another, enabling complex command chaining.
- Wildcard Expansion: Handles wildcard characters (like * and ?) in file names, allowing users to specify multiple files with pattern
    matching.
- Conditional Execution: Offers basic conditional execution capabilities using then and else constructs, enabling the shell to execute
    commands based on the success or failure of previous commands.
- Background Processes: Allows processes to run in the background when appended with &, enabling continued use of the shell during
    their execution.

## Test Plan

The project cover all features and functionalities of the shell, ensuring clearness and reliability. The test_cases.txt file outlines specific scenarios for testing, while test_input.txt contains batch mode inputs.

Interactive Mode Tests

- Command Execution: Verify basic commands like ls and cat execute correctly and display appropriate outputs.
- Built-in Commands: Test cd to various directories, pwd for correct directory output, exit to ensure shell closure, and which for  
    locating commands.
- Redirection: Test input redirection with commands like cat < input.txt and output redirection with commands like ls > output.txt.
- Pipes: Execute chained commands like ls | sort and grep 'pattern' | wc -l to ensure correct data flow between commands.
- Wildcard Expansion: Use commands like ls *.txt to test the shell's ability to expand wildcards correctly.
- Conditional Execution: Test conditional commands with then and else for correct execution based on previous command success.
- Error Handling: Enter undefined commands to validate appropriate error messages.

Batch Mode Tests

- File Execution: Run a batch file (specified in test_input.txt) containing a series of commands, validating the execution and output 
    of each command.
- Complex Scenarios: Include complex command sequences in the batch file, combining built-ins, redirection, pipes, and conditionals to 
    test comprehensive functionality.

Edge Case Testing

- Empty Commands: Test how the shell handles empty inputs or commands only containing spaces.
- Long Commands: Enter commands that exceed typical length limits to test buffer management.
- Invalid Redirection: Test behavior with incorrect redirection syntax, like missing file names.

## Compilation and Execution

Compile the Shell: make 
Run in Interactive Mode: ./mysh 
Run in Batch Mode: ./mysh file_name.txt
Clean up: make clean