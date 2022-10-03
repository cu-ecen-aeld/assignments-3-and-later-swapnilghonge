/* Author: Swapnil Ghonge
 * Assignment: Assignment 5 Part1 
 * Refernces: 	https://www.geeksforgeeks.org/socket-programming-cc/
 *		https://beej.us/guide/bgnet/html/#sockaddr_inman
 *		https://beej.us/guide/bgnet/html/#sockaddr_inman
 *		https://www.csd.uoc.gr/~hy556/material/tutorials/cs556-3rd-tutorial.pdf
 *		https://man7.org/linux/man-pages/man2/bind.2.html
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <syslog.h>
#include <net/if.h>

#define MYPORT 9000
#define BUF_SIZE 1024


int file_fd;
int sockfd;
int yes=1;

void signal_hanlder(int signum)
{
    printf("Caught signal %d", signum);
    if ((signum == SIGINT) || (signum == SIGTERM)) {
        close(file_fd);
        close(sockfd);
        remove("/var/tmp/aesdsocketdata");
        exit(0);
    }
}

int main(int argc, char **argv)
{

    signal(SIGINT, signal_hanlder);
    signal(SIGTERM, signal_hanlder);
    int rec;
    struct sockaddr_in servadd, clientadd;
    socklen_t client_len;
    int clientfd;
    
    //Calling the socket function to establish connection
   
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Error in socket\n");
        return -1;
    }
    
    
    memset((void *)&servadd, 0, sizeof(servadd));
    servadd.sin_family = AF_INET;
    servadd.sin_port = htons(MYPORT);
    servadd.sin_addr.s_addr = INADDR_ANY;

    
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) 
    {
    	perror("setsockopt error");
    	exit(1);
    }


    
    int sock = bind(sockfd, (struct sockaddr *)&servadd, sizeof(struct sockaddr));
    if (sock == -1)
    {
        close(sockfd);
        perror("Error binding \n");
        return -1;
    }

    if(argc > 1)
    {
        if(!strcmp(argv[1],"-d"))
        {
            
            /*create a process*/
            pid_t pid = fork();

    		if (pid < 0) 
    		{
    			
        		exit (-1);
    		}
    		else if (pid == 0)
    		{
    			/*create a new session and prcess group*/
    			if (setsid() < 0) 
    			{
        			
        			exit (-1);
        		}
    			/*set the working directory to the root directory*/
    			if(chdir("/") == -1)
    			{
    				perror("error changing directory");
    				exit (-1);
    			} 
    			/*redirect fd's 0,1,2 to /dev/null*/
    			open("/dev/null", O_RDWR); /*stdin*/
    			dup(0);				/*stdout*/
    			dup(0);
    			
    			printf("Daemon created\n");
    			
    		}
            else 
            {
                exit (0);
            }
        }
    }

    
    int listenfd;
    listenfd = listen(sockfd, 5);
    printf("Listening successful\n");

    if (listenfd == -1)
    {
        perror("Error in Listening process \n");
        return -1;
    }

    
    /*file_fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP | S_IROTH | S_IWOTH, 0777);*/
    file_fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_CREAT | O_TRUNC, 0777);
    
    if (file_fd == -1)
    {
        perror("Error in opening file descriptor.");
    }
    
    int file_size = 0;
    
    char receive_buffer[BUF_SIZE];
    
    while(1)
    {
        client_len = sizeof(clientadd);
        clientfd = accept(sockfd, (struct sockaddr *)&clientadd, &client_len);
        if (clientfd == -1)
        {
            perror("error accepting\n");
            return -1;
        }
        printf("Accepted connection from %s\n",inet_ntoa(clientadd.sin_addr));
        syslog(LOG_DEBUG,"Accepted connection from %s\n",inet_ntoa(clientadd.sin_addr));  

        bool status = false;
        int buffer = BUF_SIZE;
        int buffer_size = 0;
        char *write_buf = malloc(sizeof(char) * buffer);

        while(!status)
        {
            // receive the data from client
            rec = recv(clientfd, receive_buffer, BUF_SIZE, 0);
            if (rec == 0)
            {
                status = true;
            } 
            for (int i = 0; i < rec; i++)
            {
                
                // appneding new line
 		if (receive_buffer[i] == '\n')
                {
                    status = true;
                }
            }
            if ((buffer - buffer_size) < rec)
            {
                buffer = buffer +rec;
                write_buf = (char *) realloc(write_buf, sizeof(char) * buffer);
            }
            memcpy(write_buf + buffer_size, receive_buffer, rec);
            buffer_size += rec;
        }
        
        if(status)
        {
            status = false;
          // writing bytes to file
            int write_bytes = write(file_fd, write_buf, buffer_size);
            if (write_bytes == -1)
            {
                perror("error writing to file");
                return -1;
            }
            file_size = file_size+ write_bytes;

            lseek(file_fd, 0, SEEK_SET);

            char *read_buf = (char *) malloc(sizeof(char) * file_size);

           // reading bytes
            ssize_t read_bytes = read(file_fd, read_buf, file_size);
            if (read_bytes == -1)
            {
                perror("Error in read of file");
                return -1;
            }
		// send bytes
            int send_bytes = send(clientfd, read_buf, read_bytes, 0);
            if (send_bytes == -1)
            {
                perror("Error in sending data to client");
                return -1;
            }

            // freeing memory in after using the buffer.
            free(read_buf);
            free(write_buf);

        }
        close(clientfd);
        syslog(LOG_DEBUG,"Closed connection from %s\n",inet_ntoa(clientadd.sin_addr));  
    }
    
    close(sock);
    return 0;
}


