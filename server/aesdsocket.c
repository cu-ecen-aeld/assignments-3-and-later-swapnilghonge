/* Author: Swapnil Ghonge
 * Assignment: Assignment 6 Part1 
 * Refernces: 	https://www.geeksforgeeks.org/socket-programming-cc/
 *		https://beej.us/guide/bgnet/html/#sockaddr_inman
 *		https://beej.us/guide/bgnet/html/#sockaddr_inman
 *		https://www.csd.uoc.gr/~hy556/material/tutorials/cs556-3rd-tutorial.pdf
 *		https://man7.org/linux/man-pages/man2/bind.2.html
 *		https://man7.org/linux/man-pages/man3/strftime.3.html
 *		https://askubuntu.com/questions/1009266/pylint3-and-pip3-broken
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <syslog.h>
#include <net/if.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/queue.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

int file_fd;
int sockfd;
pthread_mutex_t lock;
// structure for node
typedef struct node
{
    pthread_t thread;
    bool thr_comp_stat;
    char ip_client[INET6_ADDRSTRLEN];
    int clientfd;
    
    TAILQ_ENTRY(node) entries;
}thread_data_t;

typedef TAILQ_HEAD(head_s, node) head_t;
head_t head;
// stamping the time with a new line
// RFC 2822 compliant strftime format,
void time_stamp()
{
    
    time_t clock_time;
    // buffer to  count number of bytes
    char time_buf[100];
    time(&clock_time);
    
    // strftime - format date and time
    strftime(time_buf, sizeof(time_buf), "timestamp: %Y-%m-%d %H:%M:%S\r\n", localtime(&clock_time));
    
    printf("%s", time_buf);
    
    lseek(file_fd, 0, SEEK_END);
    
    //count the number bytes written
    int cnt_byt = strlen(time_buf);
    int cnt_write_byt = write(file_fd, time_buf, cnt_byt);
    
    if (cnt_write_byt != cnt_byt)
    {
        printf("write of bytes Unsuccessful\n");
        
    }
    
}
// free the memory of head and tail pointer
void memory_free()
{
    thread_data_t *temp;
    while(!TAILQ_EMPTY(&head))
    {
        temp = TAILQ_FIRST(&head);
        TAILQ_REMOVE(&head, temp, entries);
        free(temp);
    }
}
// Signal Handler of the signal received
void signal_hanlder(int signum)
{
    printf("Caught signal %d\n", signum);
    if (signum == SIGALRM)
    {
        printf("SIGALRM ALERT\n");
        time_stamp();
        alarm(10);
    }
    
    if ((signum == SIGINT) || (signum == SIGTERM)) 
    {
        printf("Caught signal %d\n", signum);
        
        close(file_fd);
        
        close(sockfd);
        remove("/var/tmp/aesdsocketdata");
        
        memory_free();
        exit (0);
    }
}
void * thread_function(void* thread_param)
{
    thread_data_t* data = (thread_data_t *)thread_param;
    bool pak_compl_stat = false;
    bool stat_realloc = false;
    char read_data, write_data;
    char *write_buf = (char*)malloc(sizeof(char));
    int byte_rec = 0;
    data->thr_comp_stat=false;
   
   // waiting untill packet sattus us complete 
    while(!pak_compl_stat)
    {
        if(stat_realloc)
        {
            write_buf = realloc(write_buf, byte_rec+1);
            stat_realloc = false;
        }
        
        // receive bytes using recv
        int recv_byt = recv(data->clientfd, &write_data, 1, 0);
        if(recv_byt < 1)
        {
            printf("Error receiving bytes\n");
            perror("recv error\n");
        }
        
        if(recv_byt == 1)
        {
            stat_realloc = true;
            *(write_buf + byte_rec) = write_data;
            byte_rec++;
        }
        if(write_data == '\n'){
            pak_compl_stat = true;
        }
    }

     
    pthread_mutex_lock(&lock);

        // write bytes in file 
    int write_bytes = write(file_fd, write_buf, byte_rec);
    if(write_bytes != byte_rec)
    {
        printf("write bytes failed\n");
        perror("write bytes failed\n");
    } 
    printf("write bytes success\n");       
         
    lseek(file_fd, 0, SEEK_SET);

    // waiting for read file status
    while(read(file_fd, &read_data, 1) != 0)
    {
        int sent_status = send(data->clientfd, &read_data, 1, 0);
        if (sent_status == 0)
        {
            printf("send failed\n");
        }
    }
    printf("send bytes complete\n");
    pak_compl_stat = false;

    
    pthread_mutex_unlock(&lock);

    // close connection
    int close_fd = close(data->clientfd);
    if(close_fd == 0){
        syslog(LOG_DEBUG, "Closed connection from %s\n", data->ip_client);
    }

    
    byte_rec = 0;
    free(write_buf);
    data->thr_comp_stat=true;
    return thread_param;
}

int main(int argc, char **argv) {

    
    signal(SIGINT, signal_hanlder);
    signal(SIGTERM, signal_hanlder);
    signal(SIGALRM, signal_hanlder);
    

    struct sockaddr_in servadd, clientadd;
    socklen_t clientfd;
   

    // socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error in Socket\n");
        
    }
    memset((void *) &servadd, 0, sizeof(servadd));
    servadd.sin_family = AF_INET;
    servadd.sin_port = htons(9000);
    servadd.sin_addr.s_addr = INADDR_ANY;

    
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("Error in setsockopt");
        exit(1);
    }

   // binding 
    int sock = bind(sockfd, (struct sockaddr *) &servadd, sizeof(struct sockaddr));
    
    if (sock == -1) 
    {
        close(sockfd);
        
        perror("Error binding \n");
        
    }

    
    if (argc > 1) 
    {
    	 
        if (!strcmp(argv[1], "-d")) {
            
            /*create a process*/
            pid_t pid = fork();

            if (pid < 0) 
            {
                
                exit(-1);
            } 
            else if (pid == 0) 
            {
            	/*create a new session and prcess group*/
            	
                if (setsid() < 0) 
                {
                    
                    exit(-1);
                }
		/*set the working directory to the root directory*/
                if (chdir("/") == -1) {
                   
                    exit(-1);
                }
                /*redirect fd's 0,1,2 to /dev/null*/
                open("/dev/null", O_RDWR); /*stdin*/
                dup(0); 	/*stdout*/
                dup(0);
                printf("Daemon created\n");
               
            } 
            else 
            {
                exit(0);
            }
        }
    }

    //printf("opening /var/tmp/aesdsocketdata\n");
    
    // open
    file_fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_CREAT | O_TRUNC, 0777);

    if (file_fd == -1) {
        perror("Error in opening file descriptor.");
    }
   
    
   
    pthread_mutex_init(&lock, NULL);

    TAILQ_INIT(&head);

    int listenfd;
    bool alarm_flag = false;

    // listen
    listenfd = listen(sockfd, 5);
     if (listenfd == -1) 
     {
        perror("Error in Listening \n");
       
    }
   
    while (1) {
        thread_data_t *data = (thread_data_t *) malloc(sizeof(thread_data_t));
        clientfd = sizeof(clientadd);
        data->clientfd = accept(sockfd, (struct sockaddr *) &clientadd, &clientfd);

        if (data->clientfd == -1) {
            perror("Error in accepting\n");
            return -1;
        }
        
        inet_ntop(clientadd.sin_family,(struct sockaddr *) &clientadd, data->ip_client, sizeof(data->ip_client));
        syslog(LOG_DEBUG, "Accepted a connection from %s\n", data->ip_client);  

        
        pthread_create(&(data->thread), NULL, &thread_function, (void *)data);

        TAILQ_INSERT_TAIL(&head, data, entries);
        
        data = NULL;

        if (!alarm_flag) {
            alarm_flag = true;
            printf("alarm set\n");
            alarm(10);
        }

        thread_data_t *entry = NULL;
        TAILQ_FOREACH(entry, &head, entries)
        {
            printf("check for the pthread join\n");
            pthread_join(entry->thread, NULL);
            if (entry->thr_comp_stat) {
                TAILQ_REMOVE(&head, entry, entries);
                free(entry);
                break;
            }
        }
        
    }
    // closing the operation
    close(sockfd);
    close(file_fd);
}
