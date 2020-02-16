
//------------------------------//
//   Coded by 番茄
//   @summer studio
//------------------------------//
// tips 番茄@20200216
// pthread库不是 Linux 系统默认的库
// 编译链接需要使用静态库libpthread.a
// g++ -o server server.cpp -lpthread
//------------------------------//


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>


//------------------------------//
//   MACRO
//------------------------------//
#define PORT  7788
#define BUFFSIZE  256
#define THREADNUM  3


//------------------------------//
//   STRUCT
//------------------------------//
typedef struct
{
    int sock_fd;
    int thread_index;
}thread_argv;


//------------------------------//
//   Global Variables
//------------------------------//
char retbuffer[BUFFSIZE];

pthread_mutex_t ALOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_t a_thread[THREADNUM];


//------------------------------//
//   Function Declaration
//------------------------------//
void sig_int_handler(int sig);

void *handle_client(void* arg);


//------------------------------//
//   MAIN FUNC ENTRY
//------------------------------//
int main(int argc, char *argv[])
{
    int reterror = 0;
    errno = 0;

    /********** 处理信号 **********/
    struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

    act.sa_handler = sig_int_handler;
	sigaction(SIGINT, &act, 0);

    /********** 创建socket **********/
    int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_fd < 0)
    {
        printf("Failed to create socket: %d\n", sock_fd);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        return -1;
    }

    // tips 番茄@20200115
    // 设置套接字选项，允许端口重用
    // 否则受TIME_WAIT影响bind()失败
    int optvalue = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));
    
    /********** 将socket和IP&port绑定 **********/
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));   // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    reterror = bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));   // sockaddr结构体强制转换
    if(reterror < 0)
    {
        printf("Failed to bind socket: %d\n", reterror);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        return -1;
    }
    
    /********** 进入监听状态，等待用户发起请求 **********/
    reterror = listen(sock_fd, 5);
    if(reterror < 0)
    {
        printf("Failed to listen socket: %d\n", reterror);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        return -1;
    }

    int i = 0;
    for(i = 0; i<THREADNUM; i++)
    {
        thread_argv targv;
        targv.sock_fd = sock_fd;
        targv.thread_index = i;

        reterror = pthread_create(&a_thread[i], NULL, handle_client, (void *)&targv);
        if( 0 != reterror )
        {
            printf("Failed to create thread: %d\n", reterror);
            printf("Error ID-%d: %s\n", errno, strerror(errno));
            perror("ErrorLog");
            exit(-1);
        }

        usleep(1000);
    }

    void *thread_result = (char *)malloc(BUFFSIZE);
    
    for(i = 0; i<THREADNUM; i++)
    {
        printf("*** Waiting for Thread-%d Join\r\n", i);
        reterror = pthread_join(a_thread[i], &thread_result);
        if( 0 != reterror )
        {
            printf("Failed to join thread: %d\n", reterror);
            printf("Error ID-%d: %s\n", errno, strerror(errno));
            perror("ErrorLog");
            exit(-1);
        }
        else
        {
            printf("*** Thread-%d Result: %s\r\n", i, (char *)thread_result);
        }
    }

    /********** 关闭socket **********/
    close(sock_fd);
}


//------------------------------//
//   SIGINT处理函数
//------------------------------//
void sig_int_handler(int sig)
{
    printf("\r\n---------------\r\n");
	printf("Got signal: %d\r\n", sig);
    printf("Summer Exit\r\n---------------\r\n");

    exit(0);
}


//------------------------------//
//   处理client连接函数
//------------------------------//
void *handle_client(void* arg)
{
    thread_argv targv = *((thread_argv *)arg);
    
    // tips 番茄@20170906
    // 千万不要返回临时变量指针
    // 线程结束后内存将被回收
    //char retbuffer[BUFFSIZE];

    while(1)
    {
        /********** 接收客户端请求 **********/
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        memset(&clnt_addr, 0, sizeof(clnt_addr));

        printf("### Thread-%d Waiting for Mutex\r\n", targv.thread_index);
        pthread_mutex_lock(&ALOCK);   /*** 进入互斥区 ***/
        printf("### Thread-%d Waiting for Socket-Accpept\r\n", targv.thread_index);
        int csock_fd = accept(targv.sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        pthread_mutex_unlock(&ALOCK);   /*** 离开互斥区 ***/

        if(csock_fd < 0)
        {
            // tips 番茄@20200121
            // 当阻塞于某个慢系统调用的一个进程捕获某个信号且相应处理函数返回时
            // 该系统调用可能返回一个EINTR错误
            //if(errno == EINTR)
                //continue;
                
            printf("Failed to accept client socket: %d\n", csock_fd);
            printf("Error ID-%d: %s\n", errno, strerror(errno));
            perror("ErrorLog");

            sprintf(retbuffer, "Thread-%d Exit -1", targv.thread_index);
            pthread_exit(retbuffer);
        }
        else
        {
            printf("### Thread-%d Connect Client Success\r\n", targv.thread_index);

            char buffer[BUFFSIZE];
            int retr = 0;
            int retw = 0;

            printf("------------------------------\r\n");
            printf("Client Connect Success\r\n");
            printf("------------------------------\r\n");
                
            /********** 向client发送数据 **********/
            char strhello[] = "\r\n>>> Server Connect Success\r\n>>> Hello Summer\r\n";
            write(csock_fd, strhello, strlen(strhello)+1);

            while(1)
            {
                retr = read(csock_fd, buffer, sizeof(buffer)-1);
                if(retr == 0)
                {
                    printf("---------------\r\nClient Exit\r\n---------------\r\n");
                    break;
                }
                
                printf("message recv ret: %d\r\n", retr);
                printf("message form client: %s", buffer);

                retw = write(csock_fd, buffer, strlen(buffer)+1);
                printf("message send ret: %d\r\n", retw);
                printf("****************************************\r\n");
            }

            /********** 关闭socket **********/
            close(csock_fd);
        }
    }

    /********** 退出thread **********/
    sprintf(retbuffer, "Thread-%d Exit 0", targv.thread_index);
    pthread_exit(retbuffer);
}


//------------------------------//
//   River flows in summer
//------------------------------//
