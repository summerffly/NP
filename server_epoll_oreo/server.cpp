
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

#include <sys/epoll.h>


//------------------------------//
//   MACRO
//------------------------------//
#define PORT  7788
#define BUFFSIZE  256
#define CLIENTMAX  256   // epoll最大回传事件数


//------------------------------//
//   Global Variables
//------------------------------//
//char retbuffer[BUFFSIZE];


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

    /********** epoll IO复用 **********/
    struct epoll_event ev;
    struct epoll_event events[CLIENTMAX];

    // tips 番茄@20200301 - 在Linux新的内核版本中，参数已被遗弃
    int epfd = epoll_create(CLIENTMAX);

    ev.data.fd = sock_fd;
    ev.events = EPOLLIN | EPOLLET;  // tips 番茄@20200301 - 工作方式为边沿触发
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);

    while(1)
    {
        int nfds = epoll_wait(epfd, events, CLIENTMAX, 1000);
        for(int i = 0; i < nfds; i++)
        {
            if( events[i].data.fd == sock_fd )
            {
                /********** 接收客户端请求 **********/
                struct sockaddr_in clnt_addr;
                socklen_t clnt_addr_size = sizeof(clnt_addr);
                memset(&clnt_addr, 0, sizeof(clnt_addr));

                int csock_fd = accept(sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
                        
                ev.data.fd = csock_fd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, csock_fd, &ev);
                        
                printf("------------------------------\r\n");
                printf("Client Connect Success\r\n");
                printf("------------------------------\r\n");

                /********** 向client发送数据 **********/
                char strhello[] = "\r\n>>> Server Connect Success\r\n>>> Hello Summer\r\n";
                write(csock_fd, strhello, strlen(strhello)+1);
            }
            else
            {
                char buffer[BUFFSIZE];
                int retr = 0;
                int retw = 0;

                retr = read(events[i].data.fd, buffer, sizeof(buffer)-1);
                if(retr == 0)
                {
                    printf("---------------\r\nClient Exit\r\n---------------\r\n");

                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);

                    break;
                }
                        
                printf("message recv ret: %d\r\n", retr);
                printf("message form client: %s", buffer);

                retw = write(events[i].data.fd, buffer, strlen(buffer)+1);
                printf("message send ret: %d\r\n", retw);
                printf("****************************************\r\n");
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
