
#include <stdio.h>
#include <signal.h>

void inthandler();

int main(int argc, char *argv[])
{
  int i;
  struct sigaction newhandler;
  sigset_t  blocked;
  void  inthandler();
  
  newhandler.sa_handler = inthandler;
  sigfillset(&blocked);
  newhandler.sa_mask = blocked;
  
  // set up signal handler
  for (i = 1; i < 31; i++)
    if (i != SIGKILL && i != SIGSTOP)
      if (sigaction(i, &newhandler, NULL) == -1)
        printf("error setting handler for signal %d\n", i);
  
  for (i = 10; i > 0; i--)
  {
    printf("Counting down: %d\n", i);
    sleep(1);
  }
}

void inthandler()
{
  printf("Entering interrupt handler\n");
  int i;
  for (i = 10; i > 0; i--)
  {
    printf("Counting down: %d\n", i);
    sleep(1);
  }
  printf("Exiting interrupt handler\n");
}

