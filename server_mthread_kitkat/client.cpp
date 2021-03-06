
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

int recvstatus = 0;

pthread_mutex_t ALOCK = PTHREAD_MUTEX_INITIALIZER;


//------------------------------//
//   Function Declaration
//------------------------------//
void sig_int_handler(int sig);
void *thread_func(void* arg);


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
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    /********** 创建线程 **********/
    reterror = pthread_create(&a_thread, NULL, thread_func, NULL);
    if( 0 != reterror )
    {
        printf("Failed to create thread: %d\n", reterror);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        exit(-1);
    }

    /********** 向server发送数据 **********/
    while(1)
    {
        printf("input: ");
        memset(&sendbuffer, 0, sizeof(sendbuffer));

        pthread_mutex_lock(&ALOCK);   /*** 进入互斥区 ***/
        recvstatus = 0;
        pthread_mutex_unlock(&ALOCK);   /*** 离开互斥区 ***/
        
        // tips 番茄@20200208
        // fgets()读取会将换行符 '\n' 保存
        // 然后剩余的空间都用 '\0' 填充
        fgets(sendbuffer, BUFFSIZE, stdin);
        printf("message length: %lu\r\n", strlen(sendbuffer));

        // tips 番茄@20200208
        // sizeof()计算数组长度，不可用
        // 改用strlen()，该函数不包含 '\0' 需要补充
        // int ret = write(sockfd, sendbuffer, sizeof(sendbuffer)-1);
        retsend = write(sockfd, sendbuffer, strlen(sendbuffer)+1);
        printf("message send ret: %d\r\n", retsend);

        pthread_mutex_lock(&ALOCK);   /*** 进入互斥区 ***/
        recvstatus = 1;
        pthread_mutex_unlock(&ALOCK);   /*** 离开互斥区 ***/

        while(recvstatus == 1)
            usleep(100);

        printf("message recv ret: %d\r\n", retrecv);
        printf("message form server: %s", recvbuffer);
        printf("****************************************\r\n");
        pthread_mutex_lock(&ALOCK);   /*** 进入互斥区 ***/
        recvstatus = 3;
        pthread_mutex_unlock(&ALOCK);   /*** 离开互斥区 ***/
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
//   THREAD处理函数
//------------------------------//
void *thread_func(void* arg)
{
    //fprintf(stdout,"Thread created Success\r\n");   // tips 番茄 - 输入输出串流

    while(1)
    {
        retrecv = read(sockfd, recvbuffer, sizeof(recvbuffer)-1);
        if(retrecv == 0)
        {
            printf("\r\n--------------------\r\nServer Shutdown\r\n--------------------\r\n");
            exit(-1);
        }
        else
        {
            if(recvstatus == 0)
            {
                printf("\r\nmessage recv ret: %d\r\n", retrecv);
                printf("message form server: %s", recvbuffer);
                printf("****************************************\r\n");
                printf("input: ");
                fflush(stdout);   // tips 番茄 - 遇到换行符才会输出，否则fflush强制输出
            }
            else if(recvstatus == 1)
            {
                pthread_mutex_lock(&ALOCK);   /*** 进入互斥区 ***/
                recvstatus = 2;
                pthread_mutex_unlock(&ALOCK);   /*** 离开互斥区 ***/
            }
        }
        
        //printf("message form server: %s\n", recvbuffer);
    }
    //pthread_exit("Thread Exit\r\n");
}


//------------------------------//
//   River flows in summer
//------------------------------//
