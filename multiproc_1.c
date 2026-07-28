#include <stdio.h>
#include "util.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
  if (argc < 3){ //argv[0]: executable name, argv[1]: number of rep., argv[i]=strings | i>2
    printf("ERROR: the programm should take at least 3 arguments (executable name, number of rep and strings).");
    exit(-1);
  }
  int rep = atoi(argv[1]);
  if (rep <= 0) { 
    printf("ERROR: the number of repetitions should be a positive number. Current number given: %d.\n",rep);
    exit(-1);
  }
  struct sembuf up   = {0,  1, 0};
  struct sembuf down = {0, -1, 0};
  int sem = semget(IPC_PRIVATE, 1, 0600); //1 semaphore only 
  semctl(sem, 0, SETVAL, 1); //initialize sem=1 (mutual exclusion for using display(char *))
                          //entering the critical region (calling function display): DOWN(sem)
                          //exiting the critical region: UP(sem)
  int children = 0;
  for (int i=0; i < argc-2; i++){ //-2 because of the executable name and rep. number
    pid_t pid; 
    pid = fork(); 
    if (pid==0){ //child process
      for (int j=0; j < rep; j++){ 
        semop(sem, &down, 1); //entering the critical region
        display(argv[i+2]); //CRITICAL REGION
        semop(sem, &up, 1); //exiting critical region
      }
      exit(0);
    }
    if (pid<0){ //error
      perror("Fork error"); 
      continue;
    }
    children++;
  }
  int status;
  
  for (int i=0; i < children; i++){ //because we do not know for sure if a fork error might occur or not (we do not know if the processes are exactly a number of "argc-2")
    waitpid(-1, &status, 0); //-1 for any child process
  }
  semctl(sem, 0, IPC_RMID); 
  return 0;
}
