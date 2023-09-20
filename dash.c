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


char* path; // global path variable, "/bin" is the initial value

// struct defines
CMD_NODE_STRUCT(char);
CMD_TOKENS_STRUCT(char);

void overwritePath(struct Node_char* paths, int len);


int main (int argc, char* argv[]) {
   // assigning initial path to the path variable
   char initialPath[] = "/bin;";
   path = (char *)malloc(5);
   path = initialPath;

   // invoke the respective methods based on the count of arguments
   if (argc > 2) { 
      printf("unsupported invoke\n");
      //TODO
      // call the exit process with state as 1
   }
   else if (argc == 2) {
      printf("batch mode\n");
      //TODO
      // call the batch mode
   }
   else {
   //  printf("interactive mode\n");
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
   if ( strchr(line, REDIRECTION_DELIMITER) != NULL ) {
      // printf("%sit is a redirection command\n", line);
      if ( !validateRedirectionCmd(line) ) {
         printf("wrong command\n");
      }
      else {
         //TODO
         // implement redirection command process
      }
   }
   else {
      // printf("%sit is a normal command\n", line);
      struct Tokens_char* tokens = (struct Tokens_char*)malloc(sizeof(struct Tokens_char));
      char space_delimiter[] = " ";
      tokenizeString(line, space_delimiter, tokens);
      removeNewlines(tokens->token->data);
      // printf("%s\n", tokens->token->data);
      // printf("strcmp result is %d\n",strcmp(tokens->token->data, "exit"));
      if (strcmp(tokens->token->data, "path") == 0) {
         printf("it is a path command\n");
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
               printf("error while executing cd\n");
               //TODO
               // need to display standard error message
            }
         }
         else {
            printf("args for cd should be exactly one\n");
            //TODO
            // need to display standard error message
         }
      }
      else if ( strcmp( tokens->token->data, EXIT ) == 0 ) {
         printf("it is the exit command\n");
         exit(0);
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
   while(temp->data != NULL) {
      struct Node_char* var = (struct Node_char*)malloc(sizeof(struct Node_char));
      var->data = strtok(NULL, delimiter);
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
