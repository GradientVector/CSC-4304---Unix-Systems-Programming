



/****************************************
CSC 4304 - Systems Programming
Louisiana State University - Fall 2010
Programming Project #1
Due: October 3, 2010
Student: John Timothy Harvey
****************************************/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <time.h>

#include <sys/ioctl.h>

#include <pwd.h>
#include <grp.h>

#define TRUE 1
#define FALSE 0

#define MAX_NAME_LENGTH 10  // the maximum length to display for a user or group name


//---------------------------------------------------------------------------------------
// global constants
const char *PROGRAM_NAME = "myls";
const int TAB_LENGTH = 2;
const int BUFFER_SIZE = 65; // the maximum number of characters required to hold
                            // a string representation of an interger value
struct // structure holding information about which command line options were set
{
  int doNotIgnoreStartingDot;
  int listByColumns;
  int listDirectoryEntriesInsteadOfContents;
  int useLongFormat;
  int dereferenceSymbolicLinks;
  int writeSlashes;
  int reverseSortOrder;
  int listSubdirectoriesRecursively;
  int sortByFileSize;
} options = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};   
struct columnWidthInformation//
{
  int linksColumnWidth;
  int uidColumnWidth;
  int gidColumnWidth;
  int sizeColumnWidth;
};
struct stat fileInformation; // structure holding information about an individual file
char *currentDirectory;
//---------------------------------------------------------------------------------------


// function declarations
int parseCommandLineArguments(int argc, char *argv[], char *directoriesToList[]);

void sortDirectoryEntries(int numberDirectoryEntries, struct dirent *directoryEntryPointerArray[]);
int compareDirectoryEntriesAlphabetically(const void *entry1, const void *entry2);
int compareDirectoryEntriesBySize(const void *entry1, const void *entry2);

void listDirectory(char *directory);
void updateColumnWidths(char *filename, char *directory, struct columnWidthInformation *columnWidths);
void updateTotalBlocks(char *filename, char *directory, int *totalBlocks);
void printEntry(char *filename, char *directory, struct columnWidthInformation columnWidths);
void printInColumns(int numberDirectoryEntries, struct dirent *directoryEntryPointerArray[], char *directory);
int isDirectory(char *filename, char *directory);
int isSymbolicLink(char *filename, char *parentDirectory);
char *uidToName(uid_t uid);
char *gidToName(gid_t gid);
void modeToString(int mode, char str[]);


// START OF THE PROGRAM
int main(int argc, char *argv[])
{
  // Initialize an array to hold pointers to names of directories to search.
  // This array will be populated by directories specified at the command line.
  // Note that the size of this array has been set to (argc - 1). This will allow
  // for storage of all of the command line arguments if necessary. However, not all
  // of the array may be used if some of the command line arguments are options (dashes
  // followed by option specifiers). I just planned for a worst case scenario. =)
  char *directoriesToList[argc - 1]; 
  
  // Call a function to parse the command line arguments, which will initialize 
  // the directories array and set options for ls. Returns the number of directories
  // parsed from the command line arguments. The number of directories will be zero
  // if there were no directories specified at the command line.
  int numberDirectoriesToList = parseCommandLineArguments(argc, argv, directoriesToList); 
  
  // make a columnWidths struct in case we need to print long list format on 
  struct columnWidthInformation columnWidths;
  columnWidths.linksColumnWidth = 0;
  columnWidths.uidColumnWidth = 0;
  columnWidths.gidColumnWidth = 0;
  columnWidths.sizeColumnWidth = 0;
  
  // now its time to do some actual work
  // list the contents of the working directory if we didn't parse any directory names from the command line
  if (numberDirectoriesToList == 0)
  {
    if (options.listDirectoryEntriesInsteadOfContents)
    {
      updateColumnWidths(".", ".", &columnWidths);
      printEntry(".", ".", columnWidths);
    }
    else
      listDirectory(".");
  }
  else // otherwise iterate over the directoriesToList array 
  {
    int i;
    for (i = 0; i < numberDirectoriesToList; i++)
    {
      if (options.listDirectoryEntriesInsteadOfContents)
      {
        updateColumnWidths(directoriesToList[i], ".", &columnWidths);
        printEntry(directoriesToList[i], ".", columnWidths);
      }
      else
        listDirectory(directoriesToList[i]);
    }
  }

  exit(0); // We are done! Hooray!!!
}


// function to parse the command line arguments
int parseCommandLineArguments(int argc, char *argv[], char *directoriesToList[])
{
  int numberDirectoriesToList = 0; // variable to hold the number of valid directory names
                                   // parsed from the command line

  // we need to get names of directories to search or set options if at least one 
  // additional argument is present (please note that the program name is always 
  // the first command line argument) 
  if (argc > 1)
  {
    int i;
    char *argumentPointer;
    
    // iterate over all command line arguments
    for (i = 1; i < argc; i++)
    {
      argumentPointer = argv[i]; // pointer to the current argument
    
      if (*argumentPointer == '-') // check if the first character of the argument is a 'dash'
      {
        while (*++argumentPointer != '\0') // iterate until the null character is found
        {
          // for debugging purposes
          //printf("*argumentPointer = %c\n", *argumentPointer);
        
          // set the option specified by the character pointed to by argumentPointer
          switch (*argumentPointer)
          {
            case 'a':
              options.doNotIgnoreStartingDot = TRUE;
              break;
            case 'C':
              options.listByColumns = TRUE;
              options.useLongFormat = FALSE;
              break;
            case 'd':
              options.listDirectoryEntriesInsteadOfContents = TRUE;
              break;
            case 'l':
              options.useLongFormat = TRUE;
              options.listByColumns = FALSE;
              break;
            case 'L':
              options.dereferenceSymbolicLinks = TRUE;
              break;
            case 'p':
              options.writeSlashes = TRUE;
              break;
            case 'r':
              options.reverseSortOrder = TRUE;
              break;
            case 'R':
              options.listSubdirectoriesRecursively = TRUE;
              break;
            case 'S':
              options.sortByFileSize = TRUE;
              break;
            case '-': // we have to test for double-dash "--" options
              ++argumentPointer; // move to the next character after the doube-dashes '--'
              if (*argumentPointer == '\0') // no option was specified
              {
                // back the pointer up so we don't mess up the while loop check
                --argumentPointer;
              }
              else if (strcmp(argumentPointer, "help") == 0) // "--help" option (display the usage information)
              {
                printf("Usage: %s [OPTION]... [FILE]...\n", PROGRAM_NAME);
                printf("List information about the FILEs (the current directory by default)\n\n");
                printf("  -a      :do not ignore entries starting with a dot(.)\n");
                printf("  -C      :list entries by columns\n");
                printf("  -d      :list directory entries instead of contents, and do not\n");
                printf("            dereference symbolic links\n");
                printf("  -l      :use a long listing format\n");
                printf("  -L      :if argument is a symbolic link, list the file or directory\n");
                printf("            the link dereferences rather than the link itself.\n");
                printf("  -p      :write a slash('/') after each filename if that file is a directory\n");
                printf("  -r      :reverse the order of the sort\n");
                printf("  -R      :list subdirectories recursively\n");
                printf("  -S      :sort by file size\n");
                printf("  --help  :display usage information (with options and their purpose) and exit\n\n");
                printf("Report bugs to jharv18@tigers.lsu.edu\n");
                exit(0); // exit the program with "everything ok" code
              }
              else // what follows after the double-dashes '--' is unrecognized so display an error message
              {
                printf("%s: unrecognized option --%s\n", PROGRAM_NAME, argumentPointer);
                exit(1); // quit the program with an error code
              }  
              break;
            default: // Invalid option type           
              printf("%s: invalid option -- '%c'\n", PROGRAM_NAME, *argumentPointer); 
              printf("Try '%s --help' for more information.\n", PROGRAM_NAME); 
              exit(1); // quit the program with an error code
              break;
          }
        }
      }
      else // the current argument is the name of a directory (Well, it should be. If it is not we will print an error)
      {
        if (stat(argumentPointer, &fileInformation) != -1)
        {
          directoriesToList[numberDirectoriesToList] = argumentPointer;

          numberDirectoriesToList++; // increment the number of directories that we have found
        }
        else // unable to access the directory so print an error message
        {
          perror(argumentPointer);
          exit(1); // <------not sure if this is needed
        }
      }
    } // <--- end of iterations over all command line arguments 
  }
  
  return numberDirectoriesToList; // return the number of valid directory names that we found
}


void sortDirectoryEntries(int numberDirectoryEntries, struct dirent *directoryEntryPointerArray[])
{
  int (*comparisonFunction)(const void *,const void *);
  
  // choose the comparison function based on the options set at the command line
  if (options.sortByFileSize == TRUE)
  {
    comparisonFunction = compareDirectoryEntriesBySize;
  }
  else
  { 
    comparisonFunction = compareDirectoryEntriesAlphabetically;
  }  
  
  qsort(directoryEntryPointerArray, numberDirectoryEntries, sizeof(struct dirent *), comparisonFunction);
  
}


int compareDirectoryEntriesAlphabetically(const void *entry1, const void *entry2)
{
  if (options.reverseSortOrder == TRUE)
    return (strcmp((*(struct dirent **)entry2)->d_name, (*(struct dirent **)entry1)->d_name));
  else
    return (strcmp((*(struct dirent **)entry1)->d_name, (*(struct dirent **)entry2)->d_name));
}


int compareDirectoryEntriesBySize(const void *entry1, const void *entry2)
{
  // allocate a string to hold the path to the current entries
  char *filename1WithDirectory;
  if (strcmp(currentDirectory, ".") == 0)
  {
    filename1WithDirectory = (*(struct dirent **)entry1)->d_name;
  }
  else
  {
    filename1WithDirectory = calloc(strlen(currentDirectory) + strlen((*(struct dirent **)entry1)->d_name) + 2, sizeof(char));
    strcpy(filename1WithDirectory, currentDirectory);
    strcat(filename1WithDirectory, "/");
    strcat(filename1WithDirectory, (*(struct dirent **)entry1)->d_name);     
  }
  char *filename2WithDirectory;
  if (strcmp(currentDirectory, ".") == 0)
  {
    filename2WithDirectory = (*(struct dirent **)entry2)->d_name;
  }
  else
  {
    filename2WithDirectory = calloc(strlen(currentDirectory) + strlen((*(struct dirent **)entry2)->d_name) + 2, sizeof(char));
    strcpy(filename2WithDirectory, currentDirectory);
    strcat(filename2WithDirectory, "/");
    strcat(filename2WithDirectory, (*(struct dirent **)entry2)->d_name);     
  }  

  struct stat fileStatus1, fileStatus2;

  // open up the stat structure for the two entries
  if (stat(filename1WithDirectory, &fileStatus1) == -1)
  {
    perror(filename1WithDirectory);
    exit(1);
  }
  if (stat(filename2WithDirectory, &fileStatus2) == -1)
  {
    perror(filename2WithDirectory);
    exit(1);
  }
  
  // free the memory used by the temporary string holding path to the files
  if (strcmp(currentDirectory, ".") != 0)
  {
    free(filename1WithDirectory);
    free(filename2WithDirectory);
  }
  
  // compare the two entries according to their size
  if (options.reverseSortOrder == TRUE)
  {
    if(fileStatus1.st_size > fileStatus2.st_size)
      return 1;
    else if(fileStatus1.st_size == fileStatus2.st_size)
      return 0;
    else 
      return -1;
  }
  else
  {
    if(fileStatus2.st_size > fileStatus1.st_size)
      return 1;
    else if(fileStatus2.st_size == fileStatus1.st_size)
      return 0;
    else 
      return -1;
  }
}



// This is a function which scans a given directory, sorts its contents, and then lists the required information
void listDirectory(char *directory)
{
  DIR *directoryPointer;
  struct dirent *directoryEntryPointer;
  int numberDirectoryEntries = 0;
  struct dirent **directoryEntryPointerArray;
  
  // creat a new structure to hold the column widths for the number of links, user id, group id, and file size
  struct columnWidthInformation columnWidths;
  columnWidths.linksColumnWidth = 0;
  columnWidths.uidColumnWidth = 0;
  columnWidths.gidColumnWidth = 0;
  columnWidths.sizeColumnWidth = 0;
  
  // total number of blocks used by files in the directory
  int totalBlocks = 0;
  
  // open up the given directory
  if ((directoryPointer = opendir(directory)) == NULL) 
  {
    fprintf(stderr, "%s: cannot open %s\n", PROGRAM_NAME, directory); // print error message if we can't open the directory
  }
  else
  {
    // find out how many files we have in the directory
    while ((directoryEntryPointer = readdir(directoryPointer)) != NULL)
    {
      if ((*(directoryEntryPointer->d_name) != '.') || (options.doNotIgnoreStartingDot == TRUE))
      {
        numberDirectoryEntries++; 
        updateColumnWidths(directoryEntryPointer->d_name, directory, &columnWidths);
        updateTotalBlocks(directoryEntryPointer->d_name, directory, &totalBlocks);
      }  
    }
    
    rewinddir(directoryPointer); // reset the position of the directory stream to the beginning of the directory
    
    // allocate memory for an array to hold the directory entries. We will need this for sorting and arrangining into columns
    // MENTAL CHECK: Is the memory set free later? --> YES
    directoryEntryPointerArray = malloc(numberDirectoryEntries * sizeof(struct dirent *));
    
    // populate the directory entry pointer array
    int i = 0;
    while ((directoryEntryPointer = readdir(directoryPointer)) != NULL)
    {
      if ((*(directoryEntryPointer->d_name) != '.') || (options.doNotIgnoreStartingDot == TRUE))
      {
        directoryEntryPointerArray[i] = directoryEntryPointer;
        i++;
      }
    }   
             
    // sort the directory entry pointer array
    currentDirectory = directory;
    sortDirectoryEntries(numberDirectoryEntries, directoryEntryPointerArray); 
    
    // if we are to list directories recursively then print the directory name followed by a colon
    if (options.listSubdirectoriesRecursively)
    {
      printf("%s:\n", directory);
    } 
 
    // if the Columns option has been specified then we need to print the output organized into columns
    if (options.listByColumns)
    {
      printInColumns(numberDirectoryEntries, directoryEntryPointerArray, directory);
    }
    else // otherwise print everything in one column
    {     
      if (options.useLongFormat) // print the total number of blocks used by files in the directory
      {    
        printf("total %d\n", totalBlocks);
      }
      
      // iterate over the directory entry pointer array and print information for each entry
      for (i = 0; i < numberDirectoryEntries; i++)
      {
        printEntry(directoryEntryPointerArray[i]->d_name, directory, columnWidths);
      }
    }
    
    // if recursion has been requested then we need to call listDirectory on the directories
    if (options.listSubdirectoriesRecursively)
    {
      printf("\n");
    
      for (i = 0; i < numberDirectoryEntries; i++)
      {
        // allocate a string to hold the path to the current file
        char *filenameWithDirectory = calloc(strlen(directory) + strlen(directoryEntryPointerArray[i]->d_name) + 2, sizeof(char));
        strcpy(filenameWithDirectory, directory);
        strcat(filenameWithDirectory, "/");
        strcat(filenameWithDirectory, directoryEntryPointerArray[i]->d_name);      
 
            //printf("File: %s | Directory: %s\n", directoryEntryPointerArray[i]->d_name, directory); 
      
        if (isDirectory(directoryEntryPointerArray[i]->d_name, directory) && directoryEntryPointerArray[i]->d_name[0] != '.')
        {        
            
            
          if (options.dereferenceSymbolicLinks == TRUE || isSymbolicLink(directoryEntryPointerArray[i]->d_name, directory) == FALSE)
          {      
            listDirectory(filenameWithDirectory);
          }
        } 
        
        // Free the memory used by the path to the current file
        free(filenameWithDirectory);
      }
    }
    
    // close the directory
    closedir(directoryPointer);
    
    // Free the memory used by the directory entry pointer array
    free(directoryEntryPointerArray);
  }
} 


// function to inspect a directory entry and update the column width information to be used for printing
void updateColumnWidths(char *filename, char *directory, struct columnWidthInformation *columnWidths)
{
  // allocate a string to hold the path to the current file
  char *filenameWithDirectory;
  if (strcmp(directory, ".") == 0)
  {
    filenameWithDirectory = filename;
  }
  else
  {
    filenameWithDirectory = calloc(strlen(directory) + strlen(filename) + 2, sizeof(char));
    strcpy(filenameWithDirectory, directory);
    strcat(filenameWithDirectory, "/");
    strcat(filenameWithDirectory, filename);     
  }
  
  int stringLength; // represents the length of the string represenation of an integer
  char buffer[BUFFER_SIZE];// buffer to hold string representation of an integer

  if (stat(filenameWithDirectory, &fileInformation) != -1)
  {
    // update the links column
    sprintf(buffer, "%d", fileInformation.st_nlink);   
    stringLength = strlen(buffer);
    if (stringLength > columnWidths->linksColumnWidth)
      columnWidths->linksColumnWidth = stringLength;    
  
    // update the user id column
    sprintf(buffer, "%d", fileInformation.st_uid);
    stringLength = strlen(buffer);
    if (stringLength > columnWidths->uidColumnWidth)
      columnWidths->uidColumnWidth = stringLength;
          
    // update the group id column
    sprintf(buffer, "%d", fileInformation.st_gid);
    stringLength = strlen(buffer);
    if (stringLength > columnWidths->gidColumnWidth)
      columnWidths->gidColumnWidth = stringLength;
 
    // update the file size column
    sprintf(buffer, "%ld", fileInformation.st_size);
    stringLength = strlen(buffer);
    if (stringLength > columnWidths->sizeColumnWidth)
      columnWidths->sizeColumnWidth = stringLength;       
  }
  else 
  {
    perror(filenameWithDirectory);
  }
}


// function to update the total number of blocks in a directory
void updateTotalBlocks(char *filename, char *directory, int *totalBlocks)
{
  // allocate a string to hold the path to the current file
  char *filenameWithDirectory;
  if (strcmp(directory, ".") == 0)
  {
    filenameWithDirectory = filename;
  }
  else
  {
    filenameWithDirectory = calloc(strlen(directory) + strlen(filename) + 2, sizeof(char));
    strcpy(filenameWithDirectory, directory);
    strcat(filenameWithDirectory, "/");
    strcat(filenameWithDirectory, filename);     
  }
  
  if (options.dereferenceSymbolicLinks == FALSE)
  {
    if (lstat(filenameWithDirectory, &fileInformation) != -1)
    {
      double adjustment = 512 / fileInformation.st_blksize;
      *totalBlocks += fileInformation.st_blocks / 2;
    }
    else
    {
      perror(filenameWithDirectory);
    }
  }
  else
  {
    if (stat(filenameWithDirectory, &fileInformation) != -1)
    {
      double adjustment = 512 / fileInformation.st_blksize;
      *totalBlocks += fileInformation.st_blocks / 2;
    }
    else
    {
      perror(filenameWithDirectory);
    }
  }
}


// function to print an individual directory entry
void printEntry(char *filename, char *directory, struct columnWidthInformation columnWidths)
{
  // allocate a string to hold the path to the current file
  char *filenameWithDirectory;
  if (strcmp(directory, ".") == 0)
  {
    filenameWithDirectory = filename;
  }
  else
  {
    filenameWithDirectory = calloc(strlen(directory) + strlen(filename) + 2, sizeof(char));
    strcpy(filenameWithDirectory, directory);
    strcat(filenameWithDirectory, "/");
    strcat(filenameWithDirectory, filename);     
  }
  
  if (options.useLongFormat) // print extra information
  {
    if (options.dereferenceSymbolicLinks == FALSE)
    {
      if (lstat(filenameWithDirectory, &fileInformation) != -1)
      {
        char modeString[11];
        modeToString((int) fileInformation.st_mode, modeString);
        
        printf("%s ", modeString);       
        printf("%*d ", columnWidths.linksColumnWidth, (int) fileInformation.st_nlink);
        printf("%-*s ", columnWidths.uidColumnWidth, uidToName(fileInformation.st_uid));
        printf("%-*s ", columnWidths.gidColumnWidth, gidToName(fileInformation.st_gid));
        printf("%*ld ", columnWidths.sizeColumnWidth, (long) fileInformation.st_size);
        
        struct tm *timeInfo = localtime(&(fileInformation.st_mtime));
        printf("%4d-%02d-%02d %02d:%02d ", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min);
      }
      else
      {
        perror(filenameWithDirectory);
      }    
    }
    else
    {
      if (stat(filenameWithDirectory, &fileInformation) != -1)
      {
        char modeString[11];
        modeToString((int) fileInformation.st_mode, modeString);
        
        printf("%s ", modeString);       
        printf("%*d ", columnWidths.linksColumnWidth, (int) fileInformation.st_nlink);
        printf("%-*s ", columnWidths.uidColumnWidth, uidToName(fileInformation.st_uid));
        printf("%-*s ", columnWidths.gidColumnWidth, gidToName(fileInformation.st_gid));
        printf("%*ld ", columnWidths.sizeColumnWidth, (long) fileInformation.st_size);
        
        struct tm *timeInfo = localtime(&(fileInformation.st_mtime));
        printf("%4d-%02d-%02d %02d:%02d ", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min);
      }
      else
      {
        perror(filenameWithDirectory);
      }
    }
  }  
      
  // print the filename    
  if (options.dereferenceSymbolicLinks == FALSE && isSymbolicLink(filename, directory) == TRUE && options.useLongFormat == TRUE)
  {
    int bufferSize = fileInformation.st_size + 1;
    char *buffer = malloc(bufferSize * sizeof(char));    
    buffer[fileInformation.st_size] = '\0';
    readlink(filenameWithDirectory, buffer, BUFFER_SIZE);
    
    char *symbolicLinkFilename;
    char *symbolicLinkDirectory;
    if (buffer[0] == '/') // we have a full pathname
    {
      symbolicLinkFilename = buffer;
      symbolicLinkDirectory = ".";
    }
    else
    {
      symbolicLinkFilename = filename;
      symbolicLinkDirectory = directory;
    }
          
    if (options.writeSlashes && isDirectory(symbolicLinkFilename, symbolicLinkDirectory)) 
      printf("%s -> %s/\n", filename, buffer); // print the name of the file a symbolic link points to
    else
      printf("%s -> %s\n", filename, buffer); 
    free(buffer);
  }
  else if (options.writeSlashes && isDirectory(filename, directory) && isSymbolicLink(filename, directory) == FALSE) 
    printf("%s/\n", filename); // append a slash if it is a directory and the '-p' is turned on
  else
    printf("%s\n", filename);
            
  // Free the memory used by the patht to the current file
  if (strcmp(directory, ".") != 0)
    free(filenameWithDirectory);
}


// function which checks if the given file is a directory
int isDirectory(char *filename, char *parentDirectory)
{
  // allocate a string to hold the path to the current file
  char *filenameWithDirectory;
  if (strcmp(parentDirectory, ".") == 0)
  {
    filenameWithDirectory = filename;
  }
  else
  {
    filenameWithDirectory = calloc(strlen(parentDirectory) + strlen(filename) + 2, sizeof(char));
    strcpy(filenameWithDirectory, parentDirectory);
    strcat(filenameWithDirectory, "/");
    strcat(filenameWithDirectory, filename);     
  }
  
  if (stat(filenameWithDirectory, &fileInformation) != -1)
  {
    // Free the memory used by the patht to the current file
    if (strcmp(parentDirectory, ".") != 0)
      free(filenameWithDirectory);   
  
    return S_ISDIR(fileInformation.st_mode);
  }
  else
  {
    // Free the memory used by the patht to the current file
    if (strcmp(parentDirectory, ".") != 0)
      free(filenameWithDirectory);   
  
    perror(filename);
    return FALSE;
  }
}

// function which checks if the given file is a symbolic link
int isSymbolicLink(char *filename, char *parentDirectory)
{
  // allocate a string to hold the path to the current file
  char *filenameWithDirectory;
  if (strcmp(parentDirectory, ".") == 0)
  {
    filenameWithDirectory = filename;
  }
  else
  {
    filenameWithDirectory = calloc(strlen(parentDirectory) + strlen(filename) + 2, sizeof(char));
    strcpy(filenameWithDirectory, parentDirectory);
    strcat(filenameWithDirectory, "/");
    strcat(filenameWithDirectory, filename);     
  }
  
  if (options.dereferenceSymbolicLinks == FALSE)
  {
    if (lstat(filenameWithDirectory, &fileInformation) != -1)
    {
      // Free the memory used by the patht to the current file
      if (strcmp(parentDirectory, ".") != 0)
        free(filenameWithDirectory);  
          
      return S_ISLNK(fileInformation.st_mode);
    }
    else
    {
      // Free the memory used by the patht to the current file
      if (strcmp(parentDirectory, ".") != 0)
        free(filenameWithDirectory);  
    
      perror(filename);
      return FALSE;
    }
  }
  else
  {
    if (stat(filenameWithDirectory, &fileInformation) != -1)
    {
      // Free the memory used by the patht to the current file
      if (strcmp(parentDirectory, ".") != 0)
        free(filenameWithDirectory);  
          
      return S_ISLNK(fileInformation.st_mode);
    }
    else
    {
      // Free the memory used by the patht to the current file
      if (strcmp(parentDirectory, ".") != 0)
        free(filenameWithDirectory);  
    
      perror(filename);
      return FALSE;
    }
  }
}


// function to print the files into columns
void printInColumns(int numberDirectoryEntries, struct dirent *directoryEntryPointerArray[], char *directory)
{
  int numberRows = 1; // variable to hold the number of rows we will need to print
  int numberColumns;  // variable to hold the number of columns we will need to print
  int notFinished;
  int *columnWidthArray; // pointer to array holding the width of each column to be printed
  int i, j, column;

  struct winsize ws;
  ioctl(1, TIOCGWINSZ, &ws);
  //printf("Columns: %d\tRows: %d\n", ws.ws_col, ws.ws_row);  
  int windowSize = ws.ws_col;  

  // we need to make an array holding the lengths of the names for the directory entries
  int directoryEntryNameLengthsArray[numberDirectoryEntries];
  for (i = 0; i < numberDirectoryEntries; i++)
  {
    directoryEntryNameLengthsArray[i] = strlen(directoryEntryPointerArray[i]->d_name);
  }

  // find out how many columns we can fit within the window
  do
  {
    int total;
    notFinished = FALSE; // set notFinished to FALSE, meaning that we will be finished unless we later change it to TRUE.
    
    // calculate the number of columns corresponding to the number of rows that we currently have
    numberColumns = (int) ceil((float) numberDirectoryEntries / (float) numberRows); 
    
    // allocate an array to keep track of the maximum width for a column
    // MENTAL CHECK: Is the memory set free later? --> YES
    columnWidthArray = calloc(numberColumns, sizeof(int));
    
    // initialize the widths array to zeroes
    for (i = 0; i < numberColumns; i++)
    {
      columnWidthArray[i] = 0;
    }

    // iterate over each row
    for (i = 0; i < numberRows; i++)
    {
      // iterate over the entries for a row. Because the entries are ordered verticaly in columns we may have to skip by the number of rows.
      for (j = i, column = 0; j < numberDirectoryEntries; j = j + numberRows, column++)
      {
        // update the width for each column if the width of the current name is bigger
        if (columnWidthArray[column] < directoryEntryNameLengthsArray[j]) 
        {
          columnWidthArray[column] = directoryEntryNameLengthsArray[j];
        }
      }
    }
    
    // check if the entries will fit in current number of rows
    total = 0;
    for (i = 0; i < numberColumns; i++)
    {
      total += (columnWidthArray[i] + TAB_LENGTH); // add up the size of the row
    }
    if (total > windowSize) // if the total size of a row is bigger than the window size then mark us as not done yet
    {
      numberRows++;
      notFinished = TRUE;
      // free the memory allocated for holding the width of each column
      free(columnWidthArray);
    }
    
  } while (notFinished);

  //printf("Number Rows: %d\nNumber Columns: %d\n", numberRows, numberColumns);

  // print the directory entries
  for (i = 0; i < numberRows; i++)
  {
    for (column = 0, j = i; j < numberDirectoryEntries; column++, (j = j + numberRows))
    {    
      // append a slash if it is a directory and the '-p' is turned on
      char *stringPointer;
      if (options.writeSlashes && isDirectory(directoryEntryPointerArray[j]->d_name, directory) && isSymbolicLink(directoryEntryPointerArray[j]->d_name, directory) == FALSE)
      {
        stringPointer = calloc(strlen(directoryEntryPointerArray[j]->d_name) + 3, sizeof(char)); 
        strcpy(stringPointer, directoryEntryPointerArray[j]->d_name);
        strcat(stringPointer, "/");
      }
      else
      {
        stringPointer = directoryEntryPointerArray[j]->d_name;
      }  

      printf("%-*s", columnWidthArray[column] + TAB_LENGTH, stringPointer);      
      
      if (options.writeSlashes && isDirectory(directoryEntryPointerArray[j]->d_name, directory) && isSymbolicLink(directoryEntryPointerArray[j]->d_name, directory) == FALSE)
        free(stringPointer);
    }
    printf("\n"); // start a new line
  }
  
  // free the memory allocated for holding the width of each column
  free(columnWidthArray);
}



// function to convert the user id to an actual user name
char *uidToName(uid_t uid)
{
  struct passwd *passwdPointer;
  static char numberString[MAX_NAME_LENGTH];
  
  if ((passwdPointer = getpwuid(uid)) == NULL) // the uid is not associated with a particular user right now
  {
    sprintf(numberString, "%d", uid); // just make the uid into a string
    return numberString; 
  }
  else
  {
    return passwdPointer->pw_name; // get the string of the user name
  }
}


// function to convert the group id to a group name
char *gidToName(gid_t gid)
{
  struct group *groupPointer;
  static char numberString[MAX_NAME_LENGTH];
  
  if ((groupPointer = getgrgid(gid)) == NULL)
  {
    sprintf(numberString, "%d", gid);
    return numberString;
  }
  else
  {
    return groupPointer->gr_name;
  }
}


// function to convert the mode integer value and turn it into a human readable string
void modeToString(int mode, char str[])
{
  strcpy( str, "----------"); // initialize the string to hold no permisions
  
  // set the file type
  if (S_ISDIR(mode))  // file is a directory
    str[0] = 'd';
  else if (S_ISBLK(mode)) // file is a block special device
    str[0] = 'b';    
  else if (S_ISCHR(mode)) // file is a char device
    str[0] = 'c';
  else if (S_ISFIFO(mode)) // file is a FIFO
    str[0] = 'f';
  else if (S_ISLNK(mode)) // file is a symbolic link
    str[0] = 'l';
  else if (S_ISSOCK(mode)) // file is a socket
    str[0] = 's';
 
  // set the user permissions
  if (S_IRUSR & mode)
    str[1] = 'r';
  if (S_IWUSR & mode)
    str[2] = 'w';
  if (S_IXUSR & mode)
    str[3] = 'x';
  if (S_ISUID & mode)
    str[3] = 's';
    
  // set the group permissions
  if (S_IRGRP & mode)
    str[4] = 'r';
  if (S_IWGRP & mode)
    str[5] = 'w';
  if (S_IXGRP & mode)
    str[6] = 'x';   
  if (S_ISGID & mode)
    str[6] = 's';

  // set the other users permissions
  if (S_IROTH & mode)
    str[7] = 'r';
  if (S_IWOTH & mode)
    str[8] = 'w';
  if (S_IXOTH & mode)
    str[9] = 'x';
  if (S_ISVTX & mode)
    str[9] = 't';
}


