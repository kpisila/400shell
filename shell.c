#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFERSIZE 64

void shellLoop();
void readString(char *string);
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
    readString(string);

    printf("%s\n", string);

    if(strcmp(string, "exit") == 0)
    {
      exit(0);
    }

    commands = commandLineParser(string);

    int i;
    for(i = 0; commands[i] != NULL; i++)
    {
      printf("%s\n", commands[i]);
    }

    free(string);

  } while(1);


  free(commands);
}

void readString(char *string)
{
  char temp[BUFFERSIZE];

  fgets(temp, BUFFERSIZE, stdin);

  string = malloc(sizeof(char) * strlen(temp));

  strcpy(string, temp);
}

char **commandLineParser(char *string)
{
  char **tokens = malloc(BUFFERSIZE * sizeof(char*));
  char * temp;
  int i;

  temp = strtok (string, " ");

  for(i = 0; temp != NULL; i++)
  {
    tokens[i] = temp;

    temp = strtok (NULL, " ");
  }
  tokens[i] = NULL;

  return tokens;
}

void runCommand()
{

}
