
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
#include <netinet/in.h>

#define PORT  7788


int main(int argc, char *argv[])
{
    int reterror = 0;
    //errno = 0;

    /********** 创建socket **********/
    int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_fd < 0)
    {
        printf("Failed to create socket: %d\n", sock_fd);
        perror("summerLog");
        return -1;
    }

    // 设置套接字选项，允许你端口重用，否则受TIME_WAIT影响bind()失败
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
        perror("summerLog");
        return -1;
    }
    
    /********** 进入监听状态，等待用户发起请求 **********/
    reterror = listen(sock_fd, 5);
    if(reterror < 0)
    {
        printf("Failed to listen socket: %d\n", reterror);
        perror("summerLog");
        return -1;
    }
    
    /********** 接收客户端请求 **********/
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    int csock_fd = accept(sock_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    
    /********** 向client发送数据 **********/
    char str[] = "Hello Summer";
    write(csock_fd, str, sizeof(str));
   
    /********** 关闭socket **********/
    close(csock_fd);
    close(sock_fd);
    
    return 0;
}

//------------------------------//
//   River flows in summer
//------------------------------//
