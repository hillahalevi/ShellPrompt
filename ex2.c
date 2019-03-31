


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <wait.h>
#include <memory.h>

#define MAX_COMMAND 1024
#define MAX_COMMAND_ARGS 512
#define MAX_BACKGROUND_PROCESSES 512

#define PRINT_ERROR fprintf(stderr, "Error in system call");
#define UPDATE_WD strcpy(previousPwd,currentPwd);
#define PRINT_PROMPT_SIGN printf("> ");
#define NOT_SET "not set"
#define HOME "HOME"
#define EXIT "/bin/exit"
#define CD "/bin/cd"
#define JOBS "/bin/jobs"
#define MAN "/bin/man"


/*Process struct contains pid and cmd the process do*/
typedef struct Process {
    pid_t pid;
    char cmd[MAX_COMMAND];
} Process;

typedef struct CommandInfo {
    char *cmd;
    char *command;
    char *argv[MAX_COMMAND_ARGS + 1];
    int lastArgIndex;
    int backgroundFlag;

} CommandInfo;

/**
 * use strcmp to see if 2 strings r eql
 * @param a
 * @param b
 * @return true/ false
 */
bool equal(char *a, char *b) {
    if (strcmp(a, b) == 0) {
        return true;
    }
    return false;

}

/**
 * separate the command line into an array of commands and returns the amount
 * of args in total
 * @param cmd_args the array to store the arguments
 * @param cmdLine the command line to separate
 * @return number of arguments
 */
int separate(char cmd_args[MAX_COMMAND_ARGS][MAX_COMMAND], char *cmdLine) {
    char cmd_copy[MAX_COMMAND];
    strcpy(cmd_copy, cmdLine);
    /* adding the right format for command*/
    char first[MAX_COMMAND] = "/bin/";
    char *arg = strtok(cmd_copy, " ");
    arg = strcat(first, arg);

    int numOfArgs = 0;
    while (arg != NULL) {
        stpcpy(cmd_args[numOfArgs], arg);
        arg = strtok(NULL, " ");
        numOfArgs++;
    }
    return numOfArgs;


}

/**
 * set the Parms info according to the right format and the command
 * @param cmdLine command line
 * @param info the parms info
 */
void setArgs(char *cmdLine, CommandInfo *info) {

    // cmdArgs is an array of strings so it contains the command name and then the args following
    char cmdArgs[MAX_COMMAND_ARGS][MAX_COMMAND];
    int argsNum = separate(cmdArgs, cmdLine);

    //add null at the end for the execv format
    info->backgroundFlag = 0;
    info->command = cmdArgs[0];
    info->cmd = cmdLine;
    int i;
    for (i = 0; i < argsNum; i++) {

        if (i == argsNum - 1 && equal(cmdArgs[i], "&")) {
            info->backgroundFlag = 1;
            break;
        }
        info->argv[i] = cmdArgs[i];
    }
    info->argv[i] = NULL;
    // index of last real arg
    info->lastArgIndex = i - 1;

}


/**
 * clear intake of the command
 * @param command_line 
 */
void intake(char *command_line) {
    scanf(" ");
    fgets(command_line, MAX_COMMAND - 1, stdin);
    command_line[strlen(command_line) - 1] = '\0';
}


/**
 * try chdir to specific path
 * @param path
 * @param previousPwd
 * @param currentPwd
 */
void chdirTry(char *path, char *previousPwd, char *currentPwd) {
    if (chdir(path) == -1) {
        PRINT_ERROR
    } else {
        UPDATE_WD
    }

}

/**
 *  remove first quotation mark
 * @param path
 */
void removefirstQuots(char *path) {
    char temp[strlen(path)];
    bool flag = false;
    int i = 0;
    int c = 0;
    for (i = 0; i < strlen(path); ++i) {
        if (!flag && path[i] == '"') {
            flag = true;
            continue;
        } else {
            temp[c] = path[i];
            c++;
        }
    }
    temp[c] = '\0';
    stpcpy(path, temp);

}

/**
 * remove last quotation mark
 * @param path
 */
void removeLastQuots(char *path) {
    if (path[strlen(path) - 1] == '"') {
        path[strlen(path) - 1] = 0;
    }

}

/**
 * fix the path to the right format, no  quotation marks
 *  connect tokens to a legit path
 * @param commandInfo
 */
void pathOrganizer(CommandInfo *commandInfo) {
    if (commandInfo->lastArgIndex < 1) {
        //no path
        return;
    }
    removefirstQuots(commandInfo->argv[1]);
    int i = 2;
    while (commandInfo->lastArgIndex >= i) {

        strcat(commandInfo->argv[1], " "); //add space
        strcat(commandInfo->argv[1], commandInfo->argv[i]); //connect tokens
        i++;

    }
    removeLastQuots(commandInfo->argv[1]);
    commandInfo->lastArgIndex = 1;


}


/**
 * CD act
 * @param commandInfo
 */
void actCD(CommandInfo *commandInfo, char *previousPwd) {
    pid_t currPID = getpid();
    printf("%d\n", currPID);
    pathOrganizer(commandInfo);
    char currentPwd[MAX_COMMAND];
    if (getcwd(currentPwd, MAX_COMMAND) == NULL) {
        PRINT_ERROR
    }
    //  global path: cd/cd~
    if (equal(commandInfo->argv[commandInfo->lastArgIndex], CD) || equal(commandInfo->argv[1], "~")) {
        //try set working directory to HOME
        chdirTry(getenv(HOME), previousPwd, currentPwd);
    } else if (equal(commandInfo->argv[1], "-")) {
        if (!equal(previousPwd, NOT_SET)) {
            // go to previous folder
            if (chdir(previousPwd) == -1) {
                PRINT_ERROR
            } else {
                printf("%s\n", previousPwd);
                UPDATE_WD
            }
        } else {
            //OLDWD not set
            fprintf(stderr, "cd: OLDWD not set\n");
        }
    } else {
        chdirTry(commandInfo->argv[1], previousPwd, currentPwd);
    }

}


/**
 * remove all dead processes from the array
 * @param jobsArr
 * @param processCounter
 * @return number of living processes
 */
int updateJobArr(Process jobsArr[], int processCounter) {
    Process livingProcessesArr[processCounter];
    int livingProcesses = 0;
    int i = 0;
    for (i = 0; i < processCounter; i++) {
        if (waitpid(jobsArr[i].pid, NULL, WNOHANG) == 0) {
            // pid alive, save it to the temp array
            livingProcessesArr[livingProcesses] = jobsArr[i];
            livingProcesses++;
        }
    }
    // temp process arr now holds only active processes-copy back to jobs arr
    for (i = 0; i < livingProcesses; i++) {
        jobsArr[i] = livingProcessesArr[i];
    }
    return livingProcesses;


}

/**
 * update the jobs array and prints all living processes
 * @param jobs_arr
 * @param processCounter
 */
int actJOBS(Process jobs_arr[], int processCounter) {
    processCounter = updateJobArr(jobs_arr, processCounter);
    int i;
    // prints active processes
    for (i = 0; i < processCounter; i++) {
        printf("%ld ", (long) jobs_arr[i].pid);
        // clear the "&" if exists
        char *cmd;
        cmd = strtok(jobs_arr[i].cmd, "&");
        printf("%s\n", cmd);
    }
    return processCounter;

}


/**
 * EXIT act, kill all the background processes and exit
 * @param jobsArr
 * @param numOfJobs
 */
void actEXIT(Process jobsArr[], int numOfJobs) {
    pid_t currPID = getpid();
    printf("%d\n", currPID);
    int k = 0;
    for (k = 0; k < numOfJobs; k++) {
        kill(jobsArr[k].pid, SIGTERM);
    }
}

/**
 * child process
 * @param commandInfo
 */
void actChild(CommandInfo *commandInfo) {
    if (equal(commandInfo->command, MAN)) {
        //man requires special treatment
        commandInfo->command = "man";
        execvp("man", commandInfo->argv);
    } else {
        execv(commandInfo->command, commandInfo->argv);
    }
    //not supposed to happened
    PRINT_ERROR
}

/**
 * parent shell process
 * @param commandInfo
 * @param jobsArr
 * @param counter
 * @param pid
 * @return the counter updated
 */
int actParent(CommandInfo *commandInfo, Process jobsArr[], int counter, pid_t pid) {
    printf("%d\n", pid); //childes pid
    if (commandInfo->backgroundFlag) {
        // the background flag is on
        Process currProcess;
        currProcess.pid = pid;
        strcpy(currProcess.cmd, commandInfo->cmd);
        jobsArr[counter] = currProcess;
        counter++;
    } else {
        // foreground process, wait for child
        while (waitpid(pid, NULL, WNOHANG) == 0) {
        }
    }
    return counter;

}


int main() {

    // stores background processes
    Process jobsArr[MAX_BACKGROUND_PROCESSES];
    char previousPwd[MAX_COMMAND] = NOT_SET;
    int processNum = 0;
    while (1) {

        /*************intake*********/
        PRINT_PROMPT_SIGN
        char cmdLine[MAX_COMMAND];
        intake(cmdLine);
        CommandInfo commandInfo;
        setArgs(cmdLine, &commandInfo);


        /**********actions*************/
        if (equal(commandInfo.command, EXIT)) {
            actEXIT(jobsArr, processNum);
            exit(0);
        } else if (equal(commandInfo.command, CD)) {
            actCD(&commandInfo, previousPwd);
        } else if (equal(commandInfo.command, JOBS)) {
            processNum = actJOBS(jobsArr, processNum);
        }

            /*************standard command*********/
        else {
            pid_t pid = fork();
            // create process for this command
            switch (pid) {
                case -1: {
                    perror("fork error");
                }
                    break;
                case 0: {
                    actChild(&commandInfo);
                }
                    break;
                default: {
                    processNum = actParent(&commandInfo, jobsArr, processNum, pid);
                }
            }

        }
    }
}