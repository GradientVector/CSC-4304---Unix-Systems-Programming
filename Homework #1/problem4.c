
#include <stdio.h>
#include <sys/file.h>

#define BUFFERSIZE 1

main(int argc, char *argv[])
{
    int fd1, fd2;
    char buffer[BUFFERSIZE];
    int numBytes;
    long fileSize;

    //check the number of input arguments
    if (argc != 3)
    {
        printf("Error: Program requires two file names as arguments\n");
        return -1;
    }
    
    //check if the first file can be opened for reading
    if ((fd1 = open(argv[1], O_RDONLY, 0)) < 0)
    {    
        printf("Error: Unable to open file %s for reading!\n", argv[1]);
        return -1;
    }

    //check if the first file can be opened for writing
    if ((fd2 = creat(argv[2], 0755)) < 0)
    {
        printf("Error: Unable to open file %s for writing!\n", argv[2]);
        return -1;
    }    
        
    //seek to just before the end of the reading file and determine its size
    fileSize = lseek(fd1, -1L, 2);      
  
    //copy the first file to the second file in reverse order
    while ((fileSize >= 0) && ((numBytes = read(fd1, buffer, BUFFERSIZE)) > 0))
    {
        if (write(fd2, buffer, numBytes) != numBytes)
        {
            printf("Error: Unable to write file %s!", argv[2]);
            return -1;        
        }
        
        //seek to the previous character in the reading file
        fileSize = lseek(fd1, -2, 1);
    }       

    if (numBytes < 0)
    {
        printf("Error: Unable to read file %s!", argv[1]);
        return -1;        
    }
    
    //close the files
    close(fd1);
    close(fd2);

    return 0;
}
