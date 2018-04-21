#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <thread>
#include <vector>

using namespace std;

const int BUFFSIZE = 1000;

#define SERVER_MOVE "102 MOVE\\a\\b"
#define SERVER_TURN_LEFT "103 TURN LEFT\\a\\b"
#define SERVER_TURN_RIGHT "104 TURN RIGHT\\a\\b"
#define SERVER_PICK_UP "105 GET MESSAGE\\a\\b"
#define SERVER_LOGOUT "106 LOGOUT\\a\\b"
#define SERVER_OK "200 OK\\a\\b"
#define SERVER_LOGIN_FAILED "300 LOGIN FAILED\\a\\b"
#define SERVER_SYNTAX_ERROR "301 SYNTAX ERROR\\a\\b"
#define SERVER_LOGIC_ERROR "302 LOGIC ERROR\\a\\b"

#define SERVER_LOGIN_FAILED_LEN 22
#define SERVER_OK_LEN 12

class CServer
{
public:
    CServer() { setUpSocket(); }

private:
    const int ECHOPORT = 5599;
    int m_my_sockfd;
    sockaddr_in m_my_addr;
    sockaddr_in m_rem_addr;


    void setUpSocket()
    {
        m_my_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(m_my_sockfd == -1) {
            cout << "Could not created socket\n";
            exit(1);
        }

        bzero(&m_my_addr, sizeof(m_my_addr));
        m_my_addr.sin_family = AF_INET;
        m_my_addr.sin_port = htons(ECHOPORT);

        if(bind(m_my_sockfd, (struct sockaddr *)&m_my_addr, sizeof(m_my_addr)) == -1) {
            cout << "Bind error\n";
            close(m_my_sockfd);
            exit(1);
        }
    }
};


class CAuthentication
{
public:
    CAuthentication(int c_sockfd)
            :m_serverHash(0), m_clientHash(0), m_client(c_sockfd)
    {}

    bool auth()
    {
        int messLen;
        char buf[BUFFSIZE];

        cout << "Waiting for username...\n";
        //getting username
        if ((messLen = read(m_client, buf, BUFFSIZE)) == -1) {
            perror("read error");
            return false;
        }

        unsigned short int hash = makeHash(buf, messLen);
        unsigned short int confirmationCode = (hash + SERVER_KEY) % 65536;

        putCodeIntoBuffer(buf, confirmationCode);

//            printf("Client #%d: %s\n", c_sockfd, buf);

        //sending code to the client
        if (write(m_client, buf, 5) == -1) {
            perror("write error");
            return false;
        }

        unsigned short int expected = (hash + CLIENT_KEY) % 65536;
        cout << "expecting code: " << expected << endl;

        //getting client's response
        if ((messLen = read(m_client, buf, BUFFSIZE)) == -1) {
            perror("read error");
            return false;
        }

        char response[5];
        response[0] = buf[0];
        response[1] = buf[1];
        response[2] = buf[2];
        response[3] = buf[3];
        response[4] = buf[4];

        unsigned short int clientConfirmationCode = (unsigned short)atoi(response);
        cout << "client confirmation code: " << clientConfirmationCode << endl;
        // unsigned short int expected = (hash + CLIENT_KEY) % 65536;
        // return expected == clientConfirmationCode;

        return expected == clientConfirmationCode;

        // printf("sending %i bytes back to the client\n", messLen);
    }

private:
    const unsigned short int CLIENT_KEY = 45328;
    const unsigned short int SERVER_KEY = 54621;
    unsigned short int m_serverHash;
    unsigned short int m_clientHash;
    int m_client;
    char m_outgoing[5];

    void putCodeIntoBuffer(char * buf, unsigned short int hash)
    {
        buf[0] = (char)(hash / 4);
        buf[1] = (char)((hash / 3) % 10);
        buf[2] = (char)((hash / 2) % 10);
        buf[3] = (char)((hash / 1) % 10);
        buf[4] = (char)(hash % 10);
    }

    unsigned short int makeHash(const char* buf, int messLen)
    {
        int usernameLen = messLen - 4;
        unsigned short int hash = 0;
        for(int i = 0; i < usernameLen; i++) {
            hash += buf[i];
        }
        hash = (hash * 1000) % 65536;
        return hash;
    }
};

void thrdFunc(int c_sockfd);

int main()
{

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

    close(my_sockfd);

    return 0;
}

void thrdFunc(int c_sockfd)
{
    int mlen;
    char buf[BUFFSIZE];

    CAuthentication auth(c_sockfd);

    if(!auth.auth()) {
        if (write(c_sockfd, SERVER_LOGIN_FAILED, SERVER_LOGIN_FAILED_LEN) == -1)
            perror("write error");
        close(c_sockfd);
        return;
    } else
    if (write(c_sockfd, SERVER_OK, SERVER_OK_LEN) == -1)
        perror("write error");

    cout << "Authentication was successful\n";

    while (1) {
        if ((mlen = read(c_sockfd, buf, BUFFSIZE)) == -1) {
            perror("read error");
            break;
        }

        if(mlen == 0)
            break;

        printf("Client #%d: %s\n", c_sockfd, buf);

        if (write(c_sockfd, SERVER_MOVE, mlen) == -1) {
            perror("write error");
            break;
        }

        printf("sending %i bytes back to the client\n", mlen);
    }
    close(c_sockfd);
}
