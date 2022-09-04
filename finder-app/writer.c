/* Assignment -2 Intro to System Programming
 * Author : Swapnil Ghonge
 * Details: This files contains fucntions which coorelate with writer.sh				
*/
#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<string.h>
#include<unistd.h>
#include<syslog.h>


int main(int argc, char*argv[])
{
	openlog(NULL, 0, LOG_USER);
	
	//Error Checking for input of number of arguements
	if (argc !=3)
	{
		syslog(LOG_ERR,"Wrong number of arguemnts your input arguments are %d", argc);
		
		return 1;
		
	}
	//Error checking for null string
	if (*argv[2] =='\0')
	{
		syslog(LOG_ERR, "Invalid String");
	}

	else
	{
		syslog(LOG_DEBUG,"File created successfully: %s",argv[1]);
		
		int fd, write_string;
		fd = creat(argv[1],0777);
		
		write_string= write(fd,argv[2], strlen(argv[2]));
		
		if(write_string ==-1)
		{
			syslog(LOG_ERR,"Error in string");
		}
		
		syslog(LOG_DEBUG,"Writing %s to file %s", argv[2],argv[1]);
	closelog();
	return 0; 
}

}

