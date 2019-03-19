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
    bool isBackground;
    //if any of these are NULL, they are unused
  } Command;

  void shellLoop();
  char *readString();
  char **commandLineParser(char *string);
  void freeCommands(char ** cmds);
  void runCommand(Command command); ///////What about running all commands?
  void fileRedirect(char *file, int inOut);
  Command *makeStructs(char **commands);
  void freeStructs(Command *structs);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
  int main()
  {
    shellLoop();

    return 0;
  }

///////////////////////////////////////////////////////////////////////////
  void shellLoop()
  {
    char *string;
    char **commands;
    Command *commandStructs;

    do
    {
      printf("~ ");
      string = readString();
      printf("string read: %s\n", string);
      if(strcmp(string, "exit") == 0)
      {
        exit(0);
      }

      commands = commandLineParser(string);
      printf("parser done\n");
      free(string);
      int i;
      for(i = 0; commands[i] != NULL; i++)
      {
        printf("%s\n", commands[i]);
      }

      commandStructs = makeStructs(commands);
      printf("structs made\n");
      freeCommands(commands);
      printf("%s\n", commandStructs[1].args[0]);
      for(i = 0; commandStructs[i].args != NULL; i++){
        printf("%s\n", commandStructs[i].args[0]);
      }

      freeStructs(commandStructs);
    } while(1);


    free(commands);
  }

///////////////////////////////////////////////////////////////////////////

  char *readString()
  {
    char temp[BUFFERSIZE];
    char *string;

    fgets(temp, BUFFERSIZE, stdin);

    temp[strlen(temp) - 1] = '\0';

    string = malloc( (sizeof(char) * strlen(temp)) + 1);

    strcpy(string, temp);

    return string;
  }

///////////////////////////////////////////////////////////////////////////

  char **commandLineParser(char *string)
  {
    char **tokens = malloc(BUFFERSIZE * sizeof(char*));
    char * temp;
    int i;

    temp = strtok(string, " ");
    printf("token 1: %s\n", temp);
    for(i = 0; temp != NULL; i++)
    {
      tokens[i] = malloc(sizeof(char) * (strlen(temp) + 1));
      strcpy(tokens[i], temp);
      temp = strtok(NULL, " ");
    }
    tokens[i] = NULL;
    printf("finished tokenizing\n");
    return tokens;
  }

  ////////////////////////////////////////////////////////////////////////

void freeCommands(char ** cmds){
  printf("freeing commands\n");
  int i;
  for(i = 0; cmds[i] != NULL; i++){
    free(cmds[i]);
  }
  free(cmds);
}
///////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////
/*  typedef struct command_struct
  {
    char **args;
    int pipeIn[2];
    int pipeOut[2];
    char *fileDest;
    char *fileSource;
    bool isBackground;
    //if any of these are NULL, they are unused
  } Command;*/
  Command *makeStructs(char **commands)
  {
    printf("begin making structs\n");
    int i;    //to iterate through commands array
    int j = 0;//number of strings in args array
    int k = 0;//number of structs in commandStructs array
    //char **args = malloc(BUFFERSIZE * sizeof(char*));
    Command *commandStructs = malloc(sizeof(Command) * BUFFERSIZE);
    //Command *temp;
    for(i = 0; 1 ; i++){
      printf("struct loop: %d working on %s\n", i, commands[i]);
      if(!commands[i]){
        printf("Hit exit case\n");
        commandStructs[k + 1].args = NULL;
        return commandStructs;

      }else if(strcmp(commands[i], "|") == 0){
        printf("%s\n", commandStructs[k].args[0] );//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        if(pipe(commandStructs[k].pipeOut) < 0){
          printf("pipe failed\n");
        }else{
          memcpy(commandStructs[k + 1].pipeIn, commandStructs[k].pipeOut, 2 * sizeof(int));
        }
          k++;

      }else if(strcmp(commands[i], ">") == 0){
        commandStructs[k].fileDest = malloc(sizeof(char) * (strlen(commands[i + 1]) + 1));
        strcpy(commandStructs[k].fileDest, commands[i + 1]);
        i++;

      }else if(strcmp(commands[i], "<") == 0){
        commandStructs[k].fileSource = malloc(sizeof(char) * (strlen(commands[i + 1]) + 1));
        strcpy(commandStructs[k].fileSource, commands[i + 1]);
        i++;

      }else if(strcmp(commands[i], "&") == 0){
        commandStructs[k].isBackground = true;

      }else{
        //char **tempArgs;
        //tempArgs = realloc(commandStructs[k].args, sizeof(char *) * (j+1));
        commandStructs[k].args = malloc(sizeof(char *) * BUFFERSIZE);
        commandStructs[k].args[j] = malloc(sizeof(char) * (strlen(commands[i]) + 1));
        strcpy(commandStructs[k].args[j], commands[i]/*, sizeof(char) * (strlen(commands[i]) + 1)*/);

        printf("Added %s to args of struct %d\n", commandStructs[k].args[j], k);
        j++;
      }
    }
  }
////////////////////////////////////////////////////////////////////////////////////
  void freeStructs(Command * structs){
    printf("freeing structs\n");
    int i;
    int j;
    for(i = 0; structs[i].args != NULL; i++){
      free(structs[i].fileSource);
      free(structs[i].fileDest);
      for(j = 0; structs[i].args[j]; j++){
        free(structs[i].args[j]);
      }
      free(structs);
    }
  }
