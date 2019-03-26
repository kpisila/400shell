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

  enum inOut{IN, OUT};
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
  void runCommand(Command *command);
  void printCommand(Command * cmd);
  void fileRedirect(char *file, int inOut);
  Command *makeStructs(char **commands);
  Command *initStruct();
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



      //printf("args[0]: %s\n\tpipeIn[0]: %d\n\tpipeOut[0]: %d\n", temp->args[0], temp->pipeIn[0], temp->pipeOut[0]);
      Command *temp = headCommand;
      while(temp != NULL){
        //printf("args[0]: %s\n\tpipeIn[0]: %d\n\tpipeOut[0]: %d\n", temp->args[0], temp->pipeIn[0], temp->pipeOut[0]);
        printCommand(temp);
        runCommand(temp);
        temp = temp->nextCommand;
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

    fgets(temp, BUFFERSIZE, stdin); //read from stdin

    temp[strlen(temp) - 1] = '\0'; //get rid of new line character

    string = malloc( (sizeof(char) * strlen(temp)) + 1); //allocate memory for string

    strcpy(string, temp);

    return string;
  }

///////////////////////////////////////////////////////////////////////////

  char **commandLineParser(char *string)
  {
    char **tokens = malloc(BUFFERSIZE * sizeof(char*));
    char * temp;
    int i;

    temp = strtok(string, " "); //temp stores the first token
    //printf("token 1: %s\n", temp);
    for(i = 0; temp != NULL; i++)
    {
      tokens[i] = malloc(sizeof(char) * (strlen(temp) + 1)); //allocate memory for token
      strcpy(tokens[i], temp);
      temp = strtok(NULL, " "); //get next token
    }
    tokens[i] = NULL; //make sure NULL is the last argv so execvp works properly
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

  void runCommand(Command *command)
  {
    int pid, w, status;

    if( (pid = fork()) < 0)
    {
      perror("Forking error\n");
    }
    else if(pid == 0)
    { //CHILD PROCESS
      if(command->fileDest != NULL)
      { //check if there is a file to send output to
        fileRedirect(command->fileDest, OUT);
      }
      if(command->fileSource != NULL)
      { //check if there is a file to take input from
        fileRedirect(command->fileSource, IN);
      }
      if(command->pipeIn[0] != -1)
      { //check if there is a pipe to take input from
        close(command->pipeIn[WRITE]);
        dup2(command->pipeIn[READ], STDIN_FILENO);

        close(command->pipeIn[READ]);
      }
      if(command->pipeOut[0] != -1)
      { //check if there is a pipe to send output to
        close(command->pipeOut[READ]);
        dup2(command->pipeOut[WRITE], STDOUT_FILENO);

        close(command->pipeOut[WRITE]);
      }
      if(command->isBackground == true)
      {
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
      if(command->pipeIn[0] != -1)
      { //check if there is a pipe to take input from
        close(command->pipeIn[WRITE]);
        close(command->pipeIn[READ]);
      }
      if(command->pipeOut[0] != -1)
      { //check if there is a pipe to send output to
        close(command->pipeOut[READ]);
        close(command->pipeOut[WRITE]);
      }
      wait(NULL); //wait for child to complete
    }
  }

///////////////////////////////////////////////////////////////////////////

  void fileRedirect(char *file, int inOut)
  { //perform specified file redirection
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
        }else if(pipe(current->nextCommand->pipeIn) < 0){
          printf("pipe failed\n");
        }else{
          dup2(current->pipeOut[0], current->nextCommand->pipeIn[0]);
          dup2(current->pipeOut[1], current->nextCommand->pipeIn[1]);
          //memcpy(current->nextCommand->pipeIn, current->pipeOut, 2 * sizeof(int));
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

void printCommand(Command * cmd){
  printf("pipeIn[0]: %d\n", cmd->pipeIn[0]);
  printf("pipeIn[1]: %d\n", cmd->pipeIn[1]);
  printf("pipeOut[0]: %d\n", cmd->pipeOut[0]);
  printf("pipeOut[1]: %d\n", cmd->pipeOut[1]);

  printf("File Source: %s\n", cmd->fileSource);
  printf("File Source: %s\n", cmd->fileDest);
  int i;

    for(i = 0; cmd->args[i] != NULL; i++){
      printf("Args %d: %s\n", i, cmd->args[i]);
    }
}
