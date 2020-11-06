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

struct msgbuf {
  long mtype;
  char mtext[100];
};

int main (int argc, char **argv)
{
  printf("::Begin:: Child Process\n");

  unsigned int tq;      // Time Quantam allowed by scheduler
  unsigned int tq_used; // Portion of Time Quantam used in sec/nanosec

  int *clocknano;                  // Shared memory segment for clock nanoseconds
  int clocknanoid = atoi(argv[1]); // ID for shared memory clock nanoseconds segment
  int *clocksec;                   // Shared memory segment for clock seconds
  int clocksecid = atoi(argv[0]);  // ID for shared memory clock seconds segment

  int len;           // Holds length of mtext to use in msgsnd invocation
  struct msgbuf buf; // Struct for message queue
  int msgid;         // ID for message queue
  key_t msgkey;      // Key for message queue


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


  /* * SHARED MEMORY * */
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


  // RECEIVE message from the queue
  if(msgrcv(msgid, &buf, sizeof(buf.mtext), 1, 0) == -1)
  {
    perror("user.c - msgrcv");
    exit(1);
  }

  // Extract time quantam sent from message sent by 'scheduler'
  tq = (unsigned int) atoi(buf.mtext);



  // Send message
  // TODO: Make this message tell ./oss if 
  //         1) Terminating
  //         2) Ran for whole time quantam (move down level in queues)
  //         3) Ran for PART of time quantam (put in blocked queue)



  // 0-5 = terminate
  // 6-25 = ran for partial tq (blocked now)
  // 26-99 = ran for FULL tq

  // Seed random number generator
  srand((unsigned int) getpid());

  // May need to provide a seed somehow to ensure different rands from processes.....  
  int randnum = rand() % 100;
  
  // TESTING
  printf("%d randnum: %d\n", getpid(), randnum);

  if (randnum > 0 && randnum <= 5)
  {
    // TODO: TERMINATE
    printf("terminate...\n");
    tq_used = 0;
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
  sprintf(strtq_used, "%u", tq_used); 

  // Setup message to send
  buf.mtype = 99;
  strcpy(buf.mtext, strtq_used);
  len = strlen(buf.mtext);

  // SEND message
  if (msgsnd(msgid, &buf, len+1, 0) == -1)
    perror("msgsnd:");




  /* * CLEAN UP * */
  // Detach from all shared memory segments
  detach(clocksecid, clocksec);
  detach(clocknanoid, clocknano);

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
