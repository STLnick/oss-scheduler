#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

int detach(int shmid, void *shmaddr);

#define PERMS 0644

/* Message Queue structure */
struct msgbuf {
  long mtype;
  char mtext[100];
};

/* Process Control Block structure */
struct pcb {
  int cputime;    // Time on CPU
  int systemtime; // Time in system
  int recenttime; // Time used in recent burst
  int pid;        // Process ID
  int priority;   // Process priorty (0 = real-time process / 1 = user-process)
};

int main (int argc, char **argv)
{
  printf("::Begin:: Child Process\n");

  int tq;      // Time Quantam allowed by scheduler
  int tq_used; // Portion of Time Quantam used in sec/nanosec

  int pcbindex = atoi(argv[2]); // Index of PCB for process in shared memory array

  int *clocknano;                  // Shared memory segment for clock nanoseconds
  int clocknanoid = atoi(argv[1]); // ID for shared memory clock nanoseconds segment
  int *clocksec;                   // Shared memory segment for clock seconds
  int clocksecid = atoi(argv[0]);  // ID for shared memory clock seconds segment

  int len;           // Holds length of mtext to use in msgsnd invocation
  struct msgbuf buf; // Struct for message queue
  int msgid;         // ID for message queue
  key_t msgkey;      // Key for message queue

  struct pcb *shmpcbs;          // Array to store PCBs in shared memory
  int shmpcbsid = atoi(argv[3]); // ID for shared memory PCBs

  // TODO: Generate random number to 'choose' outcome:
  //         1. Terminate - send msg with time ran before terminating
  //         2. Run for full allotted time quantum - send msg appropriately
  //         3. I/O interrupt - send msg stating interrupted to be put in blocked queue and time ran before blocked


  /* * MESSAGE QUEUE * */
  // Retrieve key established in oss.c
  if ((msgkey = ftok("keys.txt", 'C')) == -1)
  {
    perror("user_proc.c - ftok");
    exit(1);
  }

  // Access message queue using retrived key
  if ((msgid = msgget(msgkey, PERMS)) == -1)
  {
    perror("user_proc.c - msgget");
    exit(1);
  }


  /* * SHARED MEMORY CLOCK * */
  if ((clocksec = (int *) shmat(clocksecid, NULL, 0)) == (void *) -1)
  {
    perror("Failed to attach to memory segment.");
    return 1;
  }

  if ((clocknano = (int *) shmat(clocknanoid, NULL, 0)) == (void *) -1)
  {
    perror("Failed to attach to memory segment.");
    return 1;
  }


  /* * SHARED MEMORY PCB ARRAY * */
  if ((shmpcbs = (struct pcb *) shmat(shmpcbsid, NULL, 0)) == (void *) - 1)
  {
    perror("Failed to attach shared memory segment.");
    if (shmctl(shmpcbsid, IPC_RMID, NULL) == -1)
      perror("Failed to remove memory segment.");
    return 1;
  }

  int loop = 2;

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* * * * *                     MAIN LOOP                       * * * * */
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  while (loop != 0) { // TESTING WITH A SIMPLE VARIABLE

  // RECEIVE message from the queue
  if(msgrcv(msgid, &buf, sizeof(buf.mtext), pcbindex, 0) == -1)
  {
    perror("user.c - msgrcv");
    exit(1);
  }

  // Extract time quantam sent from message sent by 'scheduler'
  tq = (int) atoi(buf.mtext);





  /* 0-5 = terminate
   * 6-25 = ran for partial tq (blocked now)
   * 26-99 = ran for FULL tq
  */ 

  // Seed random number generator
  srand((unsigned int) getpid());

  int randnum = rand() % 100;

  if (randnum > 0 && randnum <= 5)
  {
    // TERMINATE: ran for partial time now stopping
    tq_used = -(rand() % tq);
  }
  else if (randnum > 5 && randnum <= 25)
  {
    // RAN PARTIALLY - BLOCKED NOW
    tq_used = rand() % tq;
  }
  else if (randnum > 25 && randnum <= 99)
  {
    // RAN FOR ALL OF TQ
    tq_used = tq;
  }
  else
  {
    printf("Something went wrong.....");
    exit(1);
  }

  // Convert time quantam used to a string
  char strtq_used[100+1] = {'\0'}; // Create string from shared memory clock nanoseconds id
  sprintf(strtq_used, "%d", tq_used); 


  // Send message
  // TODO: Make this message tell ./oss if 
  //         1) Terminating
  //         2) Ran for whole time quantam (move down level in queues)
  //         3) Ran for PART of time quantam (put in blocked queue)


  // Setup message to send
  buf.mtype = 99;
  strcpy(buf.mtext, strtq_used);
  len = strlen(buf.mtext);

  // SEND message
  if (msgsnd(msgid, &buf, len+1, 0) == -1)
    perror("msgsnd:");

    --loop;
  } // END of tester while loop


  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  /* * * * *                     END MAIN LOOP                   * * * * */
  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */




  /* * CLEAN UP * */
  // Detach from all shared memory segments
  detach(clocksecid, clocksec);
  detach(clocknanoid, clocknano);
  detach(shmpcbsid, shmpcbs);

  printf("::END:: Child Process\n");

  return 0;
}

int detach(int shmid, void *shmaddr)
{
  int error = 0;

  if (shmdt(shmaddr) == -1)
    error = errno;
  if (!error)
  {
    //printf("CHILD: Successfully detached from the shared memory segment - id: %d\n", shmid);
    return 0;
  }
  errno = error;
  perror("Error: ");
  return -1;
}
