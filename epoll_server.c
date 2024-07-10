#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/epoll.h>

#define SERVER_PORT (7778)
#define EPOLL_MAX_NUM (2048)
#define BUFFER_MAX_SIZE (4096)

char buffer[BUFFER_MAX_SIZE];

int main(int argc, char **argv){
    int listen_fd = 0;
    int client_fd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;//unsigned int

    int epfd = 0;
    struct epoll_event event, *my_events;

    //get socket fd
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    //bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    //listen
    listen(listen_fd, 10);

    epfd = epoll_create(EPOLL_MAX_NUM);
    if(epfd < 0){
        perror("epoll create");
        goto END;
    }
    //listen_fd -> epoll
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) < 0){
        perror("epoll error");
        goto END;
    }

    my_events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_MAX_NUM);
    while(1){
        int active_fds_cnt = epoll_wait(epfd, my_events, EPOLL_MAX_NUM, -1);
        int i = 0;
        for(i = 0; i <active_fds_cnt; i++){
            if(my_events[i].data.fd == listen_fd){
                client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                if(client_fd < 0){
                    perror("client error");
                    continue;
                }
                char ip[20];
                printf("new connect [%s : %d\n", inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip)), ntohs(client_addr.sin_port));
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
            }
            else if(my_events[i].events & EPOLLIN){
                printf("EPOLLIN\n");
                client_fd = my_events[i].data.fd;
                buffer[0] = '\0';
                int n = read(client_fd, buffer, BUFFER_MAX_SIZE);
                if(n < 0){
                    perror("read");
                    continue;
                }
                else if(n == 0){
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, &event);
                    close(client_fd);
                }
                else{
                    printf("[read]: %s\n", buffer);
                    buffer[n] = '\0';
                    write(client_fd, buffer, strlen(buffer));
                    memset(buffer, 0, BUFFER_MAX_SIZE);
                }
            }else if(my_events[i].events & EPOLLOUT){
                printf("epollout\n");
            }
        }
    }
END:
    close(epfd);
    close(listen_fd);
    return 0;

}