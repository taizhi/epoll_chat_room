#include "utility.h"


class server
{
private:
    struct sockaddr_in serverAddr;//服务器端地址初始化
    int listenfd ; //监听套接字
    struct epoll_event events[EPOLL_SIZE]; //事件数组
    int epfd;
private:
    //初始化
    void initserver();
public:
    
    server();
    //运行
    void runserver();
    ~server();
};


server::server()
{
    this->serverAddr.sin_family = AF_INET;
    this->serverAddr.sin_port = htons(SERVER_PORT);
    this->serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    this->initserver();
}

server::~server()
{
    close(listenfd);
    close(epfd);
}

void server::initserver()
{
    //创建监听socket
    this->listenfd = socket(AF_INET,SOCK_STREAM,0);
    if(0 > this->listenfd)
        throw "socket error";

    cout << "socket created ok" << endl;

    int opt = 1;
    setsockopt(this->listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    signal(SIGPIPE,SIG_IGN);

     //绑定
    if(0 > bind(this->listenfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)))
        throw "bind error";

    
     //监听
    if(0 > listen(this->listenfd,SOMAXCONN))
      throw "listen error";

    cout << "start to listen " << SERVER_IP << endl;
        
}

void server::runserver()
{   
    //内核中创建红黑树,返回用户态句柄
    this->epfd = epoll_create(EPOLL_SIZE);
    if(0 > epfd)
        throw "epoll_create error";
     //监听套接字加入树中
     addfd(epfd,this->listenfd,true);    

      //主循环
    while(1)
    {
        //阻塞等待
        int epoll_events_count = epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if(0 > epoll_events_count)
            throw "epoll error";
        //处理
        for(int i = 0;i < epoll_events_count;++i)
        {
            int sockfd = events[i].data.fd;
                        
            //有新的客户端连接
            if(sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t clen = sizeof(client_address);
                //从完成3次握手队列中取出
                int clientfd = accept(listenfd,(struct sockaddr*)&client_address,&clen);
                printf("client connect from :%s :%d,clientfd = %d\n",inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port),clientfd);
                //join tree
                addfd(epfd,clientfd,true);
                // list save socket
                clients_list.push_back(clientfd);
                printf("Now there are %d clients int the chat room\n", (int)clients_list.size());
                char message[BUF_SIZE];
                bzero(message,BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                //欢迎新的客户端加入
                int ret = send(clientfd, message, BUF_SIZE, 0);
                if(ret < 0) 
                throw "send error";
            }
            else 
            {
                int ret = sendBroadcastmessage(sockfd);
                if(0 > ret)
                throw "sendBroadcastmessage  error";
            }


        }

    }

}

int main(int argc, char *argv[])
{
    try
    {
        server *s = new server();
        s->runserver();
        delete s;
    }
    catch(const char *str)
    {
        cout << str << endl;
    }
	return 0;
}
