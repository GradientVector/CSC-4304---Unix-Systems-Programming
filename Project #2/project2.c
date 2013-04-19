

/****************************************
CSC 4304 - Systems Programming
Louisiana State University - Fall 2010
Programming Project #2
Due: December 3, 2010
Student: John Timothy Harvey
****************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>

#define TRUE 1
#define FALSE 0

#define PROGRAM_NAME "myhttpd"
#define SERVER_NAME "John's Server v1.0"
#define BUFFERLENGTH 256
#define HOSTLENGTH 256
#define CGI_PATH "/cgi-bin/"
#define HOME_PATH "/classes/cs4304/cs430412/myhttpd/"

char *rootDirectory = ".";
// structure holding the options set at the command line
struct
{
	int debuggingMode;
	int logging;
}commandLineOptions = {FALSE, FALSE};
char *logFileName; // string holding the name of the log file
FILE *logFile; // pointer for the file which logs all requests
// root directory used to resolve requests for items
//char *rootDirectory = ".";
// port number to listen on
int portNumber = 8080;


char hostname[HOSTLENGTH];
struct sockaddr_in saddr;

// mutex lock for statistics variables
pthread_mutex_t statisticsLock = PTHREAD_MUTEX_INITIALIZER;

// server statistics variables
time_t timeServerStarted;
int bytesSentFromServer;
int numberServerRequests;

// structure for holding information to be logged
struct logInformation
{
    char *IPaddress; 
    char *timeRequestReceived;
    char *quotedFirstLineOfRequest;
    int status;
    int sizeOfResponse;
};

struct threadArguments
{
    int fd;
    struct logInformation logInfo;
};


// prototypes
void parseCommandLineArguments(int argc, char *argv[]);
int makeServerSocket();
void setup(pthread_attr_t *attrPointer);
void *handleCall(void *ta);
void skipRestOfHeader(FILE *fp);
void processRequest(char *request, int fd, struct logInformation *logInfo);
void sanitize(char *argument);
int httpReply(int fd, FILE **fpp, int code, char *message, char *type, char *content, char *modifiedTime, int contentLength, struct logInformation *logInfo);
void reply_notImplemented(int fd, struct logInformation *logInfo);
void reply_404(char *item, int fd, struct logInformation *logInfo);
int isDirectory(char *fileName);
int doesNotExist(char *fileName);
void reply_ls(char *directory, int fd, struct logInformation *logInfo);
char *getFileNameExtension(char *fileName);
void reply_cat(char *fileName, int fd, struct logInformation *logInfo);
void updateBytesSent(int bytesSent);
void updateNumberServerRequests();
void reply_status(int fd, struct logInformation *logInfo);
void sortDirectoryEntries(int numberDirectoryEntries, struct dirent *directoryEntryPointerArray[]);
int compareDirectoryEntriesAlphabetically(const void *entry1, const void *entry2);
void reply_cgi(char *program, int fd, struct logInformation *logInfo);
void logRequest(struct logInformation *logInfo);
int daemon_init(void);



// MAIN FUNCTION -----------------------------------------------------------
int main(int argc, char *argv[])
{
    pthread_t worker;
    pthread_attr_t attr;
    setup(&attr);
    struct logInformation logInfo;
    struct threadArguments taTemp;
    
	parseCommandLineArguments(argc, argv);
    printf("New logging session started on port %i on %s", portNumber, ctime(&timeServerStarted));
    int sockid = makeServerSocket();
    
    // daemonize the process if debuggin mode is not set
    if (commandLineOptions.debuggingMode == FALSE)
    {
        if (daemon_init() != 0)
        {
            printf("Error: unable to daemonize process\n");
            exit(-1);
        }   
    }
    
    while (TRUE)
    {
        // allow an incoming call
        int success = listen(sockid, 1);
        if (success == -1)
        {
            printf("Error: unable to listen on socket\n");
            exit(1);
        }
        
        // wait for an incoming call
        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        socklen_t c_len = sizeof(saddr);
        int fd = accept(sockid, (struct sockaddr *)&saddr, &c_len);
        if (success == -1)
        {
            printf("Error: unable to accept connection on socket\n");
            exit(1);
        }
        //printf("address: %s\n", (char *)inet_ntoa(saddr.sin_addr));
        logInfo.IPaddress = malloc(sizeof((char *)inet_ntoa(saddr.sin_addr)));
        strcpy((logInfo.IPaddress), (char *)inet_ntoa(saddr.sin_addr));
        //printf("We got a call! Yay!\n");
        updateNumberServerRequests(); // update the number of server requests
        
        // get the current time and store it in the log structure
        time_t currentTime = time(0);
        char *timeString = asctime(gmtime(&currentTime)); // get the Greenwich Mean Time
        char *timeStringPointer = timeString;
        // step through the time string until we find the newline character and 
        // change it to a null character to remove it
        while (*timeStringPointer != '\n')
        {
            timeStringPointer++;
        }
        *timeStringPointer = '\0';
        logInfo.timeRequestReceived = timeString;        
        //printf("time request received %s\n", logInfo.timeRequestReceived);

        //int *fdPointer = malloc(sizeof(int));
        //*fdPointer = fd;
        // copy file descriptor for the socket and logInfo structure so we can pass it to a thread
        struct threadArguments *ta = malloc(sizeof(struct threadArguments));
        taTemp.fd = fd;
        taTemp.logInfo = logInfo;
        *ta = taTemp;

        if (commandLineOptions.debuggingMode == TRUE)
        {
            handleCall(ta); // only accept one request at a time
        }
        else
        {
            // create a new thread to handle the request
            pthread_create(&worker, &attr, handleCall, ta);
        }
    }
    
	return 0;
}
// -------------------------------------------------------------------------


void parseCommandLineArguments(int argc, char *argv[])
{
	int i;
	char *currentOption;
    struct stat fileInformation;// structure used for getting file information

	if (argc > 1)
	{
		for (i = 1; i < argc; i++) // iterate over all of the command line arguments
		{
			currentOption = argv[i]; // pointer to the first character of the current command line argument
			if (*currentOption == '-')
			{
			    while (*(++currentOption) != '\0')    
                {
				    switch (*currentOption)
				    {
					    case 'd':
					        commandLineOptions.debuggingMode = TRUE; 
						    break;
					    case 'h': // print a usage summary with all options and exit
						    printf("Usage: %s [-dh] [-l file] [-p port] [-r dir]\n", PROGRAM_NAME);
				            printf("A simple webserver. It binds to a given port on the given address and waits for incoming HTTP/1.0 requests. It serves content from the given directory. That is, any requests for documents are resolved relative to the this directory (the document root, by default, is the directory where the server is running.\n\n");
				            printf("  -d   Enter debugging mode. That is, do not daemonize, only accept\n");
				            printf("       one connection at a time and enable logging to stdout \n");
				            printf("       (without this option the web server will run as a daemon \n");
				            printf("       process in the background.\n");
				            printf("  -h   Print a usage summary with all options and exit.\n");
				            printf("  -l   Log all requests to the given file.\n");
				            printf("  -p   Listen on the given port. If not provided, %s will listen \n", PROGRAM_NAME);
				            printf("       on port 8080.\n");
				            printf("  -r   Set the root directory for the http server to dir.\n");
				            printf("\nReport bugs to jharv18@tigers.lsu.edu\n\n");
				            exit(0); // exit the program with "everything ok" code
						    break;	
					    case 'l':
					        commandLineOptions.logging = TRUE;
					        // test if the given file exists
					        i++; // look at the next command line argument. It should be a file.
				            if (i >= argc || *(currentOption = argv[i]) == '-')
				            {
				                printf("Error: no filename given for '-l' option\n");
				                exit(1);
				            }
				            if (commandLineOptions.debuggingMode == FALSE)
				            {
                                logFileName = currentOption;
				            }
					        while (*currentOption != '\0')
					            currentOption++;
					        currentOption--;			            
						    break;
					    case 'p':
					        i++; // look at the next command line argument to get the port number.
					        if (i >= argc || *(currentOption = argv[i]) == '-')
					        {
					            printf("Error: no port number given for '-p' option\n");
					            exit(1);
					        }
					        portNumber = atoi(currentOption);
					        if (portNumber < 0 || portNumber > 65535)
					        {
					            printf("Error: illegal port number given\n");
					            exit(1);
					        }
					        while (*currentOption != '\0')
					            currentOption++;
					        currentOption--;					            
						    break;
					    case 'r':
					        i++; // look at the next command line argument to get root directory
					        if (i >= argc || *(currentOption = argv[i]) == '-')
					        {
					            printf("Error: no root directory given for '-r' option\n");
					            exit(1);
					        } 
					        if (stat(currentOption, &fileInformation) != -1)
					        {
					            rootDirectory = currentOption;
					            if (chdir(currentOption) != 0)
					            {
					                perror("");
					                exit(1);
					            }
					        }
					        else
					        {
					            perror(currentOption);
					            exit(1);
					        }
					        if (!S_ISDIR(fileInformation.st_mode))
					        {
					            printf("Error: %s is not a directory\n", currentOption);
					            exit(1);
					        }
					        while (*currentOption != '\0')
					            currentOption++;
					        currentOption--;
						    break;
					    default:
					        printf("%s: invalid option '%s' \n", PROGRAM_NAME, --currentOption);
				            printf("Try '%s -h' for more information.\n", PROGRAM_NAME);
				            exit(1);
				    }            
				}
			}
			else
			{
				printf("%s: invalid option '%s' \n", PROGRAM_NAME, currentOption);
				printf("Try '%s -h' for more information.\n", PROGRAM_NAME);
				exit(1);
			}
		}
	}
	
	// set the file descriptor for logging
	if (commandLineOptions.logging == TRUE && commandLineOptions.debuggingMode == FALSE)
    {
		logFile = fopen(logFileName, "a");
        if (!logFile)
        {
            perror(currentOption);
            exit(1);
        }
        
    }
    else
    {
        logFile = stdout;
    }
}

int makeServerSocket()
{
    // create a socket
    int sockid = socket(PF_INET, SOCK_STREAM, 0);
    if (sockid == -1)
    {
        printf("Error: unable to create a socket\n");
        exit(1);
    }
        
    // bind the socket to host,port address
    bzero( (void *)&saddr, sizeof(saddr)); // clear out struct
    gethostname( hostname, HOSTLENGTH); // where am I?
    //printf("Hostname = %s\n", hostname);
    struct hostent *hp = gethostbyname(hostname); // get info abou the host
    bcopy((void *)hp->h_addr, (void *)&saddr.sin_addr, hp->h_length); // fill in host part
    saddr.sin_port = htons(portNumber); // fill in socket port
    saddr.sin_family = AF_INET; // fill in addr family
    int success = bind(sockid, (struct sockaddr *)&saddr, sizeof(saddr));
    if (success == -1)
    {
        printf("Error: unable to bind socket to address\n");
        exit(1);
    }
    return sockid;
}


void setup(pthread_attr_t *attrPointer)
{
    // setup for thread attributes
    pthread_attr_init(attrPointer);
    pthread_attr_setdetachstate(attrPointer, PTHREAD_CREATE_DETACHED);
    
    // setup for statistics
    time(&timeServerStarted);
    bytesSentFromServer = 0;
    numberServerRequests = 0;
}


void *handleCall(void *ta)
{
    //int fd = *(int *)fdPointer;
    struct threadArguments *taPointer = (struct threadArguments *)ta;
    int fd = taPointer->fd;
    struct logInformation logInfo = taPointer->logInfo;
    free(ta);
    char request[BUFFERLENGTH];
    
    // read request
    FILE *inputStream = fdopen(fd, "r");
    fgets(request, BUFFERLENGTH, inputStream);
    //printf("Received a call on %d: request = %s", fd, request);
    skipRestOfHeader(inputStream);
    
    // process request
    processRequest(request, fd, &logInfo);
    
    fclose(inputStream);
    
    // clean up the memory allocated for the log strings
    //free(logInfo.IPaddress);
    //free(logInfo.timeRequestReceived);
    free(logInfo.quotedFirstLineOfRequest);
}

void skipRestOfHeader(FILE *fp)
{
    char buffer[BUFFERLENGTH];
    while (fgets(buffer, BUFFERLENGTH, fp) != NULL && strcmp(buffer, "\r\n") != 0)
        ; // do nothing, just go to next loop
}


void processRequest(char *request, int fd, struct logInformation *logInfo)
{
    char command[BUFFERLENGTH];
    char argument[BUFFERLENGTH];
    
    // parse the command and argument from the request
    if (sscanf(request, "%s%s", command, argument) != 2)
        return;
    
    sanitize(argument);
    //printf("sanitized version is %s\n", argument);
    
    // get the quoted first line of the request and store it in the log structure
    int size = 0;
    char *requestPointer = request;
    while (*requestPointer != '\r' && *requestPointer != '\n')
    {
        size++;
        requestPointer++;
    }
    logInfo->quotedFirstLineOfRequest = malloc((size+3) * sizeof(char));
    strcpy(logInfo->quotedFirstLineOfRequest, "\"");
    strncat(logInfo->quotedFirstLineOfRequest, request, size);
    strcat(logInfo->quotedFirstLineOfRequest, "\"");
    
    if (strcmp(command, "GET") == 0 || strcmp(command, "POST") == 0)
    {
        if (strcmp(argument, "status") == 0)
        {
            reply_status(fd, logInfo);
        }
        else if (strcmp(getFileNameExtension(argument), "cgi") == 0) // cgi script request
        {
            reply_cgi(argument, fd, logInfo);
        }
        else if (doesNotExist(argument))
        {    
            reply_404(argument, fd, logInfo);
        }
        else if (isDirectory(argument))
        {
            // check if the directory contains an index.html file
            char *path = malloc(strlen(argument) + strlen("index.html") + 2);
            strcpy(path, argument);
            strcat(path, "/");
            strcat(path, "index.html");
            if (doesNotExist(path))
            {
                reply_ls(argument, fd, logInfo); // if index.html does not exist then list the contents of the directory
            }    
            else // otherwise cat index.html
            {
                reply_cat(path, fd, logInfo);
            }
            free(path);
        }
        else
        {
            reply_cat(argument, fd, logInfo);
        }
    }
    else
    {
        reply_notImplemented(fd, logInfo);
    }
}


// function to make sure all paths are below the root directory
void sanitize(char *argument)
{
    char *source;
    char *destination;
    
    source = destination = argument;
    
    while (*source)
    {
        if (strncmp(source, "/../", 4) == 0)
            source += 3;
        else if (strncmp(source, "../", 3) == 0)
            source += 2;
        else if (strncmp(source, "//", 2) == 0)
            source++;
        else
            *destination++ = *source++;
    }
    
    *destination = '\0';
    
    // clean up a little bit
    if (*argument == '/')
        strcpy(argument, argument+1);
    if (argument[0] == '\0' || strcmp(argument, "./") == 0 || strcmp(argument, "./..") == 0)
        strcpy(argument, ".");  
            
    if (*argument == '~')  // translate home directory if ~ is present
    {
        int shiftAmount = sizeof(HOME_PATH) - 1;
        memmove(argument + shiftAmount, argument + 2, BUFFERLENGTH - shiftAmount);
        memmove(argument, HOME_PATH, shiftAmount);
    }
}


int httpReply(int fd, FILE **fpp, int code, char *message, char *type, char *content, char *modifiedTime, int contentLength, struct logInformation *logInfo)
{
    FILE *outputStream = fdopen(fd, "w");
    int bytes = 0;
    
    if (outputStream != NULL)
    {
        bytes = fprintf(outputStream, "HTTP/1.0 %d %s\r\n", code, message);
        time_t currentTime = time(0);
        char *timeString = asctime(gmtime(&currentTime)); // get the Greenwich Mean Time       
        char *timeStringPointer = timeString;
        // step through the time string until we find the newline character and 
        // change it to a null character to remove it
        while (*timeStringPointer != '\n')
        {
            timeStringPointer++;
        }
        *timeStringPointer = '\0';
        bytes += fprintf(outputStream, "Date: %s\r\n", timeString);
        bytes += fprintf(outputStream, "Server: %s\r\n", SERVER_NAME);
        if (modifiedTime != NULL)
        {
            bytes += fprintf(outputStream, "Last-Modified: %s\r\n", modifiedTime);
        }
        bytes += fprintf(outputStream, "Content-Type: %s\r\n", type);
        bytes += fprintf(outputStream, "Content-Length: %d\r\n\r\n", contentLength);
        if (content)
            bytes += fprintf(outputStream, "%s\r\n", content);
    }
    fflush(outputStream);
    
    // log the request
    logInfo->status = code;
    logInfo->sizeOfResponse = contentLength;
    logRequest(logInfo);
    
    // if fpp is not null then close the stream
    // otherwise return the stream so someone else can use it later
    if (fpp)
        *fpp = outputStream;
    else
        fclose(outputStream);
    return bytes;
}


void reply_cgi(char *program, int fd, struct logInformation *logInfo)
{
    int bytes = 0;
    int thePipe[2];
    int originalStandardInput;
    FILE *fpInput;
    FILE *fpOutput;
    int character;
    int *buffer;
    
    if (pipe(thePipe) == -1)
    {
        perror("Could not make a pipe!\n");
        return;
    }    
    int pid = fork();
    switch (pid)
    {
        case -1: 
            perror(program);
            break;
        case 0:    // child process
            close(thePipe[0]);
            if (dup2(thePipe[1], 1) == -1)
            {
                perror("Could not redirect stdout");
                exit(1);
            }
            char *path = malloc(strlen(rootDirectory) + strlen(CGI_PATH) + strlen(program) + 1);
            strcpy(path, rootDirectory);
            strcat(path, CGI_PATH);
            strcat(path, program);
            //printf("cgi search path = %s\n", path);
            execl(path, program, NULL);
            perror(program);
            free(path);
            exit(1);
            break;
        default: // parent process
            //printf("Pipe reading fd = %d\nPipe writing fd = %d\n", thePipe[0], thePipe[1]);
            close(thePipe[1]);
            originalStandardInput = dup(0);
            if (dup2(thePipe[0], 0) == -1)
            {
                perror("Could not redirect stdin");
                break;
            }
            close(thePipe[0]);
            wait(NULL);
            
            fpInput = fdopen(0, "r");
            // read from the pipe
            // count the number of bytes that will be sent
            if (fpInput)
            {
                // we are going to need to save what we read from the pipe because we will 
                // not be able to read it again. Allocate an initial buffer and increase it 
                // everytime we need to add something 
                buffer = malloc(sizeof(int)); 
                int i = 0;
                while ((character = getc(fpInput)) != EOF)
                {
                    buffer[i] = character;
                    i++;
                    buffer = realloc(buffer, (i+1) * sizeof(int));
                    bytes++;
                }     
                // allocate an EOF character to the end of the buffer   
                buffer = realloc(buffer, (i+1) * sizeof(int));
                buffer[i] = EOF;  
                bytes += httpReply(fd, &fpOutput, 200, "OK", "text/plain", NULL, NULL, bytes, logInfo);
                if (fpOutput)
                {
                    i = 0;
                    while (buffer[i] != EOF)
                    {
                        putc(buffer[i], fpOutput);
                        i++;
                    }            
                    fclose(fpOutput);
                }
                fclose(fpInput);
            }

            
            // reset stdin to original file descriptor
            if (dup2(originalStandardInput, 0) == -1)
                perror("Could not reset stdin to original file descriptor");
            
            // update the bytes sent statistics
            updateBytesSent(bytes); 
    } 
}


void reply_notImplemented(int fd, struct logInformation *logInfo)
{
    char message[] = "That command is not implemented on this system.";
    int bytes = httpReply(fd, NULL, 501, "Not Implemented", "text/plain", message, NULL, sizeof(message), logInfo);
    
    // update the bytes sent statistics
    updateBytesSent(bytes);
}


void reply_404(char *item, int fd, struct logInformation *logInfo)
{
    char message[] = "Could not find the item you requested.";
    int bytes = httpReply(fd, NULL, 404, "Not Found", "text/plain", message, NULL, sizeof(message), logInfo);
    
    // update the bytes sent statistics
    updateBytesSent(bytes);
}


int isDirectory(char *fileName)
{
    struct stat info;
    return (stat(fileName, &info) != -1 && S_ISDIR(info.st_mode));
}


int doesNotExist(char *fileName)
{
    struct stat info;
    return (stat(fileName, &info) == -1);
}


void reply_ls(char *directory, int fd, struct logInformation *logInfo)
{
    const TIME_STRING_LENGTH = 30;
    DIR *directoryPointer;
    struct dirent *direntPointer;
    FILE *fp;
    int bytes = 0;
    int numberEntries = 0;
    struct dirent **directoryEntryPointerArray;   
    char timeString[TIME_STRING_LENGTH]; 
    
    if ((directoryPointer = opendir(directory)) != NULL)
    {
        // count the number of bytes to be sent while simultaneously counting the number of entries
        while (direntPointer = readdir(directoryPointer))
        {
            char *pointer = direntPointer->d_name;
            if (*pointer != '.')
            {
                numberEntries++;
                while (*pointer != '\0')
                {
                    pointer++;
                    bytes++;
                }
            }
        }
        
        bytes += sizeof("Listing directory") + sizeof(directory); // count bytes in beginning of response too
        
        rewinddir(directoryPointer);

        // allocate memory for an array to hold the directory entries. We will need this for sorting and arrangining into columns
        // MENTAL CHECK: Is the memory set free later? --> YES
        directoryEntryPointerArray = malloc(numberEntries * sizeof(struct dirent *));
        
        // populate the directory entry pointer array
        int i = 0;
        while (direntPointer = readdir(directoryPointer))
        {
          if (*(direntPointer->d_name) != '.')
          {
            directoryEntryPointerArray[i] = direntPointer;
            i++;
          }
        }   
                 
        // sort the directory entry pointer array
        sortDirectoryEntries(numberEntries, directoryEntryPointerArray);         
    
        // get the last modified time for the directory
        struct stat info;
        stat(directory, &info);
        strncpy(timeString, asctime(gmtime(&(info.st_mtime))), TIME_STRING_LENGTH - 1); // get the Greenwich Mean Time
        char *timeStringPointer = timeString;
        // step through the time string until we find the newline character and 
        // change it to a null character to remove it
        while (*timeStringPointer != '\n')
        {
            timeStringPointer++;
        }
        *timeStringPointer = '\0';     
    
        // send the http status code and header
        bytes += httpReply(fd, &fp, 200, "OK", "text/plain", NULL, timeString, bytes, logInfo);
        bytes += fprintf(fp, "Listing directory %s\n", directory);
        
        // print the directory entries
        for (i = 0; i < numberEntries; i++)
        {
            fprintf(fp, "%s\n", directoryEntryPointerArray[i]->d_name);
        } 
        
        closedir(directoryPointer);
    }        
    
    // close the file pointer
    fclose(fp);  
    
    // update the bytes sent statistics
    updateBytesSent(bytes);
    
    // free memory
    free(directoryEntryPointerArray);
}
 

void sortDirectoryEntries(int numberDirectoryEntries, struct dirent *directoryEntryPointerArray[])
{
  int (*comparisonFunction)(const void *,const void *);
  comparisonFunction = compareDirectoryEntriesAlphabetically;
  
  qsort(directoryEntryPointerArray, numberDirectoryEntries, sizeof(struct dirent *), comparisonFunction);
}


int compareDirectoryEntriesAlphabetically(const void *entry1, const void *entry2)
{
    return (strcmp((*(struct dirent **)entry1)->d_name, (*(struct dirent **)entry2)->d_name));
}


char *getFileNameExtension(char *fileName)
{
    char *cp;
    if ((cp = strrchr(fileName, '.')) != NULL)
        return cp+1;
    else
        return "";
}


void reply_cat(char *fileName, int fd, struct logInformation *logInfo)
{
    const TIME_STRING_LENGTH = 30;
    char *type;
    char *extension = getFileNameExtension(fileName);
    FILE *fpSocket;
    FILE *fpLocal;
    int character;
    int bytes = 0;
    char timeString[TIME_STRING_LENGTH];
    
    if (strcmp(extension, "html") == 0)
        type = "text/html";
    else if (strcmp(extension, "gif") == 0)
        type = "image/gif";
    else if (strcmp(extension, "jpg") == 0)
        type = "image/jpeg";
    else if (strcmp(extension, "jpeg") == 0)
        type = "image/jpeg";
    else
        type = "text/plain";
        
    fpSocket = fdopen(fd, "w");
    fpLocal = fopen(fileName, "r");
    if (fpSocket != NULL && fpLocal != NULL)
    {
        // get the last modified time for the file
        struct stat info;
        stat(fileName, &info);
        strncpy(timeString, asctime(gmtime(&(info.st_mtime))), TIME_STRING_LENGTH - 1); // get the Greenwich Mean Time
        char *timeStringPointer = timeString;
        // step through the time string until we find the newline character and 
        // change it to a null character to remove it
        while (*timeStringPointer != '\n')
        {
            timeStringPointer++;
        }
        *timeStringPointer = '\0'; 
        // count the number of bytes that will be sent
        while ((character = getc(fpLocal)) != EOF)
        {
            bytes++;
        }         
        rewind(fpLocal);    
        bytes += httpReply(fd, &fpSocket, 200, "OK", type, NULL, timeString, bytes, logInfo);
        while ((character = getc(fpLocal)) != EOF)
        {
            putc(character, fpSocket);
        }
        fclose(fpSocket);
        fclose(fpLocal);
    }
    
    // update the bytes sent statistics    
    updateBytesSent(bytes);
}


void updateBytesSent(int bytesSent)
{
    pthread_mutex_lock(&statisticsLock);
    bytesSentFromServer += bytesSent;
    pthread_mutex_unlock(&statisticsLock);
}


void updateNumberServerRequests()
{
    pthread_mutex_lock(&statisticsLock);
    numberServerRequests++;
    pthread_mutex_unlock(&statisticsLock);    
}


void reply_status(int fd, struct logInformation *logInfo)
{
    FILE *fp;
    
    httpReply(fd, &fp, 200, "OK", "text/plain", NULL, NULL, 0, logInfo);
    
    pthread_mutex_lock(&statisticsLock);
    fprintf(fp, "Server started: %s", ctime(&timeServerStarted));
    fprintf(fp, "Total requests: %d\n", numberServerRequests);
    fprintf(fp, "Bytes sent out: %d\n\n", bytesSentFromServer);
    fclose(fp);
    pthread_mutex_unlock(&statisticsLock);
}


void logRequest(struct logInformation *logInfo)
{
    fprintf(logFile, "%s %s %s %d %d\n", logInfo->IPaddress, logInfo->timeRequestReceived, logInfo->quotedFirstLineOfRequest, logInfo->status, logInfo->sizeOfResponse);
    fflush(logFile);
}


int daemon_init(void)
{
    pid_t pid;
    if ((pid = fork()) < 0)
        return -1;
    else if (pid != 0)
    {
        printf("Created a daemon process!\n");
        exit(0); // close the parent process
    }
    setsid(); // becomes the session leader
    umask(0); // clear the file creation mask
    return(0);    
}

