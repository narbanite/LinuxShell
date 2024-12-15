/*Nikoletta Arvaniti, csd4844*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

#define PATH_MAX 1024

int flag = 1;

void command_prompt(){
    if(flag == 1){
	printf("\e[1;1H\e[2J"); /*regex to clear the shell*/
	flag = 0;
    }
    char *AM = "csd4844";
    char *user = getlogin();
    char current_directory[PATH_MAX];
    
    if(getcwd(current_directory, sizeof(current_directory)) != NULL){
        printf("%s-hy345sh@%s:%s> ", AM, user, current_directory);
    }else{
        perror("getcwd");
    }
    
}

int read_command(char input[]){
    char temp[PATH_MAX] = {0};

    fgets(temp, sizeof(temp), stdin); /*read input, put it inside temp*/
    if(strlen(temp) != 0){
        temp[strcspn(temp, "\n")] = 0; /*get rid of newline*/
        strcpy(input, temp); /*copy temp to input*/
    }else{
        return -1;
    }
    return 1;
}

int separate_command(char *input, char *parameters[], char *delimeter){
    /*strtok to break the input into tokens based on the delimeter*/
    int i = 0, counter = 0;
    char *tokens = strtok(input, delimeter);

    while(tokens != NULL){
        parameters[i] = tokens;
        tokens = strtok(NULL, delimeter);
        i++;
	counter++;
    }
    return counter;
} 


void redirection(char *parameters[]){
    /*INPUT REDIRECTION*/
    for(int i = 0; parameters[i] != NULL; i++){
        if(strcmp(parameters[i], "<") == 0){
            /*open the input file*/
            if(parameters[i+1] != NULL){
                FILE *input_file = fopen(parameters[i+1], "r");
                if(input_file == NULL){
                    perror("Failed to open input file");
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(input_file), STDIN_FILENO);
                fclose(input_file);
            }
            parameters[i] = NULL; /*take out redirection from the parameters*/
            break;
        }
    }

    /*OUTPUT REDIRECTION*/
    for(int i = 0; parameters[i] != NULL; i++){
        if(strcmp(parameters[i], ">") == 0){
            /*open the output file*/
            if(parameters[i+1] != NULL){
                FILE *output_file = fopen(parameters[i+1], "w");
                if(output_file == NULL){
                    perror("Failed to open output file");
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(output_file), STDOUT_FILENO);
                fclose(output_file);
            }
            parameters[i] = NULL; /*take out redirection from the parameters*/
            break;
        }else if(strcmp(parameters[i], ">>") == 0){
            if(parameters[i+1] != NULL){
                FILE *output_file = fopen(parameters[i+1], "a");
                if(output_file == NULL){
                    perror("Failed to open output file");
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(output_file), STDOUT_FILENO);
                fclose(output_file);
            }
            parameters[i] = NULL; /*take out redirection from the parameters*/
            break;
        }
    }
}

void pipes(char *input){
    char *commands[100];
    int commands_count = separate_command(input, commands, "|");

    int pipefds[commands_count - 1][2]; /*pipefds[i][0] for read, pipefds[i][1] for write*/

    for(int i = 0; i < commands_count - 1; i++){
        
        if(pipe(pipefds[i]) == -1){
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < commands_count; i++){
        pid_t pid = fork();
        if(pid == -1){
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){ /*child process*/
            /*an den einai to prwto command tote vale to stdin stou prohgoumenou pipe to read end*/
            if(i != 0){
                dup2(pipefds[i-1][0], STDIN_FILENO);
            }
            /*an den einai to teleutaio command vale to stdout stou epomenoy pipe to write end*/
            if(i != commands_count - 1){
                dup2(pipefds[i][1], STDOUT_FILENO);
            }

            /*CLOSE READ END AND WRITE END*/
            for(int j = 0; j < commands_count - 1; j++){
                close(pipefds[j][0]);
                close(pipefds[j][1]); 
            }

            char *parameters[100];
            int param_count = separate_command(commands[i], parameters, " ");
            parameters[param_count] = NULL;

            /*handle redirection inside the pipes*/
            redirection(parameters);

            if (execvp(parameters[0], parameters) == -1) {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }
    }
    /*CLOSE PIPES*/
    for (int i = 0; i < commands_count - 1; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }
    /*wait all children to finish*/
    for (int i = 0; i < commands_count; i++) {
        wait(NULL);
    }

}



void simple_commands(char *parameters[]){
    /*GLOBAL VARIABLES*/
    if(strchr(parameters[0], '=') != NULL){
        char *name = strtok(parameters[0], "=");/*before "=" is the name*/
        char *value = strtok(NULL, "=");/*after "=" is the value*/
         

        if(name && value){
            if(setenv(name, value, 1) != 0){
                perror("Failed to set environment variable");
            }
            return;
        }
    }

    for(int i = 0; parameters[i] != NULL; i++){
        if(parameters[i][0] == '$'){
            char *var_name = parameters[i] + 1; 
            char *var_value = getenv(var_name);
            if(var_value != NULL){
                parameters[i] = var_value;
            }else{
                parameters[i] = ""; /*if variable is not found, put the empty string*/
            }
        }
    } 

    /*CONTINUING WITH THE SIMPLE COMMANDS*/

    /*if command is "exit" then close the shell*/
    if(strcmp(parameters[0], "exit") == 0){
        exit(0);
    }else if(strcmp(parameters[0], "cd") == 0){/*if cd then take the path (inside parameters[1])*/
    
        if(parameters[1] == NULL){
            fprintf(stderr, "cd: missing argument\n");
        }else if(chdir(parameters[1]) != 0){
            perror("cd failed");
        }
        return;
    }else{
        int status;
        int pid = fork();

        if(pid == 0){ /*if child process then execute*/

            /*REDIRECTION HANDLING*/
            redirection(parameters);

            if (execvp(parameters[0], parameters) == -1) {
                perror("Execution failed");
                exit(EXIT_FAILURE);
            }
        }else if(pid > 0){ /*if parent process then wait for the child to finish*/
            waitpid(pid, &status, 0);
        }else{
            perror("Fork failed");
        }
    }
}


void commands_semicolon(char *input){
    int counter, counter_loop;

    char *parameters[100];
    counter = separate_command(input, parameters, ";");
    parameters[counter] = NULL;
    
    for(int i = 0; i < counter; i++){

        if(strchr(parameters[i], '|') != NULL){
            /*handle pipes*/
            pipes(parameters[i]);
        }else{
            char *prmtrs[100] = {NULL};
            counter_loop = separate_command(parameters[i], prmtrs, " ");
            prmtrs[counter_loop] = NULL;
            simple_commands(prmtrs);
        } 
    }
}

void shell_loop(){
    int status, check, counter;
    char command[PATH_MAX] = {0};
    char *parameters[100] = {NULL};

    while(1){
        command_prompt(); 
        if (read_command(command) != -1){
            
            if (strchr(command, ';') != NULL){ 
                /*semicolon handling*/
                commands_semicolon(command);

            }else if(strchr(command, '|') != NULL ){
                /*pipes handling*/
                pipes(command);
            }else{
                /*simple commands handling (contains global variables)*/
                counter = separate_command(command, parameters, " ");
                parameters[counter] = NULL;
                simple_commands(parameters);
            }
        }
        
    }

}


int main(int argc, char **argv){

    shell_loop();
     
    return 0;
}
