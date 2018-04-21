#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <thread>
#include <vector>

using namespace std;


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

class ComunicationException{};

class CServer
{
public:
    CServer()
    {
        setUpSocket();
        run();
    }

private:
    const int ECHOPORT = 5599;
    const int BUFFSIZE = 1000;
    const unsigned short CLIENT_KEY = 45328;
    const unsigned short SERVER_KEY = 54621;
    int m_my_sockfd;
    sockaddr_in m_my_addr;
    sockaddr_in m_rem_addr;


    void setUpSocket();
    void run();
    void clientRoutine(int c_sockfd);
    bool authenticate(int c_sockfd);
    unsigned short makeHash(const char * buff, int mLen);
    void putCodeIntoBuffer(char * buff, unsigned short code);
    void send(int c_sockfd, const char * message, size_t mlen) const;
};

void CServer::setUpSocket()
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

void CServer::run()
{
    if(listen(m_my_sockfd, 10) == -1) {
        cout << "Listen error\n";
        close(m_my_sockfd);
        exit(1);
    }

    cout << "Welcome to my freacking server!\n";

    vector<thread> threads;
    socklen_t rem_addr_len;
    int c_sockfd;

    while(1) {
        rem_addr_len = sizeof(m_rem_addr);
        c_sockfd = accept(m_my_sockfd, (struct sockaddr *)&m_rem_addr, &rem_addr_len);
        threads.emplace_back( thread(&CServer::clientRoutine, this, c_sockfd) );
        cout << "accepted new client\n";
    }

    close(m_my_sockfd);
}

void CServer::clientRoutine(int c_sockfd)
{
    int mlen;
    char buf[BUFFSIZE];

    if(!authenticate(c_sockfd)) {
        send(c_sockfd, SERVER_LOGIN_FAILED, SERVER_LOGIN_FAILED_LEN);
        close(c_sockfd);
        return;
    } else
        send(c_sockfd, SERVER_OK, SERVER_OK_LEN);

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

bool CServer::authenticate(int c_sockfd)
{
    int messLen;
    char buf[BUFFSIZE];

    cout << "Waiting for username...\n";
    //getting username
    if ((messLen = read(c_sockfd, buf, BUFFSIZE)) == -1) {
        perror("read error");
        return false;
    }

    unsigned short hash = makeHash(buf, messLen);
    unsigned short confirmationCode = (hash + SERVER_KEY) % 65536;

    putCodeIntoBuffer(buf, confirmationCode);

//            printf("Client #%d: %s\n", c_sockfd, buf);

    //sending code to the client
    if (write(c_sockfd, buf, 5) == -1) {
        perror("write error");
        return false;
    }

    unsigned short expected = (hash + CLIENT_KEY) % 65536;
    cout << "expecting code: " << expected << endl;

    //getting client's response
    if ((messLen = read(c_sockfd, buf, BUFFSIZE)) == -1) {
        perror("read error");
        return false;
    }

    char response[5];
    response[0] = buf[0];
    response[1] = buf[1];
    response[2] = buf[2];
    response[3] = buf[3];
    response[4] = buf[4];

    unsigned short clientConfirmationCode = (unsigned short)atoi(response);
    cout << "client confirmation code: " << clientConfirmationCode << endl;
//         unsigned short int expected = (hash + CLIENT_KEY) % 65536;

    return expected == clientConfirmationCode;
}

unsigned short CServer::makeHash(const char * buff, int mLen)
{
    int usernameLen = mLen - 4;
    unsigned short hash = 0;
    for(int i = 0; i < usernameLen; i++) {
        hash += buff[i];
    }
    hash = (hash * 1000) % 65536;
    return hash;
}

void CServer::putCodeIntoBuffer(char *buff, unsigned short code)
{
    buff[0] = (char)(code / 4);
    buff[1] = (char)((code / 3) % 10);
    buff[2] = (char)((code / 2) % 10);
    buff[3] = (char)((code / 1) % 10);
    buff[4] = (char)(code % 10);
}

void CServer::send(int c_sockfd, const char * message, size_t mlen) const
{
    if (write(c_sockfd, message, mlen) == -1) {
        perror("write error");
        throw ComunicationException();
    }

}

int main()
{

    CServer server;

    return 0;
}
