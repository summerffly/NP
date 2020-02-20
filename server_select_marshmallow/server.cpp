
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

#include <sys/select.h>


//------------------------------//
//   MACRO
//------------------------------//
#define PORT  7788
#define BUFFSIZE  256
#define THREADNUM  3


//------------------------------//
//   Global Variables
//------------------------------//
char retbuffer[BUFFSIZE];


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

    /********** select **********/
    int connect_clinet[10] = {0};
    int connect_num = 0;

    int maxfd = -1;
    fd_set sockdf_set;
    FD_ZERO(&sockdf_set);
    FD_SET(sock_fd, &sockdf_set);
    maxfd = sock_fd;

    while(1)
    {
        // tag 番茄@20200220 - 单客户端连接正常，多客户端连接异常，需要解决
        int ret = select(maxfd+1, &sockdf_set, NULL, NULL, NULL);
        switch(ret)
        {
            case 0:
                break;
            case -1:
                break;
            default:
                if(FD_ISSET(sock_fd, &sockdf_set))
                {
                    /********** 接收客户端请求 **********/
                    struct sockaddr_in clnt_addr;
                    socklen_t clnt_addr_size = sizeof(clnt_addr);
                    memset(&clnt_addr, 0, sizeof(clnt_addr));

                    int csock_fd = accept(sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

                    FD_SET(csock_fd, &sockdf_set);
                    connect_clinet[connect_num] = csock_fd;
                    connect_num++;
                    if(csock_fd > maxfd)
                        maxfd = csock_fd;
                    
                    printf("------------------------------\r\n");
                    printf("Client Connect Success\r\n");
                    printf("------------------------------\r\n");

                    /********** 向client发送数据 **********/
                    char strhello[] = "\r\n>>> Server Connect Success\r\n>>> Hello Summer\r\n";
                    write(csock_fd, strhello, strlen(strhello)+1);
                }
                for(int i = 0; i < connect_num; i++)
                {
                    if(FD_ISSET(connect_clinet[i], &sockdf_set))
                    {
                        char buffer[BUFFSIZE];
                        int retr = 0;
                        int retw = 0;

                        retr = read(connect_clinet[i], buffer, sizeof(buffer)-1);
                        if(retr == 0)
                        {
                            printf("---------------\r\nClient Exit\r\n---------------\r\n");
                            FD_CLR(connect_clinet[i], &sockdf_set);
                            close(connect_clinet[i]);
                            break;
                        }
                        
                        printf("message recv ret: %d\r\n", retr);
                        printf("message form client: %s", buffer);

                        retw = write(connect_clinet[i], buffer, strlen(buffer)+1);
                        printf("message send ret: %d\r\n", retw);
                        printf("****************************************\r\n");
                    }
                }
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
//   River flows in summer
//------------------------------//
