#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <thread>
#include <vector>

using namespace std;

const int ECHOPORT = 5599;
const int BUFFSIZE = 1000;
const int SERVER_KEY = 54621;
const int CLIENT_KEY = 45328;



void thrdFunc(int c_sockfd);


int main()
{

    // int x = 935398;
    // cout << x / 100000;

    int my_sockfd;
    sockaddr_in my_addr;
    sockaddr_in rem_addr;

    my_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(my_sockfd == -1) {
        cout << "Could not created socket\n";
        exit(1);
    }

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(ECHOPORT);

    if(bind(my_sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
        cout << "Bind error\n";
        close(my_sockfd);
        exit(1);
    }

    if(listen(my_sockfd, 10) == -1) {
        cout << "Listen error\n";
        close(my_sockfd);
        exit(1);
    }

    cout << "Welcome to my freacking server!\n";

    vector<thread> threads;
    socklen_t rem_addr_len;
    int c_sockfd;

    while(1) {
        rem_addr_len = sizeof(rem_addr);
        c_sockfd = accept(my_sockfd, (struct sockaddr *)&rem_addr, &rem_addr_len);
        threads.emplace_back( thread(thrdFunc, c_sockfd) );
        cout << "accepted new client\n";
    }

    close(c_sockfd);

    return 0;
}

void thrdFunc(int c_sockfd)
{
    int mlen;
    char buf[BUFFSIZE];

    while (1) {
        if ((mlen = read(c_sockfd, buf, BUFFSIZE)) == -1) {
            perror("read error");
            break;
        }
        printf("Client #%d: %s\n", c_sockfd, buf);

        if (send(c_sockfd, buf, mlen, 0) == -1) {
            perror("write error");
            break;
        }

        printf("sending %i bytes back to the client\n", mlen);
    }
}