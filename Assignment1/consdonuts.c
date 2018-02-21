/*Reference from the Proferors help File */

#include "donuts.h"

 int shmid, semid[3];


int main(int argc, char *argv[])
{
  int out [NUMFLAVORS][12];

  int i,j,k,s,q;
  struct donut_ring *shared_ring;
  struct timeval randtime, now;
  unsigned short xsub1[3];


   /* Start Getting shared memory */
   if((shmid = shmget (SHMKEY, sizeof(struct donut_ring),
		            IPC_CREAT | 0600)) == -1){
	perror("shared get failed: ");
	exit(1);
   }

   /* Now its Start Attaching shared memory */
   if((shared_ring = (struct donut_ring *) 
		   shmat (shmid, (const void *)0, 0)) == -1){
	perror("shared attach failed: ");
	exit(2);
   }

   /* Semaphore Allocation is been Started*/
   for(i=0; i<NUMSEMIDS; i++){
      if ((semid[i] = semget (SEMKEY+i, NUMFLAVORS, 
			 IPC_CREAT | 0600)) == -1){
	perror("semaphore allocation failed: ");
	exit(3);
      }
   }


   /* Now we are Getting Random number */
   gettimeofday (&randtime, (struct timezone *)0);

   xsub1[0] = (ushort) randtime.tv_usec;
   xsub1[1] = (ushort)(randtime.tv_usec >> 16);
   xsub1[2] = (ushort)(getpid());

	/* Now we are Geeting Random Number */
   for( i = 0; i < no_of_dozens; i++ ) {

      for(k = 0; k < NUMFLAVORS; k++){
       for(s = 0; s < 12; s++){
         out [k][s] = 0;
       }
      }

/* The P operation wastes time or sleep until a resource controlled by the semaphore becomes
	available, at which time the resource is immediatly claimed.
	The V operation is the inverse; it makes a resoureces available again after the process
	has finished using it.*/

      for( q = 0; q < 12; q++ ) {
         j=nrand48(xsub1) & 3;

         p(semid[CONSUMER], j);
         p(semid[OUTPTR], j);

         out[j][q] = shared_ring->flavor[j][shared_ring->outptr[j]];
         shared_ring->outptr[j] = (shared_ring->outptr[j] + 1) % NUMSLOTS;

         v(semid[OUTPTR], j);
         v(semid[PROD], j);
      }
     gettimeofday (&now, (struct timezone *)0);
     printf("Process ID : %d, Time : %d , Dozen #  %d\n",
                getpid(), (ushort)now.tv_usec, i+1);
     printf("*******************************************");
     printf("\n   PLAIN   JELLY   COCONUT   HONEY-DIP\n");
      for(s = 0; s < 12; s++){
       for(k = 0; k < NUMFLAVORS; k++){
         if(out[k][s] != 0)
             printf("   %3d", out[k][s]);
         else
             printf("         ");
       }
       printf("\n");
      }
	printf("*******************************************");
      printf("\n");
   }
}
