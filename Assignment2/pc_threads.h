
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>


#define         NUMFLAVORS          4
#define         NUMSLOTS            600
#define         NUMCONSUMERS        50
#define         NUMPRODUCERS        30
#define         NUMDOZENS           200

struct  donut_ring {
	int     	flavor [NUMFLAVORS] [NUMSLOTS];
	int     	outptr [NUMFLAVORS];
	int         in_ptr [NUMFLAVORS];
	int         serial [NUMFLAVORS];
};


void	*sig_waiter ( void *arg );
void	*producer   ( void *arg );
void	*consumer ( void *arg) ;
void    sig_handler ( int );
