/*shell project
  Kai Pisila
  Niall Healy*/

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

#define BUFFERSIZE 64

enum pipeState{READ, WRITE};
//improves code readability]

typedef struct command_struct Command;
struct command_struct
{
  char **args;
  short int argc;
  int pipeIn[2];
  int pipeOut[2];
  char *fileDest;
  char *fileSource;
  bool isBackground;
  Command *nextCommand;
  //if any of these are NULL, they are unused
};

void shellLoop();
char *readString();
char **commandLineParser(char *string);
void freeCommands(char ** cmds);
void runCommand(Command *command);
void printCommand(Command * cmd);
Command *makeStructs(char **commands);
Command *initStruct();
void freeStructs(Command *structs);
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
int main()
{
  shellLoop();

  return 0;
}
////////////////////////////////////////////////////////////////////////////////////
void shellLoop()
{
  char *string;
  char **commands;
  Command *headCommand;

  do
  {
    printf("~ ");
    string = readString();

    //printf("string read: %s\n", string);

    if(strcmp(string, "exit") == 0 || strcmp(string, "") == 0)
    { //implement internal shell command "exit"
      exit(0);
    }

    commands = commandLineParser(string);
    //get array of space seperated tokens

    free(string);
     //free memory for string now that the string has been parsed

    headCommand = makeStructs(commands);
    //using array of tokens, generate linked list of commands

    freeCommands(commands);
     //free memory for array of strings now that the linked list is generated

    Command *temp = headCommand;

    while(temp != NULL)
    { //traverse through list of commands to run them
      //printCommand(temp);

      runCommand(temp);

      temp = temp->nextCommand;
    }

    freeStructs(headCommand);
  } while(1);

  free(commands);
}
////////////////////////////////////////////////////////////////////////////////////
char *readString()
{
  char temp[BUFFERSIZE];
  char *string;

  fgets(temp, BUFFERSIZE, stdin);
  //read from stdin

  temp[strlen(temp) - 1] = '\0';
  //get rid of new line character

  string = malloc( (sizeof(char) * strlen(temp)) + 1);
  //allocate memory for string

  strcpy(string, temp);

  return string;
}
////////////////////////////////////////////////////////////////////////////////////
char **commandLineParser(char *string)
{
  char **tokens = malloc(BUFFERSIZE * sizeof(char*));
  char * temp;
  int i;

  temp = strtok(string, " ");
  //temp stores the first token

  for(i = 0; temp != NULL; i++)
  {
    tokens[i] = malloc(sizeof(char) * (strlen(temp) + 1));
    //allocate memory for token

    strcpy(tokens[i], temp);
    temp = strtok(NULL, " ");
    //get next token
  }
  tokens[i] = NULL;
  //make sure NULL is the last argv so execvp works properly

  //printf("finished tokenizing\n");
  return tokens;
}
////////////////////////////////////////////////////////////////////////////////////
void freeCommands(char ** cmds)
{
  //printf("freeing commands\n");
  int i;
  for(i = 0; cmds[i] != NULL; i++)
  {
    free(cmds[i]);
  }
  free(cmds);
}
////////////////////////////////////////////////////////////////////////////////////
void runCommand(Command *command)
{
  int pid;

  if( (pid = fork()) < 0)
  {
    perror("Forking error\n");
  }
  else if(pid == 0)
  { //CHILD PROCESS
    if(command->fileDest != NULL)
    { //check if there is a file to send output to
      freopen(command->fileDest, "w", stdout);
    }
    if(command->fileSource != NULL)
    { //check if there is a file to take input from
      freopen(command->fileSource, "r", stdin);
    }
    if(command->pipeIn[READ] != -1)
    { //check if there is a pipe to take input from
      close(command->pipeIn[WRITE]);
      dup2(command->pipeIn[READ], STDIN_FILENO);

      close(command->pipeIn[READ]);
    }
    if(command->pipeOut[READ] != -1)
    { //check if there is a pipe to send output to
      close(command->pipeOut[READ]);
      dup2(command->pipeOut[WRITE], STDOUT_FILENO);

      close(command->pipeOut[WRITE]);
    }
    if(command->isBackground == true)
    {
      setpgid(0, 0);
      //Code for background execution goes here
    }

    //printf("About to execvp command.\n");

    if(execvp(command->args[0], command->args) == -1)
    {
      perror("execvp Error\n");
    }
  }
  else
  { //PARENT PROCESS
    if(command->pipeIn[READ] != -1)
    { //check if there is a pipe to take input from
      close(command->pipeIn[WRITE]);
      close(command->pipeIn[READ]);
    }
    if(command->pipeOut[READ] != -1)
    { //check if there is a pipe to send output to
      close(command->pipeOut[READ]);
      close(command->pipeOut[WRITE]);
    }
    if(command->isBackground != true){
      wait(NULL);
      //wait for child to complete if it is not a background process
    }
    return;
  }
}
////////////////////////////////////////////////////////////////////////////////////
Command *makeStructs(char **commands)
{
  int i; //to iterate through commands array
  int k = 0; //number of structs in commandStructs array
  Command *head = initStruct();
  Command *current = head;
  Command *temp;

  for(i = 0; 1 ; i++)
  {
    //printf("struct loop: %d working on %s\n", i, commands[i]);
    //if(current->args[0]){printf("current args[0]: %s\n", current->args[0] );}
    if(!commands[i])
    {
      current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
      current->args[current->argc] = NULL;
      //ensure NULL is last arg so execvp functions properly

      //printf("Hit exit case\n");

      current->nextCommand = NULL;
      return head;
    }
    else if(strcmp(commands[i], "|") == 0)
    {
      current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));

      current->args[current->argc] = NULL;
      //replece "|" with NULL so execvp functions properly

      temp = initStruct();
      current->nextCommand = temp;
      //initialize struct for next command and append to linked list

      if(pipe(current->pipeOut) < 0)
      {
        printf("pipe failed\n");
      }
      else if(pipe(current->nextCommand->pipeIn) < 0)
      {
        printf("pipe failed\n");
      }
      else
      {
        dup2(current->pipeOut[READ], current->nextCommand->pipeIn[READ]);
        dup2(current->pipeOut[WRITE], current->nextCommand->pipeIn[WRITE]);
        //memcpy(current->nextCommand->pipeIn, current->pipeOut, 2 * sizeof(int));
      }
        current = current->nextCommand;
        //move on to next command
        k++;
        //add one to number of command structs
    }
    else if(strcmp(commands[i], ">") == 0)
    {
      current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));

      current->args[current->argc] = NULL;
      //replece ">" with NULL so execvp functions properly

      current->fileDest = malloc(sizeof(char) * (strlen(commands[i + 1]) + 1));
      strcpy(current->fileDest, commands[i + 1]);
      //copy file destination to struct
      i++;
      //add one to index of token array
    }
    else if(strcmp(commands[i], "<") == 0)
    {
      current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));

      current->args[current->argc] = NULL;
      //replece "<" with NULL so execvp functions properly

      current->fileSource = malloc(sizeof(char) * (strlen(commands[i + 1]) + 1));
      strcpy(current->fileSource, commands[i + 1]);
      //copy file source to struct
      i++;
      //add one to index of token array
    }
    else if(strcmp(commands[i], "&") == 0)
    {
      current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));

      current->args[current->argc] = NULL;
      //replece "&" with NULL so execvp functions properly

      current->isBackground = true;
    }
    else
    {
      current->args = realloc(current->args, sizeof(char *) * (current->argc + 1));
      current->args[current->argc] = malloc(sizeof(char) * (strlen(commands[i]) + 1));

      strcpy(current->args[current->argc], commands[i]);
      //copy arg to struct

      current->argc++;
      //update number of args
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////
Command *initStruct()
{ //initialize struct with elements indicating they are not being used (for now)
  Command *cmnd = malloc(sizeof(Command));
  cmnd->args = NULL;
  cmnd->argc = 0;
  cmnd->pipeIn[READ] = -1;
  cmnd->pipeIn[WRITE] = -1;
  cmnd->pipeOut[READ] = -1;
  cmnd->pipeOut[WRITE] = -1;
  cmnd->fileDest = NULL;
  cmnd->fileSource = NULL;
  cmnd->isBackground = false;
  cmnd->nextCommand = NULL;

  return cmnd;
}
////////////////////////////////////////////////////////////////////////////////////
void freeStructs(Command *head)
{
  //printf("freeing structs\n");
  int i;
  Command *temp;
  do
  {
    //printf("free source\n");
    free(head->fileSource);
    //printf("free dest\n");
    free(head->fileDest);

    for(i = 0; head->args[i] != NULL; i++)
    {
      //printf("freeing args %d\n", i);
      free(head->args[i]);
    }

    temp = head->nextCommand;
    free(head);
    head = temp;
  } while(temp != NULL);
}
////////////////////////////////////////////////////////////////////////////////////
void printCommand(Command * cmd)
{
  printf("pipeIn[0]: %d\n", cmd->pipeIn[READ]);
  printf("pipeIn[1]: %d\n", cmd->pipeIn[WRITE]);
  printf("pipeOut[0]: %d\n", cmd->pipeOut[READ]);
  printf("pipeOut[1]: %d\n", cmd->pipeOut[WRITE]);

  printf("File Source: %s\n", cmd->fileSource);
  printf("File Source: %s\n", cmd->fileDest);

  int i;
  for(i = 0; cmd->args[i] != NULL; i++)
  {
    printf("Args %d: %s\n", i, cmd->args[i]);
  }
}
