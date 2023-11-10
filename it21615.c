/******************************************************************
 ***                                                            ***
 *** Harokopio University - Informatics & Telematics Dpt        ***
 *** Operating Systems - Assignment 2 - UNIX C programming      ***
 ***                                                            ***
 *** Georgiou Antonios (it21615)                                ***
 *** 30/01/2023                                                 ***
 ***                                                            ***
 *** compile with: gcc -pthread it21780.c -o it21780            ***
 ***                                                            ***
 *****************************************************************/
 
#include <stdio.h>        // printf()
#include <stdlib.h>       // exit(), malloc(), free()
#include <unistd.h>       // fork(), sleep()
#include <semaphore.h>    // semaphore functions
#include <fcntl.h>        // for O_* constants (O_CREAT, O_EXEC)
#include <errno.h>        // errno, ECHILD
#include <sys/stat.h>     // for mode constants
#include <pthread.h>      // pthread functions and data structures
#include <signal.h>       // handling SIGINT & SIGTERM
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int   ChildWriteProcess(char*, FILE**);    
int   ChildReadProcess(char*, FILE**);     
void* ThreadIndexer(void* data);

   struct threadata              // structure that will pass data to each thread
     {
       char*  file_name;         // file that will be read from each thread 
       FILE** readFile;          // file reference
       int    start_index;       // first character position to read from file per thread  
       int    end_index;         // last character position to read from file per thread
       int    *chridx;           // chars indexer array to store the final index statistics
     };

struct threadata thread_data_array[4]; // since we want to fire 4 threads

// Thread locking - Declaring mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void sigint_handler(int signum) 
{ 
	//Handler for SIGINT AND SIGTERM signals
	
	char choice;
		
	printf("\n Are you sure you want to EXIT? [Y/N] ");
	
	choice = getchar();
    if (choice == '\n') choice = getchar();
    while(choice != 'n' && choice != 'N' && choice != 'y' && choice != 'Y')
    {
        printf(" invalid input, enter the choice (Y/N) please : ");
        choice = getchar();
        if (choice == '\n') choice = getchar();
    }
	
	if (choice == 'n' || choice == 'N') 
	{
		//Reset handler to catch SIGINT next time.
        signal(SIGINT, sigint_handler);   
        fflush(stdout); // empty OUTPUT buffer
	}
	else
	{
		exit(0);
	}	    
}
 
int main() 
{  
    sem_t *sem_child_Read;  // semaphore for Read process
	
	printf("\n");
	printf("**************************************************\n");
	printf("***                                            ***\n");	
    printf("*** Character Indexer Program                  ***\n");
	printf("***                                            ***\n");	
    printf("*** Antonios Georgiou  (it21615)               ***\n");    
    printf("***                                            ***\n");	
    printf("**************************************************\n\n");
    
    signal(SIGINT, sigint_handler);  // register a SIGINT handler routine to catch CTRL-C signal for exiting
    signal(SIGTERM, sigint_handler); // register a SIGTERM handler routine to catch process termination signal
    
    // create a semaphore: value=1
    // If O_CREAT is specified, then the semaphore is created if it does not already exist
    // If both O_CREAT and O_EXCL are specified, then an error is returned if a semaphore with the given name already exists
    // 0644: (6) Owner-Read/Write, (4) Other Group Users-Read, (4) Any one Else-Read    
    sem_child_Read = sem_open("Read_child_semaphore", O_CREAT|O_EXCL, 0644, 1);
           
    pid_t  pidCh1;
    pid_t  pidCh2;
    pid_t  pidEnd1;
    pid_t  pidEnd2;
    int    status;
    int    delay_sec;
            
    FILE*  fp;
    
    delay_sec = 1;  // how many secs program execution will be delayed so processes can be synchronized properly

     pidCh1 = fork();   // create Writer Process
     sleep (delay_sec);  
     if (pidCh1 < 0)
      {
        sem_unlink ("Read_child_semaphore");   
        sem_close(sem_child_Read);                        
        fprintf(stderr, "Child Write Fork Failed");     
      }  
     else if (pidCh1 == 0)
      { 
          sem_wait(sem_child_Read);	   // decrease 1 from semaphore, next attempt will be locked by any process
          sleep (delay_sec);        
          if (ChildWriteProcess("data.txt",&fp) != 0)
          {
			 printf("\n %s : ERROR opening file: 'data.txt'. \n", __FUNCTION__); // __FUNCTION__ is a GCC identifier that will print the calling function name
			 sem_unlink ("Read_child_semaphore");   
             sem_close(sem_child_Read); 
			 exit (1); 
		  }          
          sleep (delay_sec);
          sem_post(sem_child_Read);  // release semaphore for ChildReadProcess  
          sleep (delay_sec);       
          exit(0);
      }   
     else
      {                     
          pidCh2 = fork();    // create Reader Process  
          sleep (delay_sec);    
          if (pidCh2 < 0)
           {
             sem_unlink ("Read_child_semaphore");   
             sem_close(sem_child_Read);                        			   
             fprintf(stderr, "Child Read Fork Failed");     
           }  
          else if (pidCh2 == 0)
           {        			               
			 sem_wait(sem_child_Read);	// since Semaphore was at value = 0, from ChildWriteProcess, now ChildReadProcess is blocked, waiting for ChildWriteProcess to finish
			 sleep (delay_sec);				 			 
			 if (ChildReadProcess("data.txt",&fp) != 0)
			  {
				 printf("\n %s : ERROR opening file: 'data.txt'. \n", __FUNCTION__); // __FUNCTION__ is a GCC identifier that will print the calling function name
				 sem_unlink ("Read_child_semaphore");   
				 sem_close(sem_child_Read); 
				 exit (1); 
			  }          
             sleep (delay_sec);	
             exit(0);	      
           }   
          else
           {             
             /* wait for all children processes to exit */                               
             do {
				  pidEnd1 = waitpid(pidCh1, &status, WUNTRACED | WCONTINUED);				    
				  if (pidEnd1 == -1) 
				   {                     
                     perror("Writer Process: waitpid");
                     exit(EXIT_FAILURE);
                   }
            
				  if (WIFEXITED(status)) 
				    {
						printf("{Writer Process: exited, status=%d}\n\n", WEXITSTATUS(status));
					} 
				  else if (WIFSIGNALED(status)) 
					{
						printf("Writer Process: killed by signal %d\n", WTERMSIG(status));
					} 
				  else if (WIFSTOPPED(status)) 
					{
						printf("Writer Process: stopped by signal %d\n", WSTOPSIG(status));
					} 
				  else if (WIFCONTINUED(status)) 
					{
						printf("Writer Process: continued\n");
					}				  				  				  
			    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
			    			                 
             do {
				  pidEnd2 = waitpid(pidCh2, &status, WUNTRACED | WCONTINUED);				  
				  if (pidEnd2 == -1) 
				   {					 
                     perror("Reader Process: waitpid");
                     exit(EXIT_FAILURE);
                   }
            
				  if (WIFEXITED(status)) 
				    {
						printf("{Reader Process: exited, status=%d}\n\n", WEXITSTATUS(status));
					} 
				  else if (WIFSIGNALED(status)) 
					{
						printf("Reader Process: killed by signal %d\n", WTERMSIG(status));
					} 
				  else if (WIFSTOPPED(status)) 
					{
						printf("Reader Process: stopped by signal %d\n", WSTOPSIG(status));
					} 
				  else if (WIFCONTINUED(status)) 
					{
						printf("Reader Process: continued\n");
					}				  				  				  
			    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                   
			 printf("*******************************************************\n");
			 printf("***                                                 ***\n");	
			 printf("*** Character Indexer Program execution ends here   ***\n");
			 printf("***                                                 ***\n");	
			 printf("*******************************************************\n\n");
            
			 /* cleanup semaphore */             
			 sem_unlink ("Read_child_semaphore");   
			 sem_close(sem_child_Read);                          
	       }                                                       
      }    
    
    return 0;    
}

int  ChildWriteProcess(char* fileName, FILE** readFile)
{  
   char randomletter;
   int sizeArray;
   int i;
   
   sizeArray = 2000;
   
   if(( *readFile = fopen(fileName,"w+")) == NULL)
    {
        perror("Failed: ");
        return 1;
    }
   
   printf(" I am the ChildWrite Process \n");
   printf(" ---------------------------- \n\n");
   
   printf(" Filling file with random characters: \n\n");
      
   for (i = 0; i < sizeArray; i++)
    {
        randomletter = 'a' + (random() % 26);
        fputc (randomletter, *readFile);
        printf("%c ", randomletter);        
    }      
   
   printf(" \n\n");
   fclose (*readFile);
   
   return 0;   
}

int  ChildReadProcess(char* fileName, FILE** readFile)
{
   pthread_t  thread_id1;
   pthread_t  thread_id2;
   pthread_t  thread_id3;
   pthread_t  thread_id4;
   
   int        rc;
   int        charIndex[26]; // array that will store char index statistics  
   int        i;
   int        sizeArray;
             
   sizeArray = 2000;
   	   		
   printf(" I am the ChildRead Process \n");  
   printf(" ---------------------------- \n\n");
   
   for (i = 0; i < sizeArray; i++)
    {
      charIndex[i] = 0;  // initialize array
    }
          
   // prepare the struct(s) data that will be pashed to thread(s)
   thread_data_array[0].file_name = fileName;
   thread_data_array[0].readFile = readFile;
   thread_data_array[0].start_index = 0;
   thread_data_array[0].end_index = 500;
   thread_data_array[0].chridx = (int*) &charIndex;
   
   thread_data_array[1].file_name = fileName;
   thread_data_array[1].readFile = readFile;
   thread_data_array[1].start_index = 500;
   thread_data_array[1].end_index = 1000;
   thread_data_array[1].chridx = (int*) &charIndex;
   
   thread_data_array[2].file_name = fileName;
   thread_data_array[2].readFile = readFile;
   thread_data_array[2].start_index = 1000;
   thread_data_array[2].end_index = 1500;
   thread_data_array[2].chridx = (int*) &charIndex;
   
   thread_data_array[3].file_name = fileName;
   thread_data_array[3].readFile = readFile;
   thread_data_array[3].start_index = 1500;
   thread_data_array[3].end_index = 2000;
   thread_data_array[3].chridx = (int*) &charIndex;
               
   // create new thread(s) that will execute 'ThreadIndexer'
   rc = pthread_create(&thread_id1, NULL, ThreadIndexer, (void*) &thread_data_array[0]);  
   if(rc)	// could not create thread, return value <> 0
    {
        printf("\n ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }
   printf("\n Created new thread (%lu) ... \n\n", thread_id1);  
   
   rc = pthread_create(&thread_id2, NULL, ThreadIndexer, (void*) &thread_data_array[1]);  
   if(rc)	// could not create thread, return value <> 0
    {
        printf("\n ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }
   printf("\n Created new thread (%lu) ... \n\n", thread_id2);  
   
   rc = pthread_create(&thread_id3, NULL, ThreadIndexer, (void*) &thread_data_array[2]);  
   if(rc)	// could not create thread, return value <> 0
    {
        printf("\n ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }
   printf("\n Created new thread (%lu) ... \n\n", thread_id3);  
   
   rc = pthread_create(&thread_id4, NULL, ThreadIndexer, (void*) &thread_data_array[3]);  
   if(rc)	// could not create thread, return value <> 0
    {
        printf("\n ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }
   printf("\n Created new thread (%lu) ... \n\n", thread_id4);  
      
   // wait for all threads to finish   
   (void) pthread_join(thread_id1, NULL);
   (void) pthread_join(thread_id2, NULL);
   (void) pthread_join(thread_id3, NULL);
   (void) pthread_join(thread_id4, NULL);
            
   // clean up mutex
   pthread_mutex_destroy(&lock);
            
   // print Final statistics array 
   printf(" \n");
   printf(" Final Statistics for processing characters \n");
   printf(" -------------------------------------------\n\n");    
   for(i = 0; i < 26; i++)
   {
	 printf(" %c : %d\n", i+97, charIndex[i]);
   }
   printf(" \n\n");
      
   fclose (*readFile);
       
   exit(0);            
}

void* ThreadIndexer(void* data)
{
    int i;
    char c;                       // each character read by the file
    int c_ASCII;                  // equivalent ASCII value 
    int thread_char_index[26];    // temporary array to store statistics processed by this thread ONLY
    
    struct threadata *my_data = (struct threadata *) data;  /* data received by thread */

    pthread_detach(pthread_self()); // When a detached thread terminates, its resources are automatically released back to the system
    
    if(( *my_data->readFile = fopen(my_data->file_name,"r")) == NULL)
    {
        perror("Failed: ");
        pthread_exit(NULL);
    }
        
    for (i = 0; i < 26; i++)
    {
      thread_char_index[i] = 0;  // initialize array
    }
            	
	// read the characters that this thread must process
	fseek (*my_data->readFile , my_data->start_index, SEEK_SET);
    for(i = 0; i < my_data->end_index - my_data->start_index; i++)
    {
        c = getc(*my_data->readFile);
        c_ASCII = (int)c;
        // ASCII value for 'a' is 97 that is why we substruct 97 to find proper place at temporary array of statistics
        thread_char_index[c_ASCII-97] = thread_char_index[c_ASCII-97] + 1;                
    }
    
    /* ********************************************** */
    /* *** start of critical path: lock resources *** */

    // acquire a lock
    pthread_mutex_lock(&lock);
            
		// print temporary statistics array as processed by this thread
		printf(" \n");
		printf(" Statistics for processing characters from: %d - %d\n", my_data->start_index + 1, my_data->end_index);
		printf(" --------------------------------------------------------\n\n");    
		for(i = 0; i < 26; i++)
		{
			printf(" %c : %d\n", i+97, thread_char_index[i]);
		}
		printf(" \n\n");
				
		// update concatenated / final statistics array
		printf(" \n");
		printf(" Updating master Statistics for characters from: %d - %d ...\n", my_data->start_index + 1, my_data->end_index);    
		for(i = 0; i < 26; i++)
		{		
			my_data->chridx[i] = my_data->chridx[i] + thread_char_index[i];
		}                    
		printf(" done! \n\n");

    // release lock
    pthread_mutex_unlock(&lock);
    
    /* *** end of critical path: unlock resources *** */
    /* ********************************************** */
  
    // fclose (*my_data->readFile); closing the file inside the thread is wrong for multi-threading App
      
    pthread_exit(NULL);			/* terminate the thread */
}