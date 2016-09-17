#include "utility.h"


class client
{
private:
    struct sockaddr_in serverAddr;
    int sock ; //通信套接字
    int pipe_fd[2]; //管道

    struct epoll_event events[2];

    int epfd;
    bool isClientwork;//表示客户端是否正常
private:
    void InitClient();
public:
    client();

    void RunClient();

};

client::client()
{
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr.s_addr);

    isClientwork = true;
    this->InitClient();
}

void client::InitClient()
{
    //create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > sock)
        throw "socket error";
    if(connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        throw "connect error";
    //create pipe
    if(pipe(pipe_fd) < 0)
        throw "pipe error";

    //create epoll
    epfd = epoll_create(EPOLL_SIZE);
    if(0 > epfd)
        throw "epoll_create error";

    //套接字和读描述符加入树中
    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true); //read



}

void client::RunClient()
{
    //信息缓冲区
    char message[BUF_SIZE];


    //fork
    int pid = fork();
    if(pid < 0) //创建子进程失败
    {
        throw "fork error";
    }
    else if(0 == pid)//子进程
    {
        //关闭读,向管道中写入数据
        close(pipe_fd[0]);
        printf("Please input 'exit' to exit the chat room\n");

        while(isClientwork)
        {
            bzero(&message, BUF_SIZE);
            //从键盘得到内容
            fgets(message, BUF_SIZE, stdin);

            if(strncasecmp(message, EXIT, strlen(EXIT)) == 0)
            {
                isClientwork = false;
            }
            else
            {
                //向管道中写入数据
                if(write(pipe_fd[1], message, strlen(message) - 1) < 0)
                {
                    perror("write error");
                    exit(-1);
                }
            }
        }

    }
    else//父进程
    {
        //关闭管道的写端
        close(pipe_fd[1]);

        while(isClientwork)
        {
            int epoll_events_count = epoll_wait(epfd, events, 2, -1);

            for(int i = 0; i < epoll_events_count; ++i)
            {
                bzero(&message, BUF_SIZE);

                //服务端发过来的信息
                if(events[i].data.fd == sock)
                {
                    int ret = recv(sock, message, BUF_SIZE, 0);
                    if(ret == 0 )
                    {
                        kill(pid, SIGINT);
                        printf("Server closed connect:%d \n", sock);
                        shutdown(sock, SHUT_RDWR);
                        isClientwork = false;
                    }
                    else
                    {
                        printf("%s\n", message);
                    }

                }
                else //子进程写入
                {
                    int ret = read(events[i].data.fd, message, BUF_SIZE);

                    //ret = 0
                    if(ret == 0)isClientwork = false;
                    else
                    {
                        //向服务端发送数据
                        send(sock, message, BUF_SIZE, 0);
                    }
                }

            }//end for
        }//end while

    }// end else


    if(pid)//父
    {
        close(pipe_fd[0]);
        shutdown(sock, SHUT_RDWR);
        wait(NULL);
    }
    else //子
    {
        sleep(10);
        close(pipe_fd[1]);
    }

}


int main(int argc, char *argv[])
{

    try
    {
        client *c = new client();
        c->RunClient();
        delete c;
    }
    catch(const char *str)
    {
        cout << str << endl;
    }

    return 0;
}
