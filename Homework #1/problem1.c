
#include <stdio.h>	

#define COPY 0	//represents that elements still need to be copied
#define PAD  1  //represents that '\0' has been encountered and the rest of the array should be padded with '\0'

char* my_strncpy(char* destination, const char* source, size_t numCharsToCopy);

main()
{
	char message1[] = "Hello World!";
	char message2[] = "You are amazing!!!";
	
	printf("MESSAGE1: %s\n", message1);
	printf("MESSAGE2: %s\n", message2);

	my_strncpy(message2, message1, 10);
	
	printf("MESSAGE2 AFTER MY_STRNCPY(): %s\n", message2);


	return 0;
}

char* my_strncpy(char* destination, const char* source, size_t numCharsToCopy)
{
	int i;
	int state = COPY;
	
	for (i = 0; i < numCharsToCopy; i++)
	{
		if (state == COPY)
		{
			//copy the source to the destination
			destination[i] = source[i];

			if (source[i] == '\0')
			{
				//the end of the source string has been reached
				state = PAD; 
			}		
		} 
		else //state is PAD
		{
			//pad the remainder of the string with '\0' characters
			destination[i] = '\0';
		} 
	}

	return destination;	
}
