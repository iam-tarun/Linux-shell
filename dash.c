// necessary inputs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// global variables and struct array initialization
struct array {  
  char** pieces;
  int len;      
};
char* path = NULL;

// function declarations
void interactiveMode();
void batchMode(char* fileName);
void executeInput(char* input, bool isParallel);
void execute(char* input, bool isParallel);
void executeCmd(char* str, bool isParallel);
void execChildAndWait(char* dir, char** args);
void execChildAndNotWait(char* dir, char** args);

// helper functions
void triggerError();
void removeNewlines(char* str);
void removeLeadingTrailingSpaces(char* str);
struct array* breakString(char* str, char* delimiter);
void overwritePath(struct array* outArray);

int main(int argc, char* argv[]) {
  path = (char*)malloc(5);
  // initial path value
  path = "/bin:";
  if(argc == 1) {
    // interactive mode
    interactiveMode();
  }
  else if (argc == 2) {
    // batch mode
    batchMode(argv[1]);
  }
  else {
      // invalid invoke of dash
      triggerError(); 
  }
  return 0;
}

void interactiveMode() {
  char* line = NULL;
  size_t len;
  bool isParallel = false;
  while ( true ) {

    printf("dash> ");
    // read the input
    getline(&line, &len, stdin);
    // format the input
    char* input = strdup(line);
    removeNewlines(input);
    removeLeadingTrailingSpaces(input);

    // check for parallel command symbol
    if(strchr(input, '&') != NULL) {
      // parallel command
      isParallel = true;

      // break the input line into pieces, with each piece as an individual command
      struct array* outArray = breakString(input, "&");

      // if the parallel input has zero commands
      if(outArray->len == 0) {
        triggerError();
      }
      else {
        int i = 0;
        // execute each command parallelly 
        while ( i < outArray->len) {
          char* cmd = strdup(outArray->pieces[i]);
          execute(cmd, isParallel);
          i++;
        }
        i = 0;
        // after running them wait for all the child processes to complete
        while ( i < outArray->len) {
          int status;
          wait(&status);
          i++;
        }
      }
    }
    else {
      //if it is not a parallel command, execute it directly
      execute(input, isParallel);
    }
  }
}

void batchMode(char* fileName) {
  FILE* file = fopen(fileName, "r");
  bool isParallel = false;
  // if there is no file with given name
  if(file == NULL) {
    triggerError();
  }
  else {
    char* line = NULL;
    size_t len = 0;
    while(getline(&line, &len, file) != -1 ) {
      // read the input
      char* input = strdup(line);
      // format the input
      removeNewlines(input);
      removeLeadingTrailingSpaces(input);
      // if the line is empty, move to the next line
      if(!strlen(input)){
          continue;
      }
      if(strchr(input, '&') != NULL) {
        // parallel command
        isParallel = true;

        // break the input line into pieces, with each piece as an individual command
        struct array* outArray = breakString(input, "&");
        
        // if there are no commands in the input
        if(outArray->len == 0) {
          triggerError();
        }
        else {
          int i = 0;

          // execute each command parallelly 
          while ( i < outArray->len) {
            char* cmd = strdup(outArray->pieces[i]);
            execute(cmd, isParallel);
            i++;
          }
          i = 0;

          // after running them wait for all the child processes to complete
          while ( i < outArray->len) {
            int status;
            wait(&status);
            i++;
          }
        }
      }
      else {
        //if it is not a parallel command, execute it directly
        execute(input, isParallel);
      }
    }
  }
}

// function to execute an individual command
void execute(char* input, bool isParallel) {
  // check if the given command is a redirection command
  if(strchr(input, '>') != NULL) {
    // redirection command
    struct array* outArray = breakString(input, ">");
    // if there is no filename or command
    if(outArray->len != 2) {
      triggerError();
    }
    else {
      // fetch the filename
      char* filename = strdup(outArray->pieces[1]);
      // break the filename into pieces with space as delimiter
      struct array* fileArray = breakString(filename, " ");
      // if there are more than one file name, throw error
      if(fileArray->len > 1) {
        triggerError();
      }
      else {
        // reroute the stdout the given file
        FILE* file = freopen(filename, "w+", stdout);
        char* redirectInput = strdup(outArray->pieces[0]);
        // execute the command
        executeCmd(redirectInput, isParallel);
        fclose(file);
        // reroute the stdout to standard shell
        freopen("/dev/tty", "w+", stdout);
      }
    }
  }
  else {
    // if it is not a redirection command, execute it as a normal command
    executeCmd(input, isParallel);
  }
} 


// function to execute normal commands
void executeCmd(char* str, bool isParallel) {

  // break the command into pieces with space as delimiter
  struct array* outArray = breakString(str, " ");
  // if there is no command, throw error
  if(outArray->len == 0) {
    triggerError();
  }
  // check if the given command is an in-built command
  if(strcmp(outArray->pieces[0],"cd") == 0) {
    // change directory command
    if(outArray->len == 2){
      int res = chdir(outArray->pieces[1]);
      if(res == -1) {
        triggerError();
      }
    }
    else {
      triggerError();
    }
  }
  else if(strcmp(outArray->pieces[0], "path") == 0) {
    // path command
    overwritePath(outArray);
  }
  else if(strcmp(outArray->pieces[0], "exit") == 0) {
    // exit command
    if(outArray->len > 1) {
      triggerError();
    }
    else {
      exit(0);
    }
  }
  else {
    // normal command
    // need to search in the path dir
    char* dir = outArray->pieces[0];
    if(access(dir, X_OK) != 0) {
      // if the given command is not executable directly
      struct array* pathArray = breakString(path, ":");
      int i = 0;
      while ( i < pathArray->len) {
        dir = strdup(pathArray->pieces[i]);
        strcat(dir, "/");
        strcat(dir, outArray->pieces[0]);
        if(access(dir, X_OK) == 0) {
          break;
        }
        else {
          i++;
        }
      }
    }
    if(access(dir, X_OK) == 0) {
      char** args = (char**)malloc((outArray->len+1)*sizeof(char*));
      args = outArray->pieces;
      args[outArray->len] = NULL;
      // if the given input is a parallel command, just start the child process as we have to start all of them at a time, so waiting will come at a later time
      if(isParallel) {
        execChildAndNotWait(dir, args);
      }
      else {
        // if it is a normal command, start the child process and wait for it here itself
        execChildAndWait(dir, args);
      }
    }
    else {
      triggerError();
    }
  }
}

// function to create a child process and wait for it
void execChildAndWait(char* dir, char** args) {
  int pid = fork();
  if(pid == 0) {
    execv(dir, args);
  }
  else {
    int status;
      wait(&status);
  }
}

// function to creating a child process
void execChildAndNotWait(char* dir, char** args) {
  int pid = fork();
  if(pid == 0) {
    execv(dir, args);
  }
}

// in built function to overwrite the path
void overwritePath(struct array* outArray) {
  path = NULL;
  int totalLength = 0;
  int i = 1;
  // calculate the length of new path
  while(i < outArray->len) {
    totalLength += strlen(outArray->pieces[i]) + 1;
    i++;
  }
  // allocate the memory for the required length
  path = (char*)realloc(path, totalLength);
  i = 1;
  // overwrite the path
  while(i < outArray->len) {
    if(path == NULL) {
      path = strdup(outArray->pieces[i]);
    }
    else {
      strcat(path, outArray->pieces[i]);
    }
    strcat(path, ":");
    i++;
  }
}

// standard error message
void triggerError() {
   char error_message[30] = "An error has occurred\n";
   write(STDERR_FILENO, error_message, strlen(error_message));
}

// function to remove ending new line in a string
void removeNewlines(char* str) {
    size_t len = strlen(str);
    if(str[len-1] == '\n'){
         str[len-1] = '\0';
    }
}

// function to remove leading and trailing spaces in a string
void removeLeadingTrailingSpaces(char* str) {
    int len = strlen(str);
    // Remove leading spaces
    int start = 0;
    while (isspace(str[start])) {
        start++;
    }
    // if the line contains only spaces
    if(start == len){
      str[0] = '\0';
	    return;
    }
    // Remove trailing spaces
    int end = len - 1;
    while (end >= 0 && isspace(str[end])) {
        end--;
    }
    // Shift the non-space characters to the beginning of the string
    int i = start;
    for (; i <= end; i++) {
        str[i - start] = str[i];
    }
    // Null-terminate the modified string
    str[end - start + 1] = '\0';
    
}

// function to break any given string to break it into pieces using strtok and returning a struct array
struct array* breakString(char* str, char* delimiter) {
  char** result = NULL;
  char* temp = strdup(str);
  // format the input string
  removeLeadingTrailingSpaces(temp);
  removeNewlines(temp);
  int nOfPieces = 0;
  char* token = strtok(temp, delimiter);
  // count the number of pieces possible for the given string
  while(token != NULL) {
    nOfPieces++;
    token = strtok(NULL, delimiter);
  }
  // allocate the memory based on the number of pieces
  result = (char**)malloc(nOfPieces*(sizeof(char*)));
  temp = strdup(str);
  token = strtok(temp, delimiter);
  int i = 0;
  // store the pieces in the 2D array
  while(token != NULL) {
    removeNewlines(token);
    removeLeadingTrailingSpaces(token);
    result[i] = strdup(token);
    i++;
    token = strtok(NULL, delimiter);
  }
  // create a struct array and store the 2d array and number of pieces in it
  struct array* out = (struct array*)malloc(sizeof(struct array));
  out->pieces = result;
  out->len = nOfPieces;
  return out;
}