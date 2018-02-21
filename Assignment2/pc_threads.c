#include "pc_threads.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

//#define ushort short
typedef unsigned short ushort;


struct donut_ring 	shared_ring;
int			space_count [NUMFLAVORS];
int			donut_count [NUMFLAVORS];
pthread_mutex_t		prod [NUMFLAVORS];
pthread_mutex_t		cons [NUMFLAVORS];
pthread_cond_t		prod_cond [NUMFLAVORS];
pthread_cond_t		cons_cond [NUMFLAVORS];
pthread_t		thread_id [NUMPRODUCERS+NUMCONSUMERS], sig_wait_id;


ushort get_time_microseconds()
{
    struct timeval randtime;
    gettimeofday(&randtime, (struct timezone *) 0);
    return randtime.tv_usec;
}





int  main ( int argc, char *argv[] )
{
	int			i, j, k, nsigs;
	struct timeval		randtime, first_time, last_time;
	struct sigaction	new_act;
	int			arg_array[NUMPRODUCERS+NUMCONSUMERS];
	sigset_t  		all_signals;
	int sigs[] 		= { SIGBUS, SIGSEGV, SIGFPE };

	pthread_attr_t 		thread_attr;
	struct sched_param  	sched_struct;
	



/**********************************************************************/
/* INITIAL TIMESTAMP VALUE FOR PERFORMANCE MEASURE                    */
/**********************************************************************/

        gettimeofday (&first_time, (struct timezone *) 0 );
        for ( i = 0; i < NUMPRODUCERS+NUMCONSUMERS; i++ ) {
		arg_array [i] = i+1;	/** SET ARRAY OF ARGUMENT VALUES **/
        }

/**********************************************************************/
/* GENERAL PTHREAD MUTEX AND CONDITION INIT AND GLOBAL INIT           */
/**********************************************************************/

	for ( i = 0; i < NUMFLAVORS; i++ ) {
		pthread_mutex_init ( &prod [i], NULL );
		pthread_mutex_init ( &cons [i], NULL );
		pthread_cond_init ( &prod_cond [i],  NULL );
		pthread_cond_init ( &cons_cond [i],  NULL );
		shared_ring.outptr [i]		= 0;
		shared_ring.in_ptr [i]		= 0;
		shared_ring.serial [i]		= 0;
		space_count [i]			= NUMSLOTS;
		donut_count [i]			= 0;
	}



/**********************************************************************/
/* SETUP FOR MANAGING THE SIGTERM SIGNAL, BLOCK ALL SIGNALS           */
/**********************************************************************/

	sigfillset (&all_signals );
	nsigs = sizeof ( sigs ) / sizeof ( int );
	for ( i = 0; i < nsigs; i++ )
       		sigdelset ( &all_signals, sigs [i] );
	sigprocmask ( SIG_BLOCK, &all_signals, NULL );
	sigfillset (&all_signals );
	for( i = 0; i <  nsigs; i++ ) {
      		new_act.sa_handler 	= sig_handler;
      		new_act.sa_mask 	= all_signals;
      		new_act.sa_flags 	= 0;
      		if ( sigaction ( sigs[i], &new_act, NULL ) == -1 ){
         			perror("can't set signals: ");
         			exit(1);
      		}
   	}

	//printf ( "just before threads created\n" );


/*********************************************************************/
/* CREATE SIGNAL HANDLER THREAD, PRODUCER AND CONSUMERS              */
/*********************************************************************/

        if ( pthread_create (&sig_wait_id, NULL,
					sig_waiter, NULL) != 0 ){

                printf ( "pthread_create failed " );
                exit ( 3 );
        }

       pthread_attr_init ( &thread_attr );
       pthread_attr_setinheritsched ( &thread_attr,
		PTHREAD_INHERIT_SCHED );

      #ifdef  GLOBAL
        sched_struct.sched_priority = sched_get_priority_max(SCHED_OTHER);
        pthread_attr_setinheritsched ( &thread_attr,
		PTHREAD_EXPLICIT_SCHED );
        pthread_attr_setschedpolicy ( &thread_attr, SCHED_OTHER );
        pthread_attr_setschedparam ( &thread_attr, &sched_struct );  
        pthread_attr_setscope ( &thread_attr,
		PTHREAD_SCOPE_SYSTEM );
     #endif

    
    
    for ( i = 0; i < NUMCONSUMERS; i++) {
		if ( pthread_create ( &thread_id [i], &thread_attr,
                             consumer, ( void * )&arg_array [i]) != 0 ){
			printf ( "pthread_create failed" );
			exit ( 3 );
		}
	}
    
    
	for ( ; i < NUMPRODUCERS+NUMCONSUMERS; i++ ) {
		if ( pthread_create ( &thread_id [i], &thread_attr,
				producer, ( void * )&arg_array [i]) != 0 ){
			printf ( "pthread_create failed" );
			exit ( 3 );
		}
	}

	//printf ( "just after threads created\n" );




/*********************************************************************/
/* WAIT FOR ALL CONSUMERS TO FINISH, SIGNAL WAITER WILL              */
/* NOT FINISH UNLESS A SIGTERM ARRIVES AND WILL THEN EXIT            */
/* THE ENTIRE PROCESS....OTHERWISE MAIN THREAD WILL EXIT             */
/* THE PROCESS WHEN ALL CONSUMERS ARE FINISHED                       */
/*********************************************************************/

	for ( i = 0; i < NUMCONSUMERS; i++ )
             		pthread_join ( thread_id [i], NULL ); 

/*****************************************************************/
/* GET FINAL TIMESTAMP, CALCULATE ELAPSED SEC AND USEC           */
/*****************************************************************/


        	gettimeofday (&last_time, ( struct timezone * ) 0 );
        	if ( ( i = last_time.tv_sec - first_time.tv_sec) == 0 )
			j = last_time.tv_usec - first_time.tv_usec;
        	else{
			if ( last_time.tv_usec - first_time.tv_usec < 0 ) {
				i--;
				j = 1000000 + 
				   ( last_time.tv_usec - first_time.tv_usec );
            	} else {
				j = last_time.tv_usec - first_time.tv_usec; }
        	       }
	printf ( "\n\nElapsed consumer time is %d sec and %d usec\n", i, j );

	printf ( "\n\nALL CONSUMERS FINISHED, KILLING  PROCESS\n\n" );
	exit ( 0 );
}



/*********************************************/
/* INITIAL PART OF PRODUCER.....             */
/*********************************************/

void	*producer ( void *arg )
{
	int			i, j, k,id;
    int         donut_flavor_count,donut_flavor_buffer;
	unsigned short 	xsub [3];
	struct timeval 	randtime;
    id = *( int * ) arg;

	gettimeofday ( &randtime, ( struct timezone * ) 0 );
	xsub [0] = ( ushort )randtime.tv_usec;
	xsub [1] = ( ushort ) ( randtime.tv_usec >> 16 );
	xsub [2] = ( ushort ) ( pthread_self );

	while ( 1 )
    {
	  j = nrand48 ( xsub ) & 3;
	  pthread_mutex_lock ( &prod [j] );
           while ( space_count [j] == 0 )
            {
               // printf("\nProducer %d waiting for donut type %d",(id-NUMCONSUMERS),j);
                pthread_cond_wait ( &prod_cond [j], &prod [j] );
                //printf("\nProducer %d finished waiting for donut type %d",(id-NUMCONSUMERS),j);
            }
          
          	shared_ring.serial[j]=shared_ring.serial[j]+1;
            donut_flavor_count=shared_ring.serial[j];
            donut_flavor_buffer=shared_ring.in_ptr[j];
        
        
            //printf("%d",shared_ring.serial[j]);
            shared_ring.flavor[j][donut_flavor_buffer] = donut_flavor_count;
			//printf("\nProducer : %d Produced : %d",(id-NUMCONSUMERS),shared_ring.flavor[j][donut_flavor_buffer]);
            space_count[j]=space_count[j]-1;
        
        	shared_ring.in_ptr[j] = ((shared_ring.in_ptr[j]+1) % NUMSLOTS);
        	//(shared_ring.in_ptr[j])++;
			
			pthread_mutex_unlock ( &prod [j] );
			
			pthread_mutex_lock ( &cons[j] );
			donut_count[j]=donut_count[j]+1;
			pthread_mutex_unlock ( &cons [j] );
			pthread_cond_signal(&cons_cond[j]);
	}
	return NULL;
}



/*********************************************/
/* ON YOUR OWN FOR THE CONSUMER.........     */
/*********************************************/



void    *consumer ( void *arg )
{
	int     		i, j, k, m, id;
    int             donuts,out_ptr;
	int output_array[NUMFLAVORS][12];
	FILE *fp;
    char fname[64];
    pid_t consumerpid;
    int doz_id;
    int dozen_count=1;
    id = *( int * ) arg;
    
    time_t t;
    char  buf[80];
    struct tm *ts;
    
    
	for(i=0;i<NUMFLAVORS;i++)
	{
		for(j=0;j<12;j++)
		{
			output_array[i][j]=0;
		}
	}
    
    sprintf(fname,"Consumer_%d",id);
    		
    
    if((consumerpid=getpid())==-1)
    {
        printf("\nError! Could not retrieve Consumer PID");
        return 0;
    }
    
	unsigned short 	xsub [3];
	struct timeval 	randtime;
	
	gettimeofday ( &randtime, ( struct timezone * ) 0 );
	xsub [0] = ( ushort )randtime.tv_usec;
	xsub [1] = ( ushort ) ( randtime.tv_usec >> 16 );
	xsub [2] = ( ushort ) ( pthread_self );

    while(dozen_count<=(NUMDOZENS+1))
    {
           for( m = 0; m < 12; m++ )
          {

               j = nrand48( xsub ) & 3;
               pthread_mutex_lock ( &cons[j] );
               while(donut_count[j]==0)
              {
                    //printf("\nConsumer %d waiting for donut type %d",id,j);
               		pthread_cond_wait ( &cons_cond [j], &cons [j] );
                    //printf("\nConsumer %d finished waiting for donut type %d",id,j);

              }
              
              out_ptr=shared_ring.outptr[j];
              donuts=shared_ring.flavor[j][out_ptr];
              output_array[j][m] = donuts;
              
          //printf("\nConsumer: %d Consumed : %d",id,output_array[j][m]);
              
            shared_ring.outptr[j] = ((shared_ring.outptr[j] + 1) % NUMSLOTS);
            //shared_ring.outptr[j] = shared_ring.outptr[j] % NUMSLOTS;
			donut_count[j]=donut_count[j]-1;
			pthread_mutex_unlock ( &cons[j] );
			
			pthread_mutex_lock ( &prod[j] );
            space_count[j]=space_count[j]+1;
            pthread_mutex_unlock ( &prod[j] );
			pthread_cond_signal(&prod_cond[j]);

         }

/*****************************************************************/
/* USING A MICRO-SLEEP AFTER EACH DOZEN WILL ALLOW ANOTHER       */
/* CONSUMER TO START RUNNING, PROVIDING DESIRED RANDOM BEHVIOR   */
/*****************************************************************/

        sched_yield();
         //usleep(1000); /* sleep 1 ms */
        
        
        
        
        
        
        if(dozen_count<(NUMDOZENS+1))
        {
            fp=fopen(fname,"a");
            
            t=time(0);
            ts= localtime(&t);
            strftime(buf, sizeof(buf),  " %H:%M:%S ", ts);
            
            fprintf( fp,
                    "          _________________________________________________________\n");
            fprintf( fp, "Consumer Process ID: %d, \t Time : %s.%5d, \t Dozen #:  %d\n",
                    consumerpid, buf, get_time_microseconds(), dozen_count);
            fprintf( fp,
                    "          _________________________________________________________\n");
            fprintf( fp,
                    "%15s %15s %15s %15s \n", "PLAIN", "JELLY",  "COCONUT", "HONEY-DIP\n");
            fprintf( fp,
                    "          _________________________________________________________\n");
            
            for(k=0;k<12;k++)
            {
                fprintf(fp,"        ");
                for(j=0;j<NUMFLAVORS;j++)
                {
                    if(output_array[j][k]!=0)
                    {
                        fprintf(fp,"%11d",output_array[j][k]);
                    }
                    else
                    {
                        fprintf(fp,"%15s"," ");
                    }
                }
                fprintf(fp,"\n");
            }
            fprintf( fp,
                    "                                                              \n");
            fprintf( fp,
                    "          =========================================================\n");
            fprintf( fp, "\n\n");
            for(k=0; k< NUMFLAVORS; k++)
                for(j=0; j< 12; j++)
                    output_array[k][j] = 0;
            fclose(fp);
        }
        dozen_count++;
    }
    //printf("\nConsumer ID %d finished",id);
    pthread_exit(NULL);
}

/***********************************************************/
/* PTHREAD ASYNCH SIGNAL HANDLER ROUTINE...                */
/***********************************************************/

void    *sig_waiter ( void *arg )
{
	sigset_t	sigterm_signal;
	int		signo;

	sigemptyset ( &sigterm_signal );
	sigaddset ( &sigterm_signal, SIGTERM );
	sigaddset ( &sigterm_signal, SIGINT );


	if (sigwait ( &sigterm_signal, & signo)  != 0 ) {
		printf ( "\n  sigwait ( ) failed, exiting \n");
		exit(2);
	}
	printf ( "\nProcess going down on SIGNAL (number %d)\n\n", signo );
	exit ( 1 );
	return NULL;
}


/**********************************************************/
/* PTHREAD SYNCH SIGNAL HANDLER ROUTINE...                */
/**********************************************************/

void	sig_handler ( int sig )
{
	pthread_t	signaled_thread_id;
	int		i, thread_index;

	signaled_thread_id = pthread_self ( );
	for ( i = 0; i < (NUMCONSUMERS + NUMPRODUCERS ); i++) {
		if ( signaled_thread_id == thread_id [i] )  {
				thread_index = i;
				break;
		}
	}
	printf ( "\nThread %d took signal # %d, PROCESS HALT\n",
				thread_index, sig );
	exit ( 1 );
}
