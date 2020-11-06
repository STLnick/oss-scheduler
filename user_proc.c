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

  int *clocknano;                  // Shared memory segment for clock nanoseconds
  int clocknanoid = atoi(argv[1]); // ID for shared memory clock nanoseconds segment
  int *clocksec;                   // Shared memory segment for clock seconds
  int clocksecid = atoi(argv[0]);  // ID for shared memory clock seconds segment

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


  // Receive a message from the queue
  if(msgrcv(msgid, &buf, sizeof(buf.mtext), 0, 0) == -1)
  {
    perror("user.c - msgrcv");
    exit(1);
  }


  // TODO: TESTING -- incrementing by a hard-coded value instead of randomly generated one
  *clocksec += 2;
  printf("Child incremented clocksec by 2!\n");


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
