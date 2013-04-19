#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define SHARED 0

sem_t chopsticks[NUM_PHILOSOPHERS];

void* philosopherCode(void *threadid);

int main(int argc, char *argv[])
{
  pthread_t philosopher_threads[NUM_PHILOSOPHERS];
  int i;
  
  // initialize the semaphores
  for (i = 0; i < NUM_PHILOSOPHERS; i++)
  {
    sem_init(&chopsticks[i], SHARED, 1);
  }
  
  // create the philosophers
  for (i = 0; i < NUM_PHILOSOPHERS; i++)
  {
    pthread_create(&philosopher_threads[i], NULL, philosopherCode, (void *)i);
  }
  
  while(1)
  {
  }
}

void* philosopherCode(void *threadid)
{
  int tid = (int)threadid;
  
  // loop forever
  while(1)
  {
    // if last thread pick up the chopsticks in reverse to avoid deadlock
    if (tid < NUM_PHILOSOPHERS - 1)
    {
      sem_wait(&chopsticks[tid]);
      sem_wait(&chopsticks[(tid + 1)%NUM_PHILOSOPHERS]);
    }
    else
    {
      sem_wait(&chopsticks[(tid + 1)%NUM_PHILOSOPHERS]);
      sem_wait(&chopsticks[tid]);
    }
    
    // eat some spaghetti!
    printf("Philosopher %i eating some spaghetti!\n", tid);
    
    // release chopsticks so somebody else gets a turn
    sem_post(&chopsticks[tid]);
    sem_post(&chopsticks[(tid + 1)%NUM_PHILOSOPHERS]);
    
    // sleep for a bit cuz you're so full NOM NOM NOM
    sleep(1);
  }
  
  pthread_exit(NULL);
}
