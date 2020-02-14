
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

#define PORT  7788
#define BUFFSIZE  256


//------------------------------//
//   Function Declaration
//------------------------------//
void sig_int_handler(int sig);
void sig_chld_handler(int sig);


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

	act.sa_handler = sig_chld_handler;
	sigaction(SIGCHLD, &act, 0);

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
    
    /********** 接收客户端请求 **********/
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while(1)
    {
        memset(&clnt_addr, 0, sizeof(clnt_addr));
        int csock_fd = accept(sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(csock_fd < 0)
        {
            // tips 番茄@20200121
            // 当阻塞于某个慢系统调用的一个进程捕获某个信号且相应处理函数返回时
            // 该系统调用可能返回一个EINTR错误
            if(errno == EINTR)
                continue;
            
            printf("Failed to accept client socket: %d\n", csock_fd);
            printf("Error ID-%d: %s\n", errno, strerror(errno));
            perror("ErrorLog");
            return -1;
        }

        /********** Fork 父进程 **********/
        if( fork() > 0 )
        {
            close(csock_fd);
        }
        /********** Fork 子进程 **********/
        else
        {
            close(sock_fd);

            printf("\r\n------------------------------\r\nClient connect in success\r\n------------------------------\r\n");
        
            /********** 向client发送数据 **********/
            char strhello[] = "\r\n---------------\r\nHello Summer\r\n---------------\r\n";
            write(csock_fd, strhello, strlen(strhello)+1);

            char buffer[BUFFSIZE];
            while(1)
            {
                int retr = read(csock_fd, buffer, sizeof(buffer)-1);
                if(retr == 0)
                {
                    printf("---------------\r\nClient exit\r\n---------------\r\n");
                    break;
                }
                
                printf("message recv ret: %d\r\n", retr);
                printf("message form client: %s", buffer);

                int retw = write(csock_fd, buffer, strlen(buffer)+1);
                printf("message send ret: %d\r\n", retw);
                printf("****************************************\r\n");
            }

            /********** 关闭socket **********/
            close(csock_fd);
            exit(0);
        }
    }
}

//------------------------------//
//   SIGINT处理函数
//------------------------------//
void sig_int_handler(int sig)
{
    printf("---------------\r\n");
	printf("Got signal: %d\r\n", sig);
    printf("Summer Exit\r\n---------------\r\n");

    exit(0);
	//(void) signal(sig, SIG_DFL);
}

//------------------------------//
//   SIGCHLD处理函数
//------------------------------//
void sig_chld_handler(int sig)
{
    pid_t pid;
    int stat;
    while( (pid=waitpid(-1, &stat, WNOHANG)) > 0 )
        printf("child server %d terminated with status %d\r\n", pid, stat);

    return;
}


//------------------------------//
//   River flows in summer
//------------------------------//
