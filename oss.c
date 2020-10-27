#define _XOPEN_SOURCE
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int detachandremove(int shmid, void *shmaddr);
void displayhelpinfo();

int main(int argc, char **argv)
{
  int *clocknano;     // Shared memory clock nanoseconds
  int clocknanoid;    // ID for shared memory clock nanoseconds
  key_t clocknanokey; // Key to use in creation of nanoseconds shared memory
  int *clocksec;      // Shared memory clock seconds
  int clocksecid;     // ID for shared memory clock seconds
  key_t clockseckey;  // Key to use in creation of seconds shared memory

  int opt; // Used to parse command line options

  while ((opt = getopt(argc, argv, "h")) != -1)
  {
    switch (opt)
    {
      case 'h': // Help - describe how to run program and default values
        displayhelpinfo();
        exit(EXIT_SUCCESS);
        break;
      default:
        printf("OSS - Scheduler does not take any options to use from command line other than -h.\n");
        printf("Please use -h for help to see information about program.\n");
        exit(EXIT_FAILURE);
    }
  }

  printf("test\n");


  return 0;
}

void displayhelpinfo()
{
  printf("\nOperating System Simulator - Scheduler\n");
  printf("-------------------------\n\n");
  printf("Example command to run:\n\n");
  printf("./oss\n\n");
  printf("-------------------------\n");
  printf("(( INSERT PROGRAM DESCRIPTION HERE ))\n");
  printf("-------------------------\n");
  printf("Program options information:\n");
  printf("NONE - progam does not use any command line options\n");
  printf("-------------------------\n\n");
}

int detachandremove(int shmid, void *shmaddr)
{
  int error = 0;

  if (shmdt(shmaddr) == -1)
    error = errno;
  if ((shmctl(shmid, IPC_RMID, NULL) == -1) && !error)
    error = errno;
  if (!error)
  {
    printf("Successfully detached and removed the shared memory segment - id: %d\n", shmid);
    return 0;
  }
  errno = error;
  perror("Error: ");
  return -1;
}
