
#include <stdio.h>

int main(int argc, char *argv[])
{


}

int pipeToChild[2], pipeToFD[2], originalStandardOutputFD, newfd, pid, status;

int my_popen(char *command)
{ 
  // open up the pipes
  if (pipe(pipeToChild) == -1) 
  {
    perror("Cannot create a pipe to child");
    exit(1);
  }
  if (pipe(pipeToFD) == -1) 
  {
    perror("Cannot create a pipe to fd");
    exit(1);
  }
  
  if ((pid = fork()) == -1)
  {
    fprintf(stderr, "Cannot fork\n");
    exit(1);
  }

  if (pid == 0) // the child process
  {
    close(pipeToChild[1]); // close writing end of pipe to child
    close(pipeToFD[0]); // close reading end of pipe to fd
    close(0); // close standard input
    close(1); // close standard output
    newfd = dup(pipeToChild[0]); // duplicate reading end of pipe to child
    if (newfd != 0)
    {
      fprintf(stderr, "Dupe failed on reading end of pipe to child\n");
      exit(1);
    }
    newfd = dup(pipeToFD[1]); // duplicate writing end of pipe to fd
    if (newfd != 1)
    {
      fprintf(stderr, "Dupe failed on writing end of pipe to fd\n");
      exit(1);
    }    
    execl("/bin/sh", "sh", "-c", command, (char *)0);
    _exit(127) // execl error
  }
  else // the parent process
  {
    close(pipeToChild[0]); // close reading end of pipe to child
    close(pipeToFD[1]); // close writing end of pipe to fd
    originalStandardOutputFD = dup(1); // save the original standard output fd
    close(1); // close standard output
    newfd = dup(pipeToChild[1]); // duplicate writing end of pipe to child
    if (newfd != 1)
    {
      fprintf(stderr, "Dupe failed on writing end of pipe to child\n");
      exit(1);
    }
    return pipeToFD[0]; // return the reading end of the pipe to fd
  }
}

int my_pclose(int fd)
{
  while (waitpid(pid, &status, 0) < 0) // wait for the command to finish
  {
    // close the remaining fd
    close(pipeToChild[1]);
    close(pipeToFD[0]);
    close(1);
    newfd = dup(originalStandardOutputFD); // duplicate writing end of pipe to child
    if (newfd != 1)
    {
      fprintf(stderr, "Restoration of standard output failed\n");
      exit(1);
    }    
    
    // restore the original standard output
    newfd = dup(pipeToChild[1]); // duplicate writing end of pipe to child
    if (newfd != 1)
    {
      fprintf(stderr, "Dupe failed on writing end of pipe to child\n");
      exit(1);
    }    
  
    if (errno != EINTR)
    {
      status = -1; // error other than EINTR from waitpid()
      break;
    }
  }
  
  return (status);
}
