#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

void interactiveMode();
void batchMode();
char* getPath();
void updatePath(char* newPath);
void cd();
void builtInExit(int status);

int
main(int argc, char* argv[]) {
  // check for number of arguments
  // if number of args while invoking the dash are greater than one i.e, argc > 2 exit the dash
  if (argc > 2) {
    //TODO
    // display standard error message
  }
  else if (argc == 2) {
    //TODO
    // implement batch mode
  }
  else {
    //TODO
    // implement interactive mode
  }

}

void interactiveMode() { }

void batchMode() { }

char* getPath() { return NULL; }

void updatePath(char* newPath) { }

void cd() { }

void builtInExit(int status) { }