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

#define MAX_TIME_BETWEEN_PROCS_NS 500000000
#define MAX_TIME_BETWEEN_PROCS_SEC 1

int detachandremove(int shmid, void *shmaddr);
void displayhelpinfo();
void generaterandomtime(unsigned int *nano, unsigned int *sec, unsigned int maxnano, unsigned int maxsec);

/* Process Control Block structure */
struct pcb
{
  int cputime;    // Time on CPU
  int systemtime; // Time in system
  int recenttime; // Time used in recent burst
  int pid;        // Process ID
  int priority;   // Process priorty (0 = real-time process / 1 = user-process)
};

int main(int argc, char **argv)
{
  /* 0 = Available, 1 = Taken */
  int bitvector[18] = { 0 }; // Array used to track which PID / PCBs are taken in shared memory

  unsigned int delaynano; // Nanoseconds to delay before spawning new child
  unsigned int delaysec;  // Seconds to delay before spawning new child

  struct pcb *shmpcbs; // Array to store PCBs in shared memory
  int shmpcbsid;       // ID for shared memory PCBs
  key_t shmpcbskey;    // Key to use in creation of PCBs shared memory

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

  // Create dummy txt file to create a key with ftok
  system("touch keys.txt");

  /* * * SETUP SHARED MEMORY CLOCK * * */

  // Keys
  if ((clocknanokey = ftok("keys.txt", 'A')) == -1)
  {
    perror("ftok");
    exit(1);
  }
  
  if ((clockseckey = ftok("keys.txt", 'B')) == -1)
  {
    perror("ftok");
    exit(1);
  }


  // IDs
  if ((clocknanoid = shmget(clocknanokey, sizeof(unsigned int *), IPC_CREAT | 0660)) == -1)
  {
    perror("Failed to create shared memory segment.");
    return 1;
  }

  if ((clocksecid = shmget(clockseckey, sizeof(unsigned int *), IPC_CREAT | 0660)) == -1)
  {
    perror("Failed to create shared memory segment.");
    return 1;
  }

  // Attach to shared memory segments
  if ((clocknano = (unsigned int *) shmat(clocknanoid, NULL, 0)) == (void *) - 1)
  {
    perror("Failed to attach shared memory segment.");
    if (shmctl(clocknanoid, IPC_RMID, NULL) == -1)
      perror("Failed to remove memory segment.");
    return 1;
  }
  
  if ((clocksec = (unsigned int *) shmat(clocksecid, NULL, 0)) == (void *) - 1)
  {
    perror("Failed to attach shared memory segment.");
    if (shmctl(clocksecid, IPC_RMID, NULL) == -1)
      perror("Failed to remove memory segment.");
    return 1;
  }


  // Initialize Clock
  *clocknano = 0;
  *clocksec = 0;




  /* * * SETUP SHARED MEMORY PCB ARRAY * * */
  if ((shmpcbskey = ftok("keys.txt", 'C')) == -1)
  {
    perror("ftok");
    exit(1);
  }

  if ((shmpcbsid = shmget(shmpcbskey, sizeof(struct pcb) * 18, IPC_CREAT | 0660)) == -1)
  {
    perror("Failed to create shared memory segment.");
    return 1;
  }

  if ((shmpcbs = (struct pcb *) shmat(shmpcbsid, NULL, 0)) == (void *) - 1)
  {
    perror("Failed to attach shared memory segment.");
    if (shmctl(clocksecid, IPC_RMID, NULL) == -1)
      perror("Failed to remove memory segment.");
    return 1;
  }

  // EXAMPLES OF ACCESSING SHARED MEMORY PCBS
  // shmpcbs[0].cputime = 666;
  // shmpcbs[0].systemtime = 1000;

  // TESTING GENERATION OF RANDOM DELAY
  printf("nano delay: %u\n", delaynano);
  printf("sec delay: %u\n", delaysec);
  generaterandomdelay(&delaynano, &delaysec, MAX_TIME_BETWEEN_PROCS_NS, MAX_TIME_BETWEEN_PROCS_SEC);
  printf("nano delay: %u\n", delaynano);
  printf("sec delay: %u\n", delaysec);




  /* * * CLEAN UP * * */

  // Remove dummy txt file used to create keys
  system("rm keys.txt");

  // Detach and remove shared memory segments
  detachandremove(clocknanoid, clocknano);
  detachandremove(clocksecid, clocksec);
  detachandremove(shmpcbsid, shmpcbs);

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

// Generates a random number using 'maxnano' and 'maxsec' as the upper limit of number generated
// which will store an appropriate number in 'nano' and 'sec' for time between processes, time
// it takes to 'schedule' a process, and the time quantum a process is given
void generaterandomtime(unsigned int *nano, unsigned int *sec, unsigned int maxnano, unsigned int maxsec)
{
  int maxpossibletimenano = maxnano + (maxsec * 1000000000);
  int randomtime = random() % maxpossibletimenano;

  if (randomtime > 1000000000)
  {
    *sec = 1;
    *nano = randomtime - 1000000000;
  }
  else
  {
    *sec = 0;
    *nano = randomtime;
  }
}
