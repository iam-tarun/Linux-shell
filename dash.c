#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "defs.h"


// function declarations
void interactiveMode();
void executeCmd(char* line);
bool validateRedirectionCmd(char *line);
int countChar(char* line, char target);
struct Tokens_char tokenizeString(char* str, char delimiter[],struct Tokens_char* result);
CMD_NODE_STRUCT(char);
CMD_TOKENS_STRUCT(char);

int main (int argc, char* argv[]) {
   //macro structs
   
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
         // it is not a parallel command
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
   }
   else {
      printf("%sit is a normal command\n", line);
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

// common function to tokenize a string
struct Tokens_char tokenizeString(char* str, char delimiter[],struct Tokens_char* result) {
   
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
   free(input);
   free(temp);
   return *result;
}
