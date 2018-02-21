#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/signal.h>

#define		SEMKEY			(key_t)954632719
#define		SHMKEY			(key_t)987573204
#define		NUMFLAVORS	 	4
#define		NUMSLOTS       	40
#define		NUMSEMIDS	 	3
#define		PROD		 	0
#define		CONSUMER	 	1
#define		OUTPTR		 	2
#define         no_of_dozens              10


struct	donut_ring{
	int	flavor  [NUMFLAVORS]  [NUMSLOTS];
	int	outptr  [NUMFLAVORS];
};

union semun {
    int val;
    struct semid_ds *buf;
   u_short *array;
 };

extern int p (int, int);
extern int v (int, int);
extern int semsetall (int, int, int);