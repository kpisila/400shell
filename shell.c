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
  int runCommand(Command *command);
  void fileRedirect(char *file, int inOut);
  Command *makeStructs(char **commands);
  Command * initStruct();
  void freeStructs(Command *structs);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
  int main()
  {
    shellLoop();

    return 0;
  }

//----------------------------------------------------------------------//
///////////////////////////////////////////////////////////////////////////
  void shellLoop()
  {
    char *string;
    char **commands;
    Command *headCommand;
    int pid;

    do
    {
      printf("~ ");
      string = readString();
      //printf("string read: %s\n", string);
      if(strcmp(string, "exit") == 0 || strcmp(string, "") == 0)
      {
        exit(0);
      }

      commands = commandLineParser(string);
      //printf("parser done\n");
      free(string);
      /*int i;
      for(i = 0; commands[i] != NULL; i++)
      {
        printf("%s\n", commands[i]);
      }*/

      headCommand = makeStructs(commands);
      //printf("structs made\n");
      freeCommands(commands);
      //printf("headCommand args[0]: %s\n", headCommand->args[0]);

///////////// THIS SECTION CAN BE USED TO ITERATE THROUGH    //////////////
///////////// LINKED LIST OF COMMAND STRUCTS. IT ONLY PRINTS //////////////
///////////// OUT THE FIRST ARGUMENT RIGHT NOW TO VERYIFY    //////////////
///////////// THAT ALL COMMANDS ARE SAVED IN THE LIST        //////////////

      Command *temp = headCommand;

      if( (pid = fork()) < 0)
      {
        perror("Forking error\n");
      }
      else if(pid == 0)
      { //CHILD PROCESS
        while(temp->nextCommand != NULL)
        {
          printf("args[0]: %s\n\tpipeIn[0]: %d\n\tpipeOut[0]: %d\n", temp->args[0], temp->pipeIn[0], temp->pipeOut[0]);
          runCommand(temp);
          close(temp->pipeOut[WRITE]);
          temp = temp->nextCommand;
        }
        if(temp->pipeIn[READ] > 0)
        {
          dup2(temp->pipeIn[READ], STDIN_FILENO);
        }
        if(execvp(temp->args[0], temp->args) == -1)
        {
          perror("Last exec call\n");
        }
      }
      else
      { //PARENT
        wait(NULL);
      }

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

      freeStructs(headCommand);
      //printf("structs freed\n");
    } while(1);


    free(commands);
  }

///////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------//

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
    //printf("token 1: %s\n", temp);
    for(i = 0; temp != NULL; i++)
    {
      tokens[i] = malloc(sizeof(char) * (strlen(temp) + 1));
      strcpy(tokens[i], temp);
      temp = strtok(NULL, " ");
    }
    tokens[i] = NULL;
    //printf("finished tokenizing\n");
    return tokens;
  }

  ////////////////////////////////////////////////////////////////////////

void freeCommands(char ** cmds){
  //printf("freeing commands\n");
  int i;
  for(i = 0; cmds[i] != NULL; i++){
    free(cmds[i]);
  }
  free(cmds);
}
///////////////////////////////////////////////////////////////////////////

int runCommand(Command *command)
{
  int pid, w, status;

  if( (pid = fork()) < 0)
  {
    perror("Forking error\n");
  }
  else if(pid == 0)
  { //CHILD PROCESS
    if(command->nextCommand == NULL)
    {

    }
    if(command->args == NULL)
    {
      printf("Error, no arguments!\n");
    }
    if(command->fileDest != NULL)
    {
      fileRedirect(command->fileDest, OUT);
    }
    if(command->fileSource != NULL)
    {
      fileRedirect(command->fileSource, IN);
    }
    if(command->pipeIn[READ] > 0)
    {
      //close(command->pipeIn[WRITE]);
      if(dup2(command->pipeIn[READ], STDIN_FILENO) == -1)
      {
        perror("Dup2 failed\n");
      }

      if(close(command->pipeIn[READ]) == -1)
      {
        perror("Close failed\n");
      }
    }
    if(command->pipeOut[WRITE] > 0)
    {
      //printf("About to redirect output of %s\n", command->args[0]);
      //close(command->pipeOut[READ]);
      if(dup2(command->pipeOut[WRITE], STDOUT_FILENO) == -1)
      {
        perror("Dup2 failed\n");
      }

      if(close(command->pipeOut[WRITE]) == -1)
      {
        perror("Close failed\n");
      }
    }
    if(command->isBackground == true)
    {
      //Code for background execution goes here
    }

    //printf("About to execvp %s.\n", command->args[0]);

    return execvp(command->args[0], command->args);

  }
  else
  { //PARENT PROCESS
    /*do{ //WAIT CODE TAKEN FROM http://www.tutorialspoint.com/unix_system_calls/waitpid.htm
      //printf("Waiting in parent\n");
      w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
      if (w == -1)
      {
        perror("waitpid");
      }

    } while(!WIFEXITED(status) && !WIFSIGNALED(status));*/
    return pid;
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

  Command *makeStructs(char **commands)
  {
    //printf("begin making structs\n");
    int i;    //to iterate through commands array
    //int j = 0;//number of strings in args array
    int k = 0;//number of structs in commandStructs array
    Command *head = initStruct();
    Command *current = head;
    Command *temp;

    for(i = 0; 1 ; i++){
      //printf("struct loop: %d working on %s\n", i, commands[i]);
      //if(current->args[0]){printf("current args[0]: %s\n", current->args[0] );}
      if(!commands[i]){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        //printf("Hit exit case\n");
        current->nextCommand = NULL;
        return head;

      }else if(strcmp(commands[i], "|") == 0){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        temp = initStruct();
        current->nextCommand = temp;
        //printf("current args[0]: %s\n", current->args[0] );//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        if(pipe(current->pipeOut) < 0){
          printf("pipe failed\n");
        }else{
          memcpy(current->nextCommand->pipeIn, current->pipeOut, 2 * sizeof(int));
        }
          current = current->nextCommand;
          k++;

      }else if(strcmp(commands[i], ">") == 0){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        current->fileDest = malloc(sizeof(char) * (strlen(commands[i + 1]) + 1));
        strcpy(current->fileDest, commands[i + 1]);
        i++;

      }else if(strcmp(commands[i], "<") == 0){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        current->fileSource = malloc(sizeof(char) * (strlen(commands[i + 1]) + 1));
        strcpy(current->fileSource, commands[i + 1]);
        i++;

      }else if(strcmp(commands[i], "&") == 0){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        current->isBackground = true;

      }else{
        //char **tempArgs;
        //tempArgs = realloc(commandStructs[k].args, sizeof(char *) * (j+1));
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 1));
        current->args[current->argc] = malloc(sizeof(char) * (strlen(commands[i]) + 1));
        strcpy(current->args[current->argc], commands[i]);

        //printf("Added %s to args of struct %d\n", current->args[current->argc], k);
        current->argc++;
      }
    }
  }
////////////////////////////////////////////////////////////////////////////////////
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
Command * initStruct(){
  Command *cmnd = malloc(sizeof(Command));
  cmnd->args = NULL;
  cmnd->argc = 0;
  cmnd->pipeIn[0] = -1;
  cmnd->pipeIn[1] = -1;
  cmnd->pipeOut[0] = -1;
  cmnd->pipeOut[1] = -1;
  cmnd->fileDest = NULL;
  cmnd->fileSource = NULL;
  cmnd->isBackground = false;
  cmnd->nextCommand = NULL;

  return cmnd;

}
////////////////////////////////////////////////////////////////////////////////////
  void freeStructs(Command *head){
    //printf("freeing structs\n");
    int i;
    Command *temp;
    do{
      //printf("free source\n");
      free(head->fileSource);
      //printf("free dest\n");
      free(head->fileDest);
      for(i = 0; head->args[i] != NULL; i++){
        //printf("freeing args %d\n", i);
        free(head->args[i]);
      }
      temp = head->nextCommand;
      free(head);
      head = temp;
    }while(temp != NULL);
}
