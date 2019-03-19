#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFERSIZE 64

enum inOut{IN, OUT, BOTH, NEITHER};
enum pipeState{READ, WRITE};
//improves code readability

typedef struct command_struct
{
  char **args;
  int pipeIn[2];
  int pipeOut[2];
  char *fileDest;
  char *fileSource;
  //if any of these are NULL, they are unused
} Command;

void shellLoop();
char *readString();
char **commandLineParser(char *string);
void runCommand(Command command);
void fileRedirect(char *file, int inOut);
Command *makeStructs(char **commands);

int main()
{
  shellLoop();

  return 0;
}

void shellLoop()
{
  char *string;
  char **commands;

  do
  {
    printf("~ ");
    string = readString();

    if(strcmp(string, "exit") == 0)
    {
      exit(0);
    }

    commands = commandLineParser(string);

    /*int i;
    for(i = 0; commands[i] != NULL; i++)
    {
      printf("%s\n", commands[i]);
    }*/

    free(string);

  } while(1);


  free(commands);
}

char *readString()
{
  char temp[BUFFERSIZE];
  char *string;
  int i;

  fgets(temp, BUFFERSIZE, stdin);

  temp[strlen(temp) - 1] = '\0';

  string = malloc( (sizeof(char) * strlen(temp)) + 1);

  strcpy(string, temp);

  return string;
}

char **commandLineParser(char *string)
{
  char **tokens = malloc(BUFFERSIZE * sizeof(char*));
  char * temp;
  int i;

  temp = strtok(string, " ");

  for(i = 0; temp != NULL; i++)
  {
    tokens[i] = temp;

    temp = strtok(NULL, " ");
  }
  tokens[i] = NULL;

  return tokens;
}

void runCommand(Command command)
{
  int pid, w, status;

  if( (pid = fork()) < 0)
  {
    perror("Forking error\n");
  }
  else if(pid == 0)
  { //CHILD PROCESS
    if(command.fileDest != NULL)
    {
      fileRedirect(command.fileDest, OUT);
    }
    else if(command.fileSource != NULL)
    {
      fileRedirect(command.fileSource, IN);
    }
    else if(command.pipeIn != NULL)
    {
      close(command.pipeIn[WRITE]);
      dup2(command.pipeIn[READ], STDIN_FILENO);

      close(command.pipeIn[READ]);
    }
    else if(command.pipeOut != NULL)
    {
      close(command.pipeOut[READ]);
      dup2(command.pipeOut[WRITE], STDOUT_FILENO);

      close(command.pipeOut[WRITE]);
    }

    if(execvp(command.args[0], command.args) == -1)
    {
      perror("execvp Error\n");
    }
  }
  else
  { //PARENT PROCESS
    do{ //WAIT CODE TAKEN FROM http://www.tutorialspoint.com/unix_system_calls/waitpid.htm
      w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
      if (w == -1)
      {
        perror("waitpid");
      }

    } while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }
}

void fileRedirect(char *file, int inOut)
{
  if(inOut == IN)
  {
    freopen(file, "r", stdin);
  }
  else
  {
    freopen(file, "w", stdout);
  }
}

Command *makeStructs(char **commands)
{

}
