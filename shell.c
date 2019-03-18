#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

void shellLoop();
char *readString();
char **commandLineParser(char *string);
void runCommand();

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

    commands = commandLinePrser(string);


  } while();
}

char *readString()
{

}

char **commandLineParser(char *string)
{

}

void runCommand()
{
  
}
