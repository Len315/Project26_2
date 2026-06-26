/*
Sources:
  Mohamed Adan, A. "How a Simple Unix Shell Works"
    https://medium.com/@aminmohamedadan/how-a-simple-unix-shell-works-6413d6c41ffc
    -Used for understanding the general architecture of
     a Unix shell (parse, fork, exec, wait).
  Brennan, S. "Write a Shell in C"
    https://brennan.io/2015/01/16/write-a-shell-in-c/
    -Used as a reference when encountering implementation difficulties,
     particularly to understand shell execution flow (fork/exec/wait model).
AI usage: Checking grammar, style and spelling
*/

// Simple Unix Shell
// Handles built-in commands: exit, cd, path
// Excecutes external programs from configurable paths
// Handles output redirection ">" and parallel execution "&"
// Operates in interactive(stdin) or batchfile mode
// Parses and executes commands line-by-line with consistent error handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAXLINE 1024
#define MAXPATH 512
#define CAPACITY 2

char error_message[] = "An error has occurred\n";

// Stores directories, updated with path built-in commands
typedef struct pathlist{
  char** paths;
  int iPathCount;
}PathList;

typedef struct stringlist{
  char** strings;
  int iSize;
  int iCapacity;
}StringList;

void error();// Prints the error message specified in the assignment
void initializePaths(PathList* pPathList);// Initializes paths with "/bin" and allocates memory for path strings
void freePaths(PathList* pPathList);
StringList* createStringList();
void addString(StringList* pList, char* pString);// Adds a string to the StringList, reallocating if necessary
void freeStringList(StringList* pList);
void runShell(FILE* input, PathList* pPathList);
void parseLine(char* pLine, PathList* pPathList);// Parses a command line, splits and processes the commands. Called by runShell
void executeCommand(char** args, int argc, char* pOutputFile, PathList* pPathList);// Executes non-built-in commands like ls. Called by parseLine. Finds the command in the path and executes with execv
void splitCommands(char* pLine, char** ppCommands, int* iCount, const char* pDelimiter);// Used to split commands for parallel execution (commands separated with &). Splits a string into commands and stores them in ppCommands array
int checkBuiltin(char* pCommand);// Checks if a command is a built-in (exit, cd, path), called by parseLine
void handleBuiltin(char** args, int argc, PathList* pPathList);// Handles built-in commands (exit, cd, path) and performs their operations. Called by parseLine

void error(){
  write(STDERR_FILENO, error_message, strlen(error_message));
}

void initializePaths(PathList* pPathList){
  pPathList->iPathCount = 1;
  pPathList->paths = malloc(sizeof(char*));
  pPathList->paths[0] = strdup("/bin");
}

void freePaths(PathList* pPathList){
  for(int i = 0; i < pPathList->iPathCount; i++){// Free all paths
    free(pPathList->paths[i]);
  }
  free(pPathList->paths);
  pPathList->paths = NULL;
  pPathList->iPathCount = 0;
}

StringList* createStringList(){
  StringList* pList = malloc(sizeof(StringList));
  pList->iSize = 0;
  pList->iCapacity = CAPACITY;
  pList->strings = malloc(sizeof(char*) * pList->iCapacity);
  return pList;
}

void addString(StringList* pList, char* pString){
  if(pList->iSize >= pList->iCapacity){// Double capacity if reached
    pList->iCapacity *= 2;
    pList->strings = realloc(pList->strings, sizeof(char*) * pList->iCapacity);
  }
  if(pString != NULL){// Prevent NULL input to strdup
    pList->strings[pList->iSize] = strdup(pString);
  }else{
    pList->strings[pList->iSize] = NULL;
  }
  pList->iSize++;
}

void freeStringList(StringList* pList){
  for(int i = 0; i < pList->iSize; i++){// Free all strings
    free(pList->strings[i]);
  }
  free(pList->strings);
  free(pList);
}

void runShell(FILE* input, PathList* pPathList){
  char* pLine = NULL;
  size_t len = 0;

  while(1){
    if(input == stdin){printf("wish> ");}// Print prompt when input is not from file
    if(getline(&pLine, &len, input) == -1){break;}// EOF
      if(strspn(pLine, " \t\n") == strlen(pLine)){continue;}// Skip empty lines: whitespace, tabs, newlines
    parseLine(pLine, pPathList);// Parse and handle commands in the line
  }
  free(pLine);
}

void parseLine(char* pLine, PathList* pPathList){
  char* lCommands[100], *nl, *pRedirectPosition, *pCommand, *pOutputFile;
  int iCommandCount = 0;

  splitCommands(pLine, lCommands, &iCommandCount, "&");// Split commands by & and store in lCommands array

  pid_t pids[iCommandCount];// Array for process IDs

  for(int i = 0; i < iCommandCount; i++){
    nl = strchr(lCommands[i], '\n');// Remove newline at the end
    if(nl){*nl = '\0';}// If newline is found, replace it with null terminator

    pRedirectPosition = strchr(lCommands[i], '>');// ">" character redirects to the following file. E.g. ls > some.txt
    pCommand = lCommands[i];
    pOutputFile = NULL;

    if(pRedirectPosition){
      *pRedirectPosition = '\0';// Place null terminator at the position of '>'
      pRedirectPosition++;// Move to the right of '>', e.g. " some.txt"

      while(*pRedirectPosition == ' ' || *pRedirectPosition == '\t'){
	pRedirectPosition++;// Trim all whitespace -> "some.txt"
      }

      pOutputFile = strtok(pRedirectPosition, " \t");// E.g. some.txt
      // If output file is not found or multiple files are present
      if(!pOutputFile || strtok(NULL, " \t")){
        error();
        return;
      }
      if(strspn(pCommand, " \t") == strlen(pCommand)){// Empty command
        error();
        return;
      }
    }

    StringList* args = createStringList();
    char* pToken = strtok(pCommand, " \t");
    
    while(pToken != NULL){
      addString(args, pToken);// strdup is inside addString
      pToken = strtok(NULL, " \t");
    }
    addString(args, NULL); // NULL-terminate for execv compatibility

    if(args->iSize <= 1){// Empty command
      freeStringList(args);
      continue;
    }
    if(checkBuiltin(args->strings[0])){// If it's a built-in command
      if(pOutputFile != NULL){
	error();
        freeStringList(args);
        return;
      }
      handleBuiltin(args->strings, args->iSize - 1, pPathList);
      freeStringList(args);
    }else{
      pid_t pid = fork();// Create child process
      if(pid == 0){// Child process execution
        executeCommand(args->strings, args->iSize - 1, pOutputFile, pPathList);
        freeStringList(args);
        exit(0);
      }else if (pid > 0){// Store child process PID
        pids[i] = pid;
      }else{
	error();
      }
    }
  }
  // Wait for all child processes to finish
  for(int i = 0; i < iCommandCount; i++){
    waitpid(pids[i], NULL, 0);
  }
}

void executeCommand(char** args, int argc, char* pOutputFile, PathList* pPathList){
  int iFd;
  char tFullPath[MAXPATH];

  if(pOutputFile){
    // Open file for writing, create if not exists and truncate old content 
    iFd = open(pOutputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if(iFd < 0){
      error();
      exit(1);
    }
    dup2(iFd, STDOUT_FILENO);// Redirect stdout to file
    dup2(iFd, STDERR_FILENO);
    close(iFd);
  }

  for(int i = 0; i < pPathList->iPathCount; i++){// Store path, check commands and execute
    snprintf(tFullPath, sizeof(tFullPath), "%s/%s", pPathList->paths[i], args[0]);
    if(access(tFullPath, X_OK) == 0){
      execv(tFullPath, args);
    }
  }
  error();
  exit(1);
}

void splitCommands(char* pLine, char** ppCommands, int* iCount, const char* pDelimiter){// Parrarel komentojen jakaminen "&"-merkin perusteella
  char* pToken = strtok(pLine, pDelimiter);
  *iCount = 0;
  while(pToken != NULL){
    ppCommands[(*iCount)++] = pToken;
    pToken = strtok(NULL, pDelimiter);
  }
}

int checkBuiltin(char* pCommand){
  return strcmp(pCommand, "exit") == 0 || strcmp(pCommand, "cd") == 0 || strcmp(pCommand, "path") == 0;
}

void handleBuiltin(char** args, int argc, PathList* pPathList){
  if(strcmp(args[0], "exit") == 0){// Buily-in exit
    if(argc != 1){
      error();
    }else{
      exit(0);
    }
  }else if(strcmp(args[0], "cd") == 0){// Built-in cd
    if(argc != 2){
      error();
    }else if(chdir(args[1]) != 0){
      error();
    }
  }else if(strcmp(args[0], "path") == 0){// Built-in path
    freePaths(pPathList);
    pPathList->iPathCount = argc - 1;
    pPathList->paths = malloc(sizeof(char*) * pPathList->iPathCount);
    for(int i = 1; i < argc; i++){// Store paths
      pPathList->paths[i - 1] = strdup(args[i]);
    }
  }
}

int main(int argc, char* argv[]){
  PathList pathList;
  initializePaths(&pathList);

  if(argc == 1){
    runShell(stdin, &pathList);
  }else if(argc == 2){
    FILE* batchFile = fopen(argv[1], "r");
    if(!batchFile){
      error();
      exit(1);
    }
    runShell(batchFile, &pathList);
    fclose(batchFile);
  }else{
    error();
    exit(1);
  }

  freePaths(&pathList);
  return 0;
}
