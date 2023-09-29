#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "defs.h"
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char* path; // global path variable, "/bin" is the initial value

// struct defines
CMD_NODE_STRUCT(char);
CMD_TOKENS_STRUCT(char);


// function declarations
void interactiveMode();
void executeCmd(char* line);
bool validateRedirectionCmd(char *line);
int countChar(char* line, char target);
struct Tokens_char tokenizeString(char* str, char delimiter[],struct Tokens_char* result);
void removeNewlines(char* str);
void triggerError();
int execChildCmd(struct Tokens_char* cmdTokens);
void removeLeadingTrailingSpaces(char* str);
void overwritePath(struct Node_char* paths, int len);
char** formatArguments(struct Node_char* args, char* dir);
void executeRedirectCmd(struct Tokens_char* tokens);
void batchMode(char* fileName);

int main (int argc, char* argv[]) {
   // assigning initial path to the path variable
   char initialPath[] = "/bin;";
   path = (char *)malloc(5);
   path = initialPath;

   // invoke the respective methods based on the count of arguments
   if (argc > 2) { 
      // call the exit process with state as 1
      triggerError();
   }
   else if (argc == 2) {
      // call the batch mode
      batchMode(argv[1]);
   }
   else {
      // call the interactive mode
      interactiveMode();
   }
   return 0;

}

// function to execute interactive mode
void interactiveMode() {
   char *line = NULL;
   size_t len;
   while (true) {
      printf("dash> ");
      getline(&line, &len, stdin); // read the input
      // check if the input is parallel inputs or redirection
      if ( strchr(line, PARALLEL_DELIMITER) != NULL ) {
         // found a '&' symbol in the input, so it is a parallel input
         struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
         char delimiter[] = "&";
         tokenizeString(line, delimiter, tokens);
         int pid = 0;
	      int i = 0;
         // run a child process for each of the sub command
         for(; i < tokens->len; i++) {
            pid = fork();
            if (pid == 0) {
               executeCmd(tokens->token->data);
               exit(0);
            }
            tokens->token = tokens->token->next;
         }
	      i = 0;
         // wait until all the child processes are executed
         for (; i < tokens->len; i++) {
            int status;
            wait(&status);
         }
         free(tokens);
      }
      else {
         // if it is not a parallel command
         executeCmd(line);
      }
   }
}

// function to execute any given command
void executeCmd(char* line) {
   // check if it is a redirection command
   char* input = strdup(line);
   if ( strchr(line, REDIRECTION_DELIMITER) != NULL ) {
      // printf("%sit is a redirection command\n", line);
      if ( !validateRedirectionCmd(line) ) {
         triggerError();
      }
      else {
         //redirection command process
         struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
         char redirection_delimiter[] = ">";
         tokenizeString(input, redirection_delimiter, tokens);
         executeRedirectCmd(tokens);
         free(tokens);
      }
   }
   else {
      struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      char space_delimiter[] = " ";
      tokenizeString(input, space_delimiter, tokens);
      removeNewlines(tokens->token->data);
      // check if it belongs to any of the built in command;
      if (strcmp(tokens->token->data, "path") == 0) {
         struct Node_char* paths= tokens->token->next;
         overwritePath(paths, tokens->len);
      }
      else if ( strcmp( tokens->token->data, "cd" ) == 0 ) {
         if(tokens->len == 2) {
            removeNewlines(tokens->token->next->data);
            int out = chdir(tokens->token->next->data);
            if ( out == -1 ) {
               triggerError();
            }
         }
         else {
            triggerError();
         }
      }
      else if ( strcmp( tokens->token->data, EXIT ) == 0 ) {
         exit(0);
      }
      // if it is not a built in command, try to execute it directly using a child process
      else {
         execChildCmd(tokens);
      }

      free(tokens);
   }
}

// function to execute a child process using fork, access and execv
int execChildCmd(struct Tokens_char* cmdTokens) {
   char* dir = NULL;
   dir = (char *)malloc(strlen(cmdTokens->token->data) + 1);
   dir = cmdTokens->token->data;
   // check the access for given cmd path
   if(access(dir, X_OK) != 0) {
      // if it not works
      // check all the paths
      char* fDir = (char *)malloc(strlen(dir) + 1);
      strcpy(fDir, dir);
      struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      char delimiters[] = ";";
      char* pathTemp = strdup(path);
      // divide the path variable
      tokenizeString(pathTemp, delimiters, tokens);
      struct Node_char* paths = (struct Node_char*)malloc(sizeof(struct Node_char));
      paths = tokens->token;
      int i = 0;
      // traverse through each path
      for (; i < tokens->len; i++ ){
	   removeNewlines(paths->data);
         int totalLength = strlen(fDir);
         totalLength += strlen(paths->data) + 2;
         dir = NULL;
         dir = (char *)realloc(dir, totalLength);
         dir = strcat(paths->data, "/");
         strcat(dir, fDir);
         // check for access
         if (access(dir, X_OK) == 0) {
            // if we found the dir
            break;
         }
         paths = paths->next;
      } 

   }
   if(access(dir, X_OK) == 0) {
      // if we found a dir that has the executable file
      char** argsArray = NULL;
      argsArray = formatArguments(cmdTokens->token->next, dir);
      // execute the command in a child process
      int pid = fork();
      if (pid == 0) {
         execv(dir, argsArray);
      }
      else {
         // parent process waits till the child process finishes
         int status;
         wait(&status);
      }
   }
   else {
      triggerError();
   }
   return 0;
}

// function to redirect the output to a file
void executeRedirectCmd(struct Tokens_char* tokens) {
   char* fileName = tokens->token->next->data;
   // removing white spaces and newlines in the file name
   removeNewlines(fileName);
   removeLeadingTrailingSpaces(fileName);
   // rerouting the output to given fileName  
   FILE* file = freopen(fileName,"w+", stdout);
   struct Tokens_char* subTokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   char delimiter[] = " ";
   removeNewlines(tokens->token->data);
   tokenizeString(tokens->token->data, delimiter, subTokens);
   execChildCmd(subTokens);
   fclose(file);
   // rerouting the stdout to shell
   freopen("/dev/tty", "w+", stdout);
   free(subTokens);
}

// function to execute commands in batch mode
void batchMode(char* fileName) {
   FILE* file = fopen(fileName, "r");
   if(file == NULL) {
      triggerError();
   }
   else {
      char* line = NULL;
      size_t len = 0;
      struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      // read the file line by line
      while(getline(&line, &len, file) != -1 ) {
         if ( strchr(line, PARALLEL_DELIMITER) != NULL ) {
         // found a '&' symbol in the input, so it is a parallel input
         char delimiter[] = "&";
         tokenizeString(line, delimiter, tokens);
         int pid = 0;
	 int i = 0;
         for(; i < tokens->len; i++) {
            pid = fork();
            if (pid == 0) {
               executeCmd(tokens->token->data);
               exit(0);
            }
            tokens->token = tokens->token->next;
         }
	 i = 0;
         for (; i < tokens->len; i++) {
            int status;
            wait(&status);
         }
         free(tokens);
      }
      else {
         // if it is not a parallel command
         executeCmd(line);
      }
      }
   } 
}

// builtin function to overwrite the path
void overwritePath(struct Node_char* paths, int len) {
   path = NULL;
   while(paths != NULL && paths->data != NULL) {
     int length = ( path == NULL ) ? 0 : strlen(path);
     removeNewlines(paths->data);
     path = (char *)realloc(path, len + strlen(paths->data) + 2);
     if(length > 0) {
        strcat(path, ";");
        strcat(path, paths->data);
     }
     else {
        path = strdup(paths->data);
     }
     paths = paths->next;
   }
}

// helper functions

// function to count the number of times a character has occurred in a string
int countChar(char* line, char target) {
   char* temp = line;
   int count = 0;
   while (*temp) {
      if(*temp == target) {
         count++;
      }
      temp++;
   }
   return count;
}

// function to validate a redirection command
bool validateRedirectionCmd(char *line) {
   // check the number of occurrences of '>' symbol
   char delimiters[] = ">";
   if(countChar(line, REDIRECTION_DELIMITER) > 1) {
      return false;
   }
   // divide the string with > as delimiter
   struct Tokens_char *result = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   tokenizeString(line, delimiters, result);
   // storing the right side string of ">"
   char* filepath = result->token->next->data;
   struct Tokens_char *filepathResult = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   char space_delimiter[] = " ";
   tokenizeString(filepath, space_delimiter, filepathResult);
   // check for number of strings divided with " " as delimiter for the second block of the command i.e towards the right side of ">"
   if(filepathResult->len != 1) {
      return false;
   }
   free(result);
   free(filepathResult);
   return true;
}

// common function to tokenize a string w.r.t the delimiters given
struct Tokens_char tokenizeString(char* str, char delimiter[],struct Tokens_char* result) {
   // storing the tokens in a linked list data structure
   char* input = (char*)malloc(sizeof(str));
   strcpy(input, str);
   // store the initial input in cmd of struct tokens_char
   result->cmd = str;
   // please look into the defs.h file for structure of the Tokens_char and Node_char
   // Node_char is linked_list with data being of char type
   // Tokens_char stores a linked list with all the string divided by the given delimiter, length of total strings that we get as len and the initial input as cmd
   struct Node_char* temp = (struct Node_char*)malloc(sizeof(struct Node_char));
   result->token = temp;
   temp->data = strtok(str, delimiter);
   // build the linked list
   while(temp->data != NULL) {
      struct Node_char* var = (struct Node_char*)malloc(sizeof(struct Node_char));
      var->data = strtok(NULL, delimiter);
      temp->next = var;
      temp = var;
      // increment the len for every string divided using the delimiter
      result->len++;
   }
   // freeing the temporary vars
   free(input);
   free(temp);
   return *result;
}

// function to remove ending new line in a string
void removeNewlines(char* str) {
    size_t len = strlen(str);
    if(str[len-1] == '\n'){
         str[len-1] = '\0';
    }
}

// standard error message
void triggerError() {
   char error_message[30] = "An error has occurred\n";
   write(STDERR_FILENO, error_message, strlen(error_message));
}

// function to format the arguments
// returns a string array with cmd name as first value and NULL as last value for using in execv function
char** formatArguments(struct Node_char* args, char* dir) {
   char* dirCopy = strdup(dir);
   char delimiters[] = "/";
   struct Tokens_char* subtokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   // dividing the given string with "/" as delimiter
   tokenizeString(dirCopy, delimiters, subtokens);
   struct Node_char* token_node = (struct Node_char*)malloc(sizeof(struct Node_char));
   token_node = subtokens->token;
   int p = 0;
   // traversing the linked list to find the cmd name
   for(; p < subtokens->len - 1; p++) {
      token_node = token_node->next;
   }
   int nOfArgs = 2;
   struct Node_char* current = args;
   // counting the number of args given other than the cmd name and NULL
   while ((current->data != NULL) && (current->next != NULL) ) {
      nOfArgs++;
      current = current->next;
   }
   // allocating the memory based on the number of args
   char **argsArray = (char **)malloc(nOfArgs * sizeof(char *));
   current = args;
   // storing th cmd name as first value
   argsArray[0] = token_node->data;
   int i = 1;
   // storing the array
   for (; i < nOfArgs-1; i++ ) {
      removeLeadingTrailingSpaces(current->data);
      if ( current->data != NULL ) {
         argsArray[i] = strdup(current->data);
      }
      current = current->next;
   }
   // storing NULL as last value
   argsArray[nOfArgs-1] = NULL;
   free(subtokens);
   free(token_node);
   return argsArray;
}

// function to remove leading and trailing spaces in a string
void removeLeadingTrailingSpaces(char* str) {
    int len = strlen(str);
    // Remove leading spaces
    int start = 0;
    while (isspace(str[start])) {
        start++;
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

