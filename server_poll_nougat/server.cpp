
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
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include <poll.h>


//------------------------------//
//   MACRO
//------------------------------//
#define PORT  7788
#define BUFFSIZE  256
#define CLIENTMAX  3


//------------------------------//
//   Global Variables
//------------------------------//
char retbuffer[BUFFSIZE];
bool bConnectFull = false;


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
    reterror = listen(sock_fd, CLIENTMAX);
    if(reterror < 0)
    {
        printf("Failed to listen socket: %d\n", reterror);
        printf("Error ID-%d: %s\n", errno, strerror(errno));
        perror("ErrorLog");
        return -1;
    }

    /********** poll IO复用 **********/
    // tips 番茄@20200301 - TCP连接数量，也是fds最后下标
    int connect_num = 0;

    struct pollfd fds[CLIENTMAX];
    int maxfd = -1;

    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    maxfd = sock_fd;

    while(1)
    {
        int ret = poll(fds, maxfd+1, -1);
        switch(ret)
        {
            case 0:   /*** 超时 ***/
                break;
            case -1:   /*** 错误 ***/
                break;
            default:
                if( (fds[0].revents & POLLIN) == POLLIN )
                {
                    /********** 接收客户端请求 **********/
                    if( connect_num == CLIENTMAX )
                    {
                        printf("------------------------------\r\n");
                        printf("Connect Full:  Keep Waiting...\r\n");
                        printf("------------------------------\r\n");

                        bConnectFull = true;

                        fds[0].fd = -1;
                        fds[0].revents = 0;

                        maxfd = 0;
                        for(int i=1; i<=connect_num; i++)
                        {
                            if(fds[i].fd > maxfd)
                            {
                                maxfd = fds[i].fd;
                            }
                        }
                    }
                    else
                    {
                        struct sockaddr_in clnt_addr;
                        socklen_t clnt_addr_size = sizeof(clnt_addr);
                        memset(&clnt_addr, 0, sizeof(clnt_addr));

                        int csock_fd = accept(sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
                        
                        connect_num++;
                        fds[connect_num].fd = csock_fd;
                        fds[connect_num].events = POLLIN;

                        if(csock_fd > maxfd)
                            maxfd = csock_fd;
                        
                        printf("------------------------------\r\n");
                        printf("Client Connect Success\r\n");
                        printf("------------------------------\r\n");

                        /********** 向client发送数据 **********/
                        char strhello[] = "\r\n>>> Server Connect Success\r\n>>> Hello Summer\r\n";
                        write(csock_fd, strhello, strlen(strhello)+1);
                    }
                }
                for(int i = 1; i <= connect_num; i++)
                {
                    if( (fds[i].revents & POLLIN) == POLLIN )
                    {
                        char buffer[BUFFSIZE];
                        int retr = 0;
                        int retw = 0;

                        retr = read(fds[i].fd, buffer, sizeof(buffer)-1);
                        if(retr == 0)
                        {
                            printf("---------------\r\nClient Exit\r\n---------------\r\n");

                            fds[i].fd = fds[connect_num].fd;
                            fds[i].events = POLLIN;

                            fds[connect_num].fd = -1;
                            fds[connect_num].revents = 0;

                            connect_num--;

                            if( bConnectFull == true )
                            {
                                bConnectFull = false;
                                
                                fds[0].fd = sock_fd;
                                fds[0].events = POLLIN;
                            }

                            maxfd = 0;
                            for(int i=0; i<=connect_num; i++)
                            {
                                if(fds[i].fd > maxfd)
                                {
                                    maxfd = fds[i].fd;
                                }
                            }

                            close(fds[i].fd);

                            break;
                        }
                        
                        printf("message recv ret: %d\r\n", retr);
                        printf("message form client: %s", buffer);

                        retw = write(fds[i].fd, buffer, strlen(buffer)+1);
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
