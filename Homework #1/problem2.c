
#include <stdio.h>

int my_strcmp(const char* cs, const char* ct);

main()
{
    char message1[] = "Hello World!";
    char message2[] = "Hello World!";
    char message3[] = "Dare to be different!";
    char message4[] = "Now is the time to act!";

    printf("Message 1: \"%s\"\n", message1);
    printf("Message 2: \"%s\"\n", message2);
    printf("Message 3: \"%s\"\n", message3);
    printf("Message 4: \"%s\"\n", message4);

    printf("Comparison of message 1 and 2: %d\n", my_strcmp(message1, message2));
    printf("Comparison of message 1 and 3: %d\n", my_strcmp(message1, message3));
    printf("Comparison of message 1 and 4: %d\n", my_strcmp(message1, message4));

    return 0;
}


int my_strcmp(const char* cs, const char* ct)
{
	int i = 0;

    //compare both strings until the end of one or both string has been reached
	while (cs[i] != '\0' && ct[i] != '\0')
	{
		if (cs[i] == ct[i])
		{
			i++;
		}
		else if (cs[i] < ct[i])
		{
			return -1;
		}
		else if (cs[i] > ct[i])
		{
			return 1;
		}
	}
	
	if (cs[i] == '\0' && ct[i] == '\0')
	{
        //the strings are equal
		return 0;
	}
	else if (cs[i] == '\0')
	{
        //cs[] comes before ct[]
		return -1;
	}
	else if (ct[i] == '\0')
	{
        //ct[] comes before cs[]
		return 1;
	}
}
