/* Assignment 1: sh360 program
 * Zenara Daley V00820899
 * CSC 360, Summer 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

const int MAX_ARGS = 8; // 1 command + 7 args
const int MAX_INPUT = 82; // 80 user inputed chars + stdin '\n' + fgets '\0'

void execve_on_paths(char *commands[3][9], char *paths[], int c) {
  char *envp[] = { 0 };
  char * lcommand = commands[c][0];
  int i;
  // max length of pathname (ref: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx)
  char line[260]; 

  //attempt to run command without paths
  execve(lcommand, commands[c], envp);
  // attempt to run execve on all paths
  for(i = 0; paths[i] != NULL; i++){
    line[0] = '\0'; 
    strcpy(line, paths[i]);
    strcat(line, "/");
    strcat(line, lcommand);
    commands[c][0] = line;
    
    execve(commands[c][0], commands[c], envp);
  }
}

void or_handler(char *commands[3][9], char *paths[]){
  // copy linux command
  char lcommand[20];
  strcpy(lcommand, commands[0][0]);

  // set variables used in fork
  char *envp[] = { 0 };
  char *filename = commands[1][0];
  int pid, fd;
  int status; 

  if ((pid = fork()) == 0) {
      fd = open(filename, O_CREAT|O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
      if (fd == -1) {
          fprintf(stderr, "Cannot open %s for writing\n", filename);
          exit(1);
      }
      dup2(fd, 1);
      dup2(fd, 2); 

      execve_on_paths(commands, paths, 0);
      exit(12);
  } 
  waitpid(pid, &status, 0);
  
  // check if child exited (aka execve failure)
  if (WIFEXITED(status) && WEXITSTATUS(status) == 12)
    printf("Location of '%s' was not found on paths in .sh360\n", lcommand);
  else 
    printf("Command output successfully written to '%s'\n", filename);
}

void pp_handler(char *commands[3][9], char *paths[], int arrow_count){

    // copy linux command
    char command1[20];
    char command2[20];
    char command3[20];
    strcpy(command1, commands[0][0]);
    strcpy(command2, commands[1][0]);
    strcpy(command3, commands[2][0]);

    // char *envp[] = { 0 };
    int status;
    int pid_c1, pid_c2, pid_c3;
    
    int fd1[2];
    int fd2[2];
    pipe(fd1);
    pipe(fd2);

    // first command
    if ((pid_c1 = fork()) == 0) {
        // pipe fd 1 to STDOUT
        dup2(fd1[1], 1);
        close(fd1[0]);

        execve_on_paths(commands, paths, 0);
        exit(10);
    }

    // second command
    if ((pid_c2 = fork()) == 0) {
        // pipe fd1 to STDIN       
        dup2(fd1[0], 0);
        close(fd1[1]);

        if(arrow_count > 1){
          // pipe fd2 to STDOUT               
          dup2(fd2[1], 1);
          close(fd2[0]);
        }

        execve_on_paths(commands, paths, 1);
        exit(11);
    }

    // third command (optional)
    if (arrow_count == 2){
      if ((pid_c3 = fork()) == 0) {
        // close fd1 STDIN
        close(fd1[1]);  
        
        // pipe fd2 to STDIN
        dup2(fd2[0], 0);
        close(fd2[1]);

        execve_on_paths(commands, paths, 2);
        exit(12);
      }
    }
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);

     // wait and check status of child 1
    waitpid(pid_c1, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 10)
      printf("Location of '%s' was not found on paths in .sh360\n", command1);

    // wait and check status of child 2
    waitpid(pid_c2, &status, 0); 
    if (WIFEXITED(status) && WEXITSTATUS(status) == 11)
      printf("Location of '%s' was not found on paths in .sh360\n", command2);  

    // wait and check status of child 3
    waitpid(pid_c3, &status, 0); 
    if (WIFEXITED(status) && WEXITSTATUS(status) == 12)
      printf("Location of '%s' was not found on paths in .sh360\n", command3); 
}

// Ref: https://stackoverflow.com/questions/7898215/how-to-clear-input-buffer-in-c
void flush_extra_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Ref: https://stackoverflow.com/questions/3981510/getline-check-if-line-is-whitespace
int is_empty(char *s) {
  while (*s != '\0') {
    if (!isspace((unsigned char)*s))
      return 0;
    s++;
  }
  return 1;
}

int main(void) {
  FILE * fp;
  char * prompt = NULL;
  size_t len = 0;
  ssize_t nread;

  // Open config file
  fp = fopen(".sh360", "r");
  if (fp == NULL)
    exit(EXIT_FAILURE);

  // Read prompt and check that it is less than 10 chars
  nread = getline(&prompt, &len, fp);
  if(nread == -1 || nread > 10)
    exit(EXIT_FAILURE);
  prompt[nread - 1] = ' ';

  // get all paths
  char *paths[10] = { NULL };
  int i;
  for(i = 0; i < 10 && (nread = getline(&paths[i], &len, fp)) != -1 ; i++)
    paths[i][nread - 1] = 0;
  
  fclose(fp);

  while(1) {
    // prompt user
    fprintf(stdout, "%s", prompt);
    fflush(stdout);

    //get user input
    char input[MAX_INPUT];
    fgets(input, MAX_INPUT, stdin);

    // validate input
    if(strchr(input, '\n')) {
      input[strlen(input) - 1] = '\0';
    } else{
      printf("Error: Maximum input is 80 characters\n");
      flush_extra_input();
      continue;
    }
    if(is_empty(input))
      continue;

    //DEBUG
    // printf("%s\n", input);
    
    // validate command type
    char *type;
    type = strtok(input, " ");
    if(strcmp(type, "exit") != 0 && strcmp(type, "PP") != 0 && strcmp(type, "OR") != 0) {
      printf("Error: Input must begin with 'PP', 'OR', or 'exit'\n");
      continue;
    }
    
    // map commands
    char *commands[3][9] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    char *temp;
    int arrow_count = 0;
    int i, j;
    for(i = 0; i < 3; i++){
      for(j= 0; j < (MAX_ARGS + 1); j++){
        temp = strtok(NULL, " ");
        if(temp == NULL) {
          break;
        }
        if(strcmp(temp, "->") == 0){
          arrow_count++;
          commands[i][j] = 0;
          break;
        }
        commands[i][j] = temp;
      }
    }

    if(strcmp(type, "exit") == 0 && commands[0][0] == NULL)
      break;
    
    //check for missing arrow
    if(arrow_count == 0) {
      printf("Missing '->' symbol\n");
      continue;
    }
    //check for too many arrows (OR case)
    if(arrow_count == 2 && strcmp(type, "OR") == 0) {
      printf("OR can only have 1 '->' symbol\n");
      continue;
    }
    // check for too many arguments (OR case)
    if(commands[1][1] != NULL && strcmp(type, "OR") == 0) {
      printf("OR can only output to 1 file\n");
      continue;
    }
    
    //check for general format errors
    bool error = false;
    char error_message[50] = {0};
    for(i = 0; i < 3; i++){
      // check for too many arguments (general case)
      if(commands[i][MAX_ARGS] != NULL) {
        error = true;
        sprintf(error_message, "Too many arguments in command %d", (i+1));
        break;
      }
      // check for not enough arguments
      if((commands[i][0] == NULL || is_empty(commands[i][0])) && i <= arrow_count) {
        error = true;
        sprintf(error_message, "Not enough arguments in command %d", (i+1));
        break;
      }
    }
    if(error) {
      printf("%s\n", error_message);
      continue;
    }

    if(strcmp(type, "OR") == 0) {
      or_handler(commands, paths);
      continue;
    }
    pp_handler(commands, paths, arrow_count);
  }  
  return 0;
}                                                                                                                                                                                                                                           
