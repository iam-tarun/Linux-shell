#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "defs.h"
#include <ctype.h>
#include <unistd.h>


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

char* path; // global path variable, "/bin" is the initial value

// struct defines
CMD_NODE_STRUCT(char);
CMD_TOKENS_STRUCT(char);

void overwritePath(struct Node_char* paths, int len);
char** formatArguments(struct Node_char* args, char **argsArray, char* dir);
void executeRedirectCmd(struct Tokens_char* tokens);
void batchMode(char* fileName);

int main (int argc, char* argv[]) {
   // assigning initial path to the path variable
   char initialPath[] = "/bin;";
   path = (char *)malloc(5);
   path = initialPath;

   // invoke the respective methods based on the count of arguments
   if (argc > 2) { 
      printf("unsupported invoke\n");
      // call the exit process with state as 1
      triggerError();
   }
   else if (argc == 2) {
      printf("batch mode\n");
      //TODO
      // call the batch mode
      batchMode(argv[1]);
   }
   else {
      // printf("interactive mode\n");
      interactiveMode();
   }


}

// function to execute interactive mode
void interactiveMode() {
   char *line = NULL;
   size_t len;
   while (true) {
      printf("dash> ");
      getline(&line, &len, stdin); // read the input
      // printf("input command is %slength is %zu\n", line, len);
      // check if the input is parallel inputs or redirection
      if ( strchr(line, PARALLEL_DELIMITER) != NULL ) {
         // found a '&' symbol in the input, so it is a parallel input
         // printf("it is a parallel input\n");
         struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
         char delimiter[] = "&&";
         tokenizeString(line, delimiter, tokens);
         int pid = 0;
         for(int i = 0; i < tokens->len; i++) {
            pid = fork();
            if (pid == 0) {
               executeCmd(tokens->token->data);
               exit(0);
            }
            tokens->token = tokens->token->next;
         }
         for (int i = 0; i < tokens->len; i++) {
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
         // printf("wrong command\n");
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
      // printf("%sit is a normal command\n", line);
      struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      char space_delimiter[] = " ";
      tokenizeString(input, space_delimiter, tokens);
      removeNewlines(tokens->token->data);
      // printf("%s\n", tokens->token->data);
      // printf("strcmp result is %d\n",strcmp(tokens->token->data, "exit"));
      // check if it belongs to any of the built in command;
      if (strcmp(tokens->token->data, "path") == 0) {
         // printf("it is a path command\n");
         struct Node_char* paths= tokens->token->next;
         overwritePath(paths, tokens->len);
      }
      else if ( strcmp( tokens->token->data, "cd" ) == 0 ) {
         // printf("it is a cd command\n");
         if(tokens->len == 2) {
            removeNewlines(tokens->token->next->data);
            int out = chdir(tokens->token->next->data);
            // printf("%d\n", out);
            if ( out == -1 ) {
               // printf("error while executing cd\n");
               triggerError();
            }
         }
         else {
            // printf("args for cd should be exactly one\n");
            triggerError();
         }
      }
      else if ( strcmp( tokens->token->data, EXIT ) == 0 ) {
         // printf("it is the exit command\n");
         exit(0);
      }
      // if it is not a built in command, try to execute it directly using a child process
      else {
         execChildCmd(tokens);
      }

      free(tokens);
   }
}

// function to validate a redirection command
bool validateRedirectionCmd(char *line) {
   // check the number of occurrences of '>' symbol
   char delimiters[] = ">";
   if(countChar(line, REDIRECTION_DELIMITER) > 1) {
      return false;
   }
   struct Tokens_char *result = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   tokenizeString(line, delimiters, result);
   char* cmd = result->token->data;
   char* filepath = result->token->next->data;
   struct Tokens_char *filepathResult = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   char space_delimiter[] = " ";
   tokenizeString(filepath, space_delimiter, filepathResult);
   if(filepathResult->len != 1) {
      return false;
   }
   free(result);
   free(filepathResult);
   return true;
}

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

// common function to tokenize a string w.r.t the delimiters given
struct Tokens_char tokenizeString(char* str, char delimiter[],struct Tokens_char* result) {
   // storing the tokens in a linked list data structure
   char* input = (char*)malloc(sizeof(str));
   strcpy(input, str);
   result->cmd = str;
   struct Node_char* temp = (struct Node_char*)malloc(sizeof(struct Node_char));
   result->token = temp;
   temp->data = strtok(str, delimiter);
   // removeNewlines(temp->data);
   while(temp->data != NULL) {
      struct Node_char* var = (struct Node_char*)malloc(sizeof(struct Node_char));
      var->data = strtok(NULL, delimiter);
      // removeNewlines(var->data);
      temp->next = var;
      temp = var;
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

// builtin function to overwrite the path
void overwritePath(struct Node_char* paths, int len) {
   path = NULL;
   for(int i = 0; i < len - 1; i++) {
      int totalLength = 0;
      if (path != NULL) {
         totalLength += strlen(path);
      }
      totalLength += strlen(paths->data) + 1;
      path = (char *)realloc(path, totalLength);
      removeNewlines(paths->data);
      strcat(path, paths->data);
      strcat(path, ";");
      paths = paths->next;
   }
}

// standard error message
void triggerError() {
   char error_message[30] = "An error has occurred\n";
   write(STDERR_FILENO, error_message, strlen(error_message));
}

// function to execute a child process using fork, access and execv
int execChildCmd(struct Tokens_char* cmdTokens) {
   char* dir = NULL;
   dir = (char *)malloc(strlen(cmdTokens->token->data) + 1);
   dir = cmdTokens->token->data;
   // printf("%s\n", dir);
   // printf("%d\n",access(dir, X_OK));
   if(access(dir, X_OK) != 0) {
      // check all the paths
      char* fDir = (char *)malloc(strlen(dir) + 1);
      strcpy(fDir, dir);
      struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      char delimiters[] = ";";
      char* pathTemp = strdup(path);
      tokenizeString(pathTemp, delimiters, tokens);
      struct Node_char* paths = (struct Node_char*)malloc(sizeof(struct Node_char));
      paths = tokens->token;
      for ( int i = 0; i < tokens->len; i++ ){
         int totalLength = strlen(fDir);
         totalLength += strlen(paths->data) + 2;
         dir = NULL;
         dir = (char *)realloc(dir, totalLength);
         strcat(dir, paths->data);
         strcat(dir, "/");
         strcat(dir, fDir);
         if (access(dir, X_OK) == 0) {
            // found the dir
            break;
         }
         paths = paths->next;
      } 

   }
   if(access(dir, X_OK) == 0) {
      // printf("%s\n", dir);
      char** argsArray;
      argsArray = formatArguments(cmdTokens->token->next, argsArray, dir);
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
   }//190290
   // 7X4BMFXC
   return 0;
}

// function to format the arguments
char** formatArguments(struct Node_char* args, char **argsArray, char* dir) {
   char* dirCopy = strdup(dir);
   char delimiters[] = "/";
   struct Tokens_char* subtokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
   tokenizeString(dirCopy, delimiters, subtokens);
   struct Node_char* token_node = (struct Node_char*)malloc(sizeof(struct Node_char));
   token_node = subtokens->token;
   // printf("%d\n", subtokens->len);
   int p = 0;
   for(; p < subtokens->len - 1; p++) {
      token_node = token_node->next;
   }
   int nOfArgs = 2;
   struct Node_char* current = args;
   while ((current->data != NULL) && (current->data != NULL) ) {
      nOfArgs++;
      current = current->next;
   }
   // printf("number of args are %d\n", nOfArgs);
   argsArray = (char **)malloc(nOfArgs * sizeof(char *));
   current = args;
   // printf("program name is %s\n", token_node->data);
   argsArray[0] = token_node->data;
   int i = 1;
   for ( ; i < nOfArgs-1; i++ ) {
      removeLeadingTrailingSpaces(current->data);
      if ( current->data != NULL ) {
         argsArray[i] = strdup(current->data);
         // printf("%s\n", argsArray[i]);
      }
      current = current->next;
   }
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
    for (int i = start; i <= end; i++) {
        str[i - start] = str[i];
    }
    // Null-terminate the modified string
    str[end - start + 1] = '\0';
}

// function to redirect the output to a file
void executeRedirectCmd(struct Tokens_char* tokens) {
   char* fileName = tokens->token->next->data;
   FILE* file = fopen(fileName, "w");
   if(file == NULL) {
      triggerError();
   }
   else {
      freopen(fileName, "w", stdout);
      struct Tokens_char* subTokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      char delimiter[] = " ";
      tokenizeString(tokens->token->data, delimiter, subTokens);
      execChildCmd(subTokens);
      fclose(file);
      freopen("/dev/tty", "w", stdout);
      free(subTokens);
   }
}

void batchMode(char* fileName) {
   FILE* file = fopen(fileName, "r");
   if(file == NULL) {
      triggerError();
   }
   else {
      char* line = NULL;
      size_t len = 0;
      while(getline(&line, &len, file) != -1 ) {
         if ( strchr(line, PARALLEL_DELIMITER) != NULL ) {
         // found a '&' symbol in the input, so it is a parallel input
         // printf("it is a parallel input\n");
         struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
         char delimiter[] = "&&";
         tokenizeString(line, delimiter, tokens);
         int pid = 0;
         for(int i = 0; i < tokens->len; i++) {
            pid = fork();
            if (pid == 0) {
               executeCmd(tokens->token->data);
               exit(0);
            }
            tokens->token = tokens->token->next;
         }
         for (int i = 0; i < tokens->len; i++) {
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