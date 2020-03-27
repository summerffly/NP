
//------------------------------//
//   Coded by 番茄
//   @summer studio
//------------------------------//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>


//------------------------------//
//   MACRO
//------------------------------//
#define PORT  7788
#define BUFFSIZE  256


//------------------------------//
//   Global Variables
//------------------------------//
int sockfd = 0;

int retsend = 0;
int retrecv = 0;
char sendbuffer[BUFFSIZE];
char recvbuffer[BUFFSIZE];


//------------------------------//
//   Function Declaration
//------------------------------//
void sig_int_handler(int sig);


//------------------------------//
//   MAIN FUNC ENTRY
//------------------------------//
int main(int argc, char *argv[])
{
    int reterror = 0;
    pthread_t a_thread;
    errno = 0;

    if(argc != 2)
    {
        printf("illegal argument\r\n");
        exit(-1);
    }   
    else
        printf("argument %d - IP: %s\r\n", argc, argv[1]);

    /********** 处理SIGINT信号 **********/
    struct sigaction act;
	act.sa_handler = sig_int_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);

    /********** 创建socket **********/
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd < 0)
    {
        printf("Failed to create socket: %d\n", sockfd);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        exit(-1);
    }

    /********** 填充目标server(IP&port) **********/
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));   // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(PORT);

    /********** 向server发送数据 **********/
    struct sockaddr_in udp_addr;
    socklen_t addrlen = sizeof(udp_addr);

    while(1)
    {
        memset(&sendbuffer, 0, sizeof(sendbuffer));
        memset(&recvbuffer, 0, sizeof(recvbuffer));

        fgets(sendbuffer, BUFFSIZE, stdin);
        printf("message length: %lu\r\n", strlen(sendbuffer));

        retsend = sendto(sockfd, sendbuffer, strlen(sendbuffer)+1, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if(retsend > 0)
        {
            printf("message send ret: %d\r\n", retsend);

            retrecv = recvfrom(sockfd, recvbuffer, BUFFSIZE, 0, (struct sockaddr*)&udp_addr, &addrlen);
            printf("message send ret: %d\r\n", retrecv);
            printf("message form server: %s", recvbuffer);
            printf("****************************************\r\n");
        }       
    }
    
    /********** 关闭socket **********/
    close(sockfd);
    
    return 0;
}

//------------------------------//
//   SIGINT处理函数
//------------------------------//
void sig_int_handler(int sig)
{
    printf("\r\n---------------\r\n");
	printf("Got signal: %d\r\n", sig);
    printf("Summer Exit\r\n---------------\r\n");

	//(void) signal(sig, SIG_DFL);
    exit(0);
}


//------------------------------//
//   River flows in summer
//------------------------------//
