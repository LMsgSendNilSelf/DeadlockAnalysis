/*Reference from the Proferors help File */


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
       int val;  /* Values for SETVAL */
       struct semid_ds *buf; /* Buffer for IPC_STAT & IPC_SET */
       u_short *array; /* Array for GETALL & SETALL */
 };

int p (int semidgroup, int donut_type)
{
	struct sembuf semopbuf; /*Struct in <Sys/sem.h>*/
	semopbuf.sem_num = donut_type;
        semopbuf.sem_op = (-1);     /* -1 is a P operation */
	semopbuf.sem_flg = 0;

	if(semop (semidgroup, &semopbuf,1) == -1){
		perror ("p operation failed: ");
		return (-1);
	}
	return (0);
}
	
int   v (int semidgroup, int donut_type)
{
	struct sembuf semopbuf;

	semopbuf.sem_num = donut_type;
        semopbuf.sem_op = (+1);     /* +1 is a V operation */
	semopbuf.sem_flg = 0;

	if(semop(semidgroup, &semopbuf,1) == -1){
		perror("p operation failed: ");
		return (-1);
	}
	return (0);
}

int   semsetall (int semgroup, int number_in_group, 
			      int set_all_value)
{
	int	i, j, k;
	union semun sem_ctl_un;

	sem_ctl_un.val = set_all_value;

	for (i = 0; i < number_in_group; i++){
	  if(semctl(semgroup, i, SETVAL, sem_ctl_un) == -1){
		perror("semset failed");
		return (-1);
	  }
	}
	return (0);
}
