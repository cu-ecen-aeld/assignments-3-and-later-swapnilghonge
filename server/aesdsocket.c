/* Assignment -5 part1 
 * Author : Swapnil Ghonge
 * Details: This files contains fucntions which coorelate with aesdsocket.c.
 */
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>


#define TOTAL_MSG	3	
#define ERROR		1
#define SUCCESS		0
#define PORT		9000


int sockfd = 0;
char *sendbuffer = NULL;
int acceptfd;
int fd = 0;
sigset_t mask;

//filename and path in one variable
const char *filepath = "/var/tmp/aesdsocketdata";

 void closeall(void)
{
	if(sendbuffer != NULL)	
		free(sendbuffer);

	if(acceptfd)
		close(accept);

	if(sockfd)
		close(sockfd);

	if(fd)	
	{
		close(fd);
		remove(filepath);
	}
	
	closelog();	
}

void signal_handler(int signo)
{

	syslog(LOG_DEBUG, "in handler\n");
	
	if(signo == SIGINT)
		syslog(LOG_DEBUG, "Caught signal SIGINT, exiting\n");
	else
		syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting\n");

	closeall();

	exit(EXIT_SUCCESS);
	
}




int main(int argc, char *argv[])
{
	openlog(NULL, LOG_CONS, LOG_USER);

	struct sockaddr_in saddr;
	char buffer[50] = {0};
	int nbytes = 0;
	ssize_t nr = 0;
	unsigned int total_bytes = 0;
	off_t location =0;
	int daemon=0;

	
    if ((argc == 2) && (strcmp("-d", argv[1])==0)) 
		daemon = 1;

	
	if(signal(SIGINT, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Cannot handle SIGINT!\n");
		exit(EXIT_FAILURE);
	}

	
	if(signal(SIGTERM, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Cannot handle SIGTERM!\n");
		exit(EXIT_FAILURE);
	}

	
    if (sigemptyset(&mask) == -1) 
	{
        syslog(LOG_ERR, "creating empty signal set failed");
        exit(EXIT_FAILURE);
    }
	
    if (sigaddset(&mask, SIGINT) == -1) 
	{
        syslog(LOG_ERR, "Adding SIGINT failed");
        exit(EXIT_FAILURE);
    }
	
    if (sigaddset(&mask, SIGTERM) == -1) 
	{
        syslog(LOG_ERR, "Adding SIGTERM failed");
        exit(EXIT_FAILURE);
    }



	// setting the arguements for socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		syslog(LOG_ERR, "Error in socket\n");
		exit(EXIT_FAILURE);
	}

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(PORT);

	// calling bind function
	int bindstatus = bind(sockfd, (struct sockaddr *) &saddr, sizeof saddr);
	if (bindstatus == -1)
	{
		syslog(LOG_ERR, "socket binding failed\n");
		closeall();
		exit(EXIT_FAILURE);
	}
	
	// calling listen function
	bindstatus = listen(sockfd, 0);
	if (bindstatus == -1)
	{
		syslog(LOG_ERR, "socket listening failed\n");
		closeall();
		exit(EXIT_FAILURE);
	}
	
	socklen_t len = sizeof(struct sockaddr);

	//create a file on the file
	fd = open(filepath, O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0777);
	
	if (fd == -1)	
	{
		syslog(LOG_ERR, "can't open or create file '%s'\n", filepath);
		closeall();
		exit(EXIT_FAILURE);
	}
	
	syslog(LOG_DEBUG, "opened or created file '%s' successfully\n", filepath);

	
	
	// start daemon
	if (daemon) 
	{
        	pid_t pid = fork();
        	if (pid == -1) 
		{
            		syslog(LOG_ERR, "Error in creating a child process");
			closeall();
            		exit(EXIT_FAILURE);
        	}
        
        	else if (pid != 0)
		{
            	exit(EXIT_SUCCESS);
        	}
		/*create a new session and process group*/	
		if (setsid() == -1) 
		{
			syslog(LOG_ERR, "cannot create a new session");
			closeall();
			exit(EXIT_FAILURE);
		}
		/*set the working directory to the root directory*/
		if (chdir("/") == -1) 
		{
			syslog(LOG_ERR, "cannot change directory");
			closeall();
			exit(EXIT_FAILURE);
		}
		/*redirect fd's 0,1,2 to dev/null*/
		open("/dev/null", O_RDWR);
		dup(0);	/*stdin*/
		dup(0); /*stdout*/
		dup(0);	/*stderror*/

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
   	}
	
	while(1)
	{
		/*Accept command*/
		
		acceptfd = accept(sockfd, (struct sockaddr *) &saddr, &len);
		if (acceptfd == -1)
		{
			syslog(LOG_ERR, "socket accepting failed\n");
			closeall();
			exit(EXIT_FAILURE);
		}

		syslog(LOG_DEBUG, "Accepted connection from '%s'\n", inet_ntoa((struct in_addr)saddr.sin_addr));
	

		//Signals from set are added to invoking process's signal mask
        	if (sigprocmask(SIG_BLOCK, &mask, NULL)) 
		{
            syslog(LOG_ERR, "sigprocmask");
			closeall();
            exit(EXIT_FAILURE);
        }

		do
		{
			memset(buffer, 0, sizeof(buffer));
			
			nbytes = recv(acceptfd, buffer, sizeof(buffer), 0);

			if (nbytes)
			{
				
				if(nbytes > strlen(buffer))
					nr = write(fd, buffer, strlen(buffer));
				else
					nr = write(fd, buffer, nbytes);
					
				if (nr == -1)	
				{//if error
					syslog(LOG_ERR, "can't write received string in file '%s'\n", filepath);
					break;	
				}
				total_bytes += nr;
			}
			else
				break;

		}while(strchr(buffer, '\n') == NULL);
		
				
		nbytes=0;
		lseek(fd, 0, SEEK_SET);
		
		sendbuffer = (char *)malloc(sizeof(char) * (total_bytes+location));
		if (sendbuffer == NULL)
			syslog(LOG_ERR, "malloc failed\n");
		
		memset(sendbuffer, 0, sizeof(sendbuffer));
		nr = read(fd, sendbuffer, total_bytes+location);
		if (nr != total_bytes+location)	
		{//if error
			syslog(LOG_ERR, "can't read proper bytes from file '%s'\n", filepath);
			break;	
		}
		
		//syslog(LOG_DEBUG, "'%s'\n", sendbuffer);
		nbytes = send(acceptfd, sendbuffer, nr, 0);
		if(nbytes != nr)
		{
			syslog(LOG_ERR, "not all bytes sent\n");
			break;
		}
		
		total_bytes = 0;
		
		location=lseek(fd, 0, SEEK_END);

		syslog(LOG_DEBUG, "Closed connection from '%s'\n", inet_ntoa((struct in_addr)saddr.sin_addr));


		//Signals from set are added to invoking process's signal mask
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL)) 
		{
            syslog(LOG_ERR, "sigprocmask");
			closeall();
            exit(EXIT_FAILURE);
        }
	
		free(sendbuffer);
		close(acceptfd);
	}

	remove(filepath);
	close(sockfd);
	close(fd);
	
	closelog();
	return 0;
}



