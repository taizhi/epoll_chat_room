#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

using namespace std;

//服务端存储所有在线用户socket, 便于广播信息
list<int> clients_list;

/**************************** macro defintion  ***************************************/

//server ip
#define SERVER_IP "127.0.0.1"

//server port
#define SERVER_PORT 8888

//epoll size
#define EPOLL_SIZE 5000

//message buffer size 63k
#define BUF_SIZE 0xFFFF


#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"

#define SERVER_MESSAGE "ClientID %d say >> %s"

// exit
#define EXIT "EXIT"

#define CAUTION "There is only one int the char room!"

/**************************** some function  ***************************************/

/* -------------------------------------------*/
/**
 * @brief  setnonblocking 设置非阻塞
 *
 * @param sockfd
 *
 * @return 0
 */
/* -------------------------------------------*/
int setnonblocking(int sockfd)
{
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
    return 0;
}

/* -------------------------------------------*/
/**
 * @brief  addfd 加入内核红黑树中
 *
 * @param epollfd
 * @param fd
 * @param enable_et true,epoll use ET,other epoll use LT
 */
/* -------------------------------------------*/
void addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et)
        ev.events = EPOLLIN | EPOLLET;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);

    setnonblocking(fd);

    cout << "fd added to epoll!" << endl;
}

/* -------------------------------------------*/
/**
 * @brief  sendBroadcastmessage
 *
 * @param clientfd
 *
 * @return len
 */
/* -------------------------------------------*/
int sendBroadcastmessage(int clientfd)
{
    char buf[BUF_SIZE], message[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);

    cout << "read from client(clientID = " << clientfd << ")" << endl;

    int len = recv(clientfd, buf, BUF_SIZE, 0);

    if(0 == len)
    {
        close(clientfd);
        clients_list.remove(clientfd);
        cout << "clientID = " << clientfd << "closed .\n now there are " << (int)clients_list.size() << " client in the char room " << endl;
    }
    else
    {
        //当只有一个客户端的时候
        if(1 == clients_list.size())
        {
            send(clientfd, CAUTION, strlen(CAUTION), 0);
            return len;
        }

        //format message
        sprintf(message, SERVER_MESSAGE, clientfd, buf);

        list<int>::iterator it;
        for(it = clients_list.begin(); it != clients_list.end(); ++it)
        {
            if(*it == clientfd)
                continue;

            if(send(*it, message, BUF_SIZE, 0) < 0)
            {
                perror("error");
                exit(-1);
            }

        }

    }
    return len;
}

#endif
