#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define MAX_HISTORY 100

typedef struct{
    char input[MAX_INPUT];
    char command[MAX_INPUT];
    pid_t pid;
    time_t start_time;
    double duration;
}command_history;

command_history history[MAX_HISTORY];
int history_count = 0;

void add_to_history(const char *input, const char *command, pid_t pid, time_t start_time, double duration){
    if(history_count < MAX_HISTORY){
        strncpy(history[history_count].input, input, MAX_INPUT);
        strncpy(history[history_count].command, command, MAX_INPUT);
        history[history_count].pid = pid;
        history[history_count].start_time = start_time;
        history[history_count].duration = duration;
        history_count++;
    }else{
        printf("ERROR: History Limit Excedded.\n");
    }
}

void display_all_info(){
    for(int i=0; i<history_count; i++){
        printf("%d: %s (PID: %d, Start Time: %sDuration: %.2f seconds)\n", 
            i+1, history[i].input, history[i].pid, 
            ctime(&history[i].start_time), history[i].duration);
    }
}

void display_history(){
    for(int i=0; i<history_count; i++){
        printf("%s", history[i].input);
    }
}

void parse_input(char *user_input, char **command, char **args){
    char *token;
    int arg_index = 0;

    token = strtok(user_input, " \t\r\n");
    if(token == NULL){
        *command = NULL;
        return;
    }

    *command = token;

    while(token != NULL && arg_index < MAX_ARGS-1){
        args[arg_index++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    args[arg_index] = NULL;
}

void run_command(char *input, char *command, char **args){
    pid_t pid = fork();
    if(pid < 0){
        printf("ERROR: Fork Failed\n");
        return;
    }
    if(pid == 0){ 
        if(execvp(command, args) < 0){
            printf("ERROR: Command Execution Failed: %s\n", command);
            exit(1);
        }
        exit(0);
    }else{ 
        time_t start_time = time(NULL); 
        int status;
        waitpid(pid, &status, 0);
        time_t end_time = time(NULL);
        double duration = difftime(end_time, start_time);
        add_to_history(input, command, pid, start_time, duration);
    }
}

void run_piped_commands(char *input){
    char *command;
    char *args[MAX_ARGS];
    char *pipe_cmds[10];
    int pipe_count = 0;

    char user_input[MAX_INPUT];
    strncpy(user_input, input, MAX_INPUT);

    command = strtok(input, "|");
    while(command != NULL && pipe_count < 10){
        pipe_cmds[pipe_count++] = command;
        command = strtok(NULL, "|");
    }

    int pipes[2*(pipe_count-1)];

    for(int i=0; i<pipe_count-1; i++){
        if(pipe(pipes + i*2) < 0){
            printf("ERROR: Pipe Creation Failed");
            return;
        }
    }

    time_t start_time = time(NULL);
    pid_t pid1=0;

    for(int i=0; i<pipe_count; i++){
        char *args[MAX_ARGS];
        parse_input(pipe_cmds[i], &args[0], args);
        pid_t pid = fork();
        if(pid < 0){
            printf("ERROR: Fork Failed");
            return;
        }
        if(pid == 0){
            if(i < pipe_count-1){
                dup2(pipes[i*2+1], STDOUT_FILENO);
            }
            if(i > 0){
                dup2(pipes[(i-1)*2], STDIN_FILENO);
            }
            for(int j=0; j<2*(pipe_count-1); j++){
                close(pipes[j]);
            }
            execvp(args[0], args);
            printf("ERROR: Command Execution Failed");
            exit(1);
        }
        if(pid > 0){
            if(i == 0) pid1 = pid;
        }
    }

    for(int i=0; i<2*(pipe_count-1); i++){
        close(pipes[i]);
    }

    for(int i=0; i<pipe_count; i++){
        wait(NULL);
    }

    time_t end_time = time(NULL);
    double duration = difftime(end_time, start_time);
    add_to_history(user_input, user_input, pid1, start_time, duration);
}

void launch(char *input, char *command, char **args){
    run_command(input, command, args);
}

static void my_handler(int signum){
    if(signum == SIGINT){
        printf("\n");
        display_all_info();
        exit(0);
    }
}

void shell_loop(){

    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = my_handler;
    sigaction(SIGINT, &sig, NULL);

    int status = 1;
    char *shell_user = "SimpleShell@abhishek: ";
    char user_input[MAX_INPUT];
    char input[MAX_INPUT];
    char *command;
    char *args[MAX_ARGS];

    do{
        printf("%s", shell_user);
        if(fgets(user_input, MAX_INPUT, stdin) == NULL){
            break;
        }
        strncpy(input, user_input, MAX_INPUT);
        if(strchr(user_input, '|') != NULL){
            run_piped_commands(user_input);
        }else{
            parse_input(user_input, &command, args);
            if(command == NULL){
                continue;
            }
            if(strcmp(command, "history") == 0){
                display_history();
            }else{
                launch(input, command, args);
            }
        }
    }while(status);
}

int main(){
    shell_loop();
    return 0;
}
