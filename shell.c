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
    char *fileDest;
    char *fileSource;
    bool isBackground;
    Command *nextCommand;
    //if any of these are NULL, they are unused
  };

//////////////////////////////////////////////////////////////////////////

  void shellLoop();
  char *readString();
  char **commandLineParser(char *string);
  void freeCommands(char ** cmds);
  int runCommand(Command *command);
  void fileRedirect(char *file, int inOut);
  Command *makeStructs(char **commands);
  Command * initStruct();
  void freeStructs(Command *structs);

//////////////////////////// MAIN /////////////////////////////////////////
//-----------------------------------------------------------------------//

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
      if(strcmp(string, "exit") == 0 || strcmp(string, "") == 0)
      {
        exit(0);
      }

      commands = commandLineParser(string);
      free(string);

      headCommand = makeStructs(commands);
      freeCommands(commands);

      Command *command = headCommand;

      //++++++++++++++++++++++ Fork a child to exec commands +++++++++++++++++//

      if( (pid = fork()) < 0){
        perror("Forking error\n");
      }else if(pid == 0){

        ////////////EXEC CHILD PROCESS/////////////
        int pid, status;
        int pipeOut[2];
        int pipeIn[2];
        int notFirst = 0;

            //====== CREATE PIPES ======//
        if(pipe(pipeOut) < 0){
          printf("pipeOut failed\n");
        }
        if(pipe(pipeIn) < 0){
          printf("pipeIn failed\n");
        }
            //====== CHECK IF FILE SOURCE ======//
        if(command->fileSource != NULL){
          fileRedirect(command->fileSource, IN);
        }

            //====== IF MORE THAT 1 COMMAND ======//
            //====== EXECUTE ALL COMMANDS   ======//
        while(command->nextCommand != NULL){

          if( (pid = fork() ) < 0){
            perror("Forking error\n");
          }
          else if(pid == 0){
            /////////// LOOP CHILD PROCESS /////////////
            printf("in child\n");

            if(command->args == NULL){
              printf("Error, no arguments!\n");
            }
            //================== PIPING ======================//
                    //////////// Redirect stdin //////////
            if(notFirst){
              printf("this is not the first command\n");
              close(pipeIn[WRITE]);
              if(dup2(pipeIn[READ], STDIN_FILENO) == -1){
                perror("Dup2 failed\n");
              }
              if(close(pipeIn[READ]) == -1){
                perror("Close failed\n");
              }
            }
                    /////// Redirect stdout ////////////
            if(command->nextCommand != NULL){
              printf("About to redirect output of %s\n", command->args[0]);

// ERROR OCCURS ERROR OCCURS ERROR OCCURS ERROR OCCURS ERROR OCCURS ERROR OCCURS //

              close(pipeOut[READ]);
              if(dup2(pipeOut[WRITE], STDOUT_FILENO) == -1){
                perror("Dup2 failed\n");
              }
              if(close(pipeOut[WRITE]) == -1){
                perror("Close failed\n");
              }
            }
            //===================================================//

            if(command->isBackground == true){
              //Code for background execution goes here
            }

            execvp(command->args[0], command->args);
          }else{
            ////////// LOOP PARENT ///////////////

            close(pipeIn[WRITE]);
            close(pipeIn[READ]);

            close(pipeOut[WRITE]);
            close(pipeOut[READ]);

            waitpid(pid, &status, 0);
            command = command->nextCommand;
          }
          notFirst = 1; //TO DETERMINE IF STDIN NEEDS TO BE REDIRECTED
        }

        ////////////// Outside While Loop /////////////////
        if(command->fileDest != NULL){
          fileRedirect(command->fileDest, OUT);
        }
        if(pipeOut[READ] > 0)
        {
          dup2(pipeIn[READ], STDIN_FILENO);
        }
        if(execvp(command->args[0], command->args) == -1)
        {
          perror("Last exec call\n");
        }
      }else{
        /////////// PARENT /////////////
        wait(NULL);
      }

      freeStructs(headCommand);
    } while(1);
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
    for(i = 0; temp != NULL; i++)
    {
      tokens[i] = malloc(sizeof(char) * (strlen(temp) + 1));
      strcpy(tokens[i], temp);
      temp = strtok(NULL, " ");
    }
    tokens[i] = NULL;

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

  void fileRedirect(char *file, int inOut){
    if(inOut == IN){
      freopen(file, "r", stdin);
    }
    else{
      freopen(file, "w", stdout);
    }
  }

///////////////////////////////////////////////////////////////////////////

  Command *makeStructs(char **commands){
    Command *head = initStruct();
    Command *current = head;
    Command *temp;

    int i;
    for(i = 0; 1 ; i++){
      if(!commands[i]){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        current->nextCommand = NULL;
        return head;

      }else if(strcmp(commands[i], "|") == 0){
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 2));
        current->args[current->argc] = NULL;
        temp = initStruct();
        current->nextCommand = temp;
        current = current->nextCommand;

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
        current->args = realloc(current->args, sizeof(char *) * (current->argc + 1));
        current->args[current->argc] = malloc(sizeof(char) * (strlen(commands[i]) + 1));
        strcpy(current->args[current->argc], commands[i]);
        current->argc++;
      }
    }
  }

////////////////////////////////////////////////////////////////////////////////////

Command * initStruct(){
  Command *cmnd = malloc(sizeof(Command));
  cmnd->args = NULL;
  cmnd->argc = 0;
  cmnd->fileDest = NULL;
  cmnd->fileSource = NULL;
  cmnd->isBackground = false;
  cmnd->nextCommand = NULL;

  return cmnd;
}

////////////////////////////////////////////////////////////////////////////////////

  void freeStructs(Command *head){
    int i;
    Command *temp;
    do{
      free(head->fileSource);
      free(head->fileDest);
      for(i = 0; head->args[i] != NULL; i++){
        free(head->args[i]);
      }
      temp = head->nextCommand;
      free(head);
      head = temp;
    }while(temp != NULL);
}
