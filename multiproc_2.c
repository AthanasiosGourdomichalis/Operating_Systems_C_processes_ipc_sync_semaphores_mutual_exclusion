#include <stdio.h>
#include "util.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>

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
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//SEMAPHORE:
  int sem = semget(IPC_PRIVATE, 1, 0600); //1 semaphore (mutual exclusion for the shared memory counter and calling function init(void))
  semctl(sem, 0, SETVAL, 1); //initialize sem=1 
                             //the same semaphore will be used in the critical region of display(char *))
                             //entering the critical region (calling display or init and the increment of the shared counter): DOWN(sem)
                             //exiting the critical region: UP(sem)
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//SHARED MEMORY:  
  int shmid = shmget(IPC_PRIVATE, sizeof(int), 0600);//Create shared memory segment
  int *shared_counter = (int*) shmat(shmid, 0, 0); //Attach. 

  *shared_counter=0; //counts how many processes have executed the init() function, with values between [0, argc-2], therefore waiting for all of them to finish before executing the display() function 
  int children = 0; //counts how many processes have been constracted succesfully without a fork error occuring


  for (int i=0; i < argc-2; i++){ //-2 because of the executable name and rep. number
    char* str = argv[i+2];
    pid_t pid = fork(); 

    if (pid==0){ //child process

//--------------------------------------------------------------------------------------------------------------------------------------------------------      
//INIT():   

      semop(sem, &down, 1); //entering the critical region: DOWN(sem)
      init();               //exclusive access to the counter's critical region and the calling of init()
      (*shared_counter)++;  //critical region 
      semop(sem, &up, 1);   //exiting the critical region: UP(sem)

      while((*shared_counter)< argc-2); //Spin-lock (explained in README.pdf)

//--------------------------------------------------------------------------------------------------------------------------------------------------------      
//DISPLAY():
      
      for (int j=0; j < rep; j++){ 
        semop(sem, &down, 1); //entering the critical region
        display(str); //CRITICAL REGION
        semop(sem, &up, 1); //exiting critical region
      }
      exit(0);
    }
    if (pid<0){ //error
      perror("Fork error"); 
      exit(-1);
    }
    children++;
  }
  int status;

  for (int i=0; i < children; i++){ //because we do not know for sure if a fork error might occur or not (we do not know if the processes are exactly a number of "argc-2")
    waitpid(-1, &status, 0); //-1 for any child process
  }
//--------------------------------------------------------------------------------------------------------------------------------------------------------      
//CLEAN-UP()
  shmdt((void *) shared_counter); //detach from shared memory segment
  shmctl(shmid, IPC_RMID, 0); //destroy the shared memory segment
  semctl(sem, 0, IPC_RMID); //delete the semaphore array
  return 0;
}
