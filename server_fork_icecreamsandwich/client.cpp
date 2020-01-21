
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

#define PORT  7788
#define BUFFSIZE  256


void siginthandler(int sig)
{
    printf("---------------\r\n");
	printf("Got signal: %d\r\n", sig);
    printf("Summer Exit\r\n---------------\r\n");

    exit(0);
	//(void) signal(sig, SIG_DFL);
}

int main(int argc, char *argv[])
{
    int reterror = 0;
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
	act.sa_handler = siginthandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);

    /********** 创建socket **********/
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
    {
        printf("Failed to create socket: %d\n", sockfd);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        exit(-1);
    }

    /********** 连接目标server(IP&port) **********/
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));   // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(PORT);
    reterror = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(reterror < 0)
    {
        printf("Failed to connect socket: %d\n", reterror);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        exit(-1);
    }
   
    /********** 读取server回传的数据 **********/
    char recvbuffer[BUFFSIZE];
    char sendbuffer[BUFFSIZE];
    read(sockfd, recvbuffer, sizeof(recvbuffer)-1);
    printf("message form server: %s\n", recvbuffer);

    while(1)
    {
        printf("input: ");
        memset(&sendbuffer, 0, sizeof(sendbuffer));
        //gets(sendbuffer);
        fgets(sendbuffer, BUFFSIZE, stdin);
        write(sockfd, sendbuffer, sizeof(sendbuffer)-1);
        
        int ret = read(sockfd, recvbuffer, sizeof(recvbuffer)-1);
        if(ret == 0)
        {
            printf("---------------\r\nServer shutdown\r\n---------------\r\n");
            exit(-1);
        }
        printf("message form server: %s\n", recvbuffer);
    }
    
    /********** 关闭socket **********/
    close(sockfd);
    
    return 0;
}

//------------------------------//
//   River flows in summer
//------------------------------//
