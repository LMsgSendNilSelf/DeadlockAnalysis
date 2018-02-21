/***** Reference from the Professors help File*/

#include "donuts.h"

 int shmid, semid[3];
 void sig_handler (int);


int main(int argc, char *argv[])
{
  int in_ptr [NUMFLAVORS];
  int serial [NUMFLAVORS];
  int i,j,k;
  struct donut_ring *shared_ring;
  struct timeval randtime;
  unsigned short xsub1[3]; /*Use microsecond component for Unniquenss*/

	/** Only need to catch some signal **/
  sigset_t   mask_sigs;
  struct sigaction  new;
  int  nsigs;
  int sigs [] = {SIGHUP, SIGINT, SIGQUIT, SIGBUS, 
                        SIGTERM, SIGSEGV, SIGFPE};

   /* producer initializes serial counters and in-pointers  */
   for(i = 0; i < NUMFLAVORS; i++){
    in_ptr [i] = 0;
    serial [i]  = 0;
   }

   /* Start Catching some signals */
   nsigs = sizeof (sigs) / sizeof (int);

   sigemptyset(&mask_sigs);

   for(i=0; i< nsigs; i++)
       sigaddset(&mask_sigs, sigs [i]);

   for(i = 0; i <  nsigs; i++){
      new.sa_handler = sig_handler;
      new.sa_mask = mask_sigs;
      new.sa_flags = 0;
      if(sigaction(sigs [i], &new, NULL) == -1){
         perror("can't set signals: ");
         exit(1);
      }
   }

   /* Start Getting shared memory */
   if((shmid = shmget (SHMKEY, sizeof(struct donut_ring),
		            IPC_CREAT | 0600)) == -1){
	perror("shared get failed: ");
	exit(1);
   }

   /* Start Attaching shared memory */
   if((shared_ring = (struct donut_ring *) 
		   shmat (shmid, (const void *)0, 0)) == -1){
	perror("shared attach failed: ");
	sig_handler(-1);
   }


   /* Semaphore Allocation is now started*/
   for(i=0; i<NUMSEMIDS; i++){
      if ((semid[i] = semget (SEMKEY+i, NUMFLAVORS, 
			 IPC_CREAT | 0600)) == -1){
	perror("semaphore allocation failed: ");
	sig_handler(-1);
      }
   }

   /* Use semetall utility to set initializing semaphore values */

   if(semsetall (semid [PROD], 
		   NUMFLAVORS, 	NUMSLOTS) == -1){
         perror("semsetsall failed: ");
         sig_handler(-1);
   }
   if(semsetall (semid [CONSUMER], 
		   NUMFLAVORS, 	0) == -1){
         perror("semsetsall failed: ");
         sig_handler(-1);
   }
   if(semsetall (semid [OUTPTR], 
		   NUMFLAVORS, 	1) == -1){
         perror("semsetsall failed: ");
         sig_handler(-1);
   }

   /* Getting the Random number */
   gettimeofday (&randtime, (struct timezone *)0);

   xsub1[0] = (ushort) randtime.tv_usec;
   xsub1[1] = (ushort)(randtime.tv_usec >> 16);
   xsub1[2] = (ushort)(getpid());

/* Producer Start Producing Numbers Of Donuts */

   while(1){
     j=nrand48(xsub1) & 3;

     p(semid[PROD], j);
     p(semid[OUTPTR], j);
    
     serial[j]++; 

     shared_ring->flavor[j][in_ptr[j]] = serial[j];
     in_ptr[j] = (in_ptr[j] + 1) % NUMSLOTS;

     v(semid[OUTPTR], j);
     v(semid[CONSUMER], j);

     printf("donut type = %d, In ptr =  %d, serial ptr =  %d\n",
                j, in_ptr[j], serial[j]);

   }

}

	/* Producer Signal handler */
void    sig_handler(int sig)
{
	int	i,j,k;

	printf("In signal handler with signal # %d\n",sig);

	if(shmctl(shmid, IPC_RMID, 0) == -1){
		perror("handler failed shm RMID: ");
	}
	for(i = 0; i < NUMSEMIDS; i++){
	  if(semctl (semid[i], 0, 
 			    IPC_RMID, (union semun)0) == -1){
		perror("handler failed sem RMID: ");
	  }
	}
        exit(5);
}