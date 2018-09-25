/*PIPE FUNKAR?*/
/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive:
      $> tar cvf lab1.tgz lab1/
 *
 * All the best
 */


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/*
 * Function declarations
 */
void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void sighandler(int);
void executeCommand(Command *cmd);
void executePipe(Command *cmd);

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void) {
    Command cmd;
    int n;
    signal(SIGINT, sighandler);

    while (!done) {
        char *line;
        line = readline("> ");

        if (!line) {
            /* Encountered EOF at top level */
            done = 1;

        } else
        {
            /*
             * Remove leading and trailing whitespace from the line
             * Then, if there is anything left, add it to the history list
             * and execute it.
             */
            stripwhite(line);

            if(*line) {
                add_history(line);
                /* execute it */
                n = parse(line, &cmd);
                //PrintCommand(n, &cmd);

                //cd and exit are built in functions
                if(strcmp(cmd.pgm->pgmlist[0], "exit") == 0) {
                    exit(0);
                } else if (strcmp(cmd.pgm->pgmlist[0], "cd") == 0) {
                    if(chdir(cmd.pgm->pgmlist[1]) == -1) {
                        printf("Unsuccessful completion of cd");
                        errno = 2;
                    }
                      chdir(cmd.pgm->pgmlist[1]);
                } else {
                    executeCommand(&cmd);
                }
          }
        }
        if(line) {
            free(line);
        }
        //https://www.geeksforgeeks.org/zombie-processes-prevention/
        waitpid(1, NULL, 0);
    }
    return 0;
}

/*
 * Name: sighandler
 *
 * Description: handles a signal
 * https://www.tutorialspoint.com/c_standard_library/c_function_signal.htm
 */
void sighandler(int signum) {
    printf("Caught signal %d, coming out...\n", signum);
    exit(1);
}

/*
 * Name: executeCommand
 *
 * Description: executes simple commands
 *
 */
void executeCommand(Command *cmd) {
  // executes simple commands
    Pgm *p;
    p = cmd->pgm;
    int pid;

    //fork
    pid = fork();
    if(pid == -1) {printf("No child process created\n"); exit(1);}
    else if(pid == 0) { //child process

        //redirect stdout
      if(cmd->rstdout) {
        //create file, give rwx permission
        int fd = open(cmd->rstdout, O_CREAT,S_IRWXU);
        dup2(fd, 1);
      }
      //redirect stdin
      if(cmd->rstdin) {
        int fd = open(cmd->rstdin, 0);
        dup2(fd, 0);
      }

      if(p->next) {
        // if it has pipes, execute pipe
        executePipe(cmd);
      } else {

        if(cmd->bakground != 0) {
          //if there is a background process, ignore interrupts
          signal(SIGINT, SIG_IGN);
        }

        //execute!
        execvp(p->pgmlist[0], p->pgmlist);

      }

    } else {
      // wait for the child to complete and reap the exit status of the child
        int status;
        waitpid(pid, &status, 0);

        //https://www.geeksforgeeks.org/zombie-processes-prevention/
        signal(SIGCHLD,SIG_IGN); //prevent zombie process
    }

}

void executePipe(Command *cmd) {
    Pgm *p;
    p = cmd->pgm;
    int pipefd[2]; //pipe file descriptors
    int pid;

    //creates a new pipe
    if(pipe(pipefd) == -1) {
        //https://www.tldp.org/LDP/lpg/node11.html
        perror("fork");
        exit(1);
    }

    //fork off the child process
    //parent & child have their own pair of r/w fd to the same pipe object
    pid = fork();

    if(pid == -1) {
        printf("No child process created\n");
        exit(1);
    } else if(pid == 0) { //child process

      //http://tldp.org/LDP/lpg/node11.html
      close(pipefd[0]); //child process closes input side of pipe
      dup2(pipefd[1], 1); //send through output end of pipe, close old fd

      //redirect stdin
      if(cmd->rstdin) {
        int fd = open(cmd->rstdin, 0);
        dup2(fd, 0);
      }

      p = p->next;
      if(p->next) {
        //if more pipes, use recursion
        executePipe(cmd);
      } else {

          if(cmd->bakground != 0) {
              //if there is a background process, ignore interrupts
              signal(SIGINT, SIG_IGN);
          }
          execvp(p->pgmlist[0], p->pgmlist);
      }

    }
    else { //parent process
        close(pipefd[1]); //parent closes output side of pipe
        dup2(pipefd[0], 0); //read from pipe, close old fd

        if(cmd->bakground != 0) {
            signal(SIGINT, SIG_IGN);
        }

        execvp(p->pgmlist[0], p->pgmlist);
        signal(SIGCHLD,SIG_IGN); //prevent zombie process
    }
}


/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void PrintCommand (int n, Command *cmd) {
    printf("Parse returned %d:\n", n);
    printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
    printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
    printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
    PrintPgm(cmd->pgm);

}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void PrintPgm (Pgm *p) {
    if (p == NULL) {
        return;
    } else
    {
        char **pl = p->pgmlist;
        /* The list is in reversed order so print
         * it reversed to get right
         */
        PrintPgm(p->next);
        printf("    [");

        while (*pl) {
            printf("%s ", *pl++);

        }
        printf("]\n");
    }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void stripwhite (char *string) {
      register int i = 0;

      while (isspace( string[i] )) {
          i++;
      }

      if (i) {
          strcpy (string, string + i);
      }

      i = strlen( string ) - 1;
      while (i> 0 && isspace (string[i])) {
          i--;
      }

      string [++i] = '\0';
}
