#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <queue>

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

#define CLIENT_USERNAME_LEN 12
#define CLIENT_CONFIRMATION_LEN 7
#define CLIENT_OK_LEN 12
#define CLIENT_RECHARGING_LEN 12
#define CLIENT_FULL_POWER_LEN 12
#define CLIENT_MESSAGE_LEN 100



class ComunicationException{};
class SyntaxError{};

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
    char m_buffer[1000];
    queue<string> m_commands;


    void setUpSocket();
    void run();
    void clientRoutine(int c_sockfd);
    bool authenticate(int c_sockfd);
    unsigned short makeHash();
    void putCodeIntoBuffer(unsigned short code);
    void sendMessage(int c_sockfd, const char *message, int mlen) const;
    int receiveMessage(int c_sockfd, size_t expectedLen);
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

    if(!authenticate(c_sockfd)) {
        sendMessage(c_sockfd, SERVER_LOGIN_FAILED, SERVER_LOGIN_FAILED_LEN);
        close(c_sockfd);
        return;
    } else
        sendMessage(c_sockfd, SERVER_OK, SERVER_OK_LEN);

    //robot creation here

    cout << "Authentication was successful\n";

    while (1) {

        try {
            mlen = receiveMessage(c_sockfd, CLIENT_USERNAME_LEN);
        }
        catch (ComunicationException e) {
            break;
        }


        if(mlen == 0)
            break;

        printf("Client #%d: %s\n", c_sockfd, m_buffer);

        try {
            sendMessage(c_sockfd, m_buffer, mlen);
        }
        catch (ComunicationException e) {
            break;
        }

        printf("sending %i bytes back to the client\n", mlen);
    }
    close(c_sockfd);
}

bool CServer::authenticate(int c_sockfd)
{
    int messLen;

    cout << "Waiting for username...\n";
    //getting username
    try {
        messLen = receiveMessage(c_sockfd, CLIENT_USERNAME_LEN);
    }
    catch (ComunicationException e) {
        return false;
    }
    catch(SyntaxError e) {
        throw SyntaxError();
    }

    unsigned short hash = makeHash();
    unsigned short confirmationCode = (hash + SERVER_KEY) % 65536;

    putCodeIntoBuffer(confirmationCode);

//            printf("Client #%d: %s\n", c_sockfd, buf);

    //sending code to the client
    try {
        sendMessage(c_sockfd, m_buffer, 5);
    }
    catch (ComunicationException e) {
        return false;
    }

    unsigned short expected = (hash + CLIENT_KEY) % 65536;
    cout << "expecting code: " << expected << endl;

    //getting client's response
    try {
        messLen = receiveMessage(c_sockfd, CLIENT_USERNAME_LEN);
    }
    catch (ComunicationException e) {
        return false;
    }

    char response[5];
    response[0] = m_buffer[0];
    response[1] = m_buffer[1];
    response[2] = m_buffer[2];
    response[3] = m_buffer[3];
    response[4] = m_buffer[4];

    unsigned short clientConfirmationCode = (unsigned short)atoi(response);
    cout << "client confirmation code: " << clientConfirmationCode << endl;
//         unsigned short int expected = (hash + CLIENT_KEY) % 65536;

    return expected == clientConfirmationCode;
}

unsigned short CServer::makeHash()
{
    unsigned short hash = 0;
    string username(m_commands.front());
    m_commands.pop();
    for(int i = 0; i < username.length(); i++) {
        hash += username[i];
    }
    hash = (hash * 1000) % 65536;
    return hash;
}

void CServer::putCodeIntoBuffer(unsigned short code)
{
    m_buffer[0] = (char)(code / 4);
    m_buffer[1] = (char)((code / 3) % 10);
    m_buffer[2] = (char)((code / 2) % 10);
    m_buffer[3] = (char)((code / 1) % 10);
    m_buffer[4] = (char)(code % 10);
}

void CServer::sendMessage(int c_sockfd, const char *message, int mlen) const
{
    if (send(c_sockfd, message, mlen, 0) == -1) {
        perror("write error");
        throw ComunicationException();
    }
}

int CServer::receiveMessage(int c_sockfd, size_t expectedLen)
{
    int mlen = 0;
    string tmpContainer;
    size_t found;

        if ((mlen = recv(c_sockfd, m_buffer, BUFFSIZE, 0)) == -1)
        {
            perror("read error");
            throw ComunicationException();
        }

        tmpContainer.append(m_buffer, mlen);

        while(true)
        {
            found = tmpContainer.find("\\a\\b");

            if(found != string::npos)
            {
                string tmpCommand(tmpContainer, 0, found);
                m_commands.push(tmpCommand);
                tmpContainer.erase(0, found + 4);
                if(!tmpContainer.empty())
                    continue;
                break;
            }
            else if(tmpContainer.size() > expectedLen)
                throw SyntaxError();
            else {
                if ((mlen = recv(c_sockfd, m_buffer, BUFFSIZE, 0)) == -1)
                {
                    perror("read error");
                    throw ComunicationException();
                }
                tmpContainer.append(m_buffer, mlen);
            }
        }

    return (int)m_commands.front().length();
}

int main()
{

    CServer server;

    return 0;
}
