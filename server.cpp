#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <queue>
#include <sstream>

using namespace std;


#define SERVER_MOVE "102 MOVE\a\b"
#define SERVER_TURN_LEFT "103 TURN LEFT\a\b"
#define SERVER_TURN_RIGHT "104 TURN RIGHT\a\b"
#define SERVER_PICK_UP "105 GET MESSAGE\a\b"
#define SERVER_LOGOUT "106 LOGOUT\a\b"
#define SERVER_OK "200 OK\a\b"
#define SERVER_LOGIN_FAILED "300 LOGIN FAILED\a\b"
#define SERVER_SYNTAX_ERROR "301 SYNTAX ERROR\a\b"
#define SERVER_LOGIC_ERROR "302 LOGIC ERROR\a\b"

//#define SERVER_MOVE_LEN 10
//#define SERVER_TURN_LEFT_LEN 17
//#define SERVER_TURN_RIGHT_LEN 18
//#define SERVER_PICK_UP_LEN 19
//#define SERVER_LOGOUT_LEN 15
//#define SERVER_OK_LEN 8
//#define SERVER_LOGIN_FAILED_LEN 20
//#define SERVER_SYNTAX_ERROR_LEN 20
//#define SERVER_LOGIC_ERROR_LEN 19

#define CLIENT_USERNAME_LEN 12
#define CLIENT_CONFIRMATION_LEN 7
#define CLIENT_OK_LEN 999
#define CLIENT_RECHARGING_LEN 12
#define CLIENT_FULL_POWER_LEN 12
#define CLIENT_MESSAGE_LEN 100

#define POSTFIX "\a\b"


class CommunicationError{};
class SyntaxError{};
class LoginError{};

enum EDirection {Default, Up, Right, Down, Left};

class CMessenger
{
protected:
    char m_buffer[1000];
    const int BUFFSIZE = 1000;
    static queue<string> m_commands;

    void sendMessage(int c_sockfd, const char *message, int mlen) const;
    int receiveMessage(int c_sockfd, size_t expectedLen);
    void validateMessage(const string & message, const size_t expLen) const;
};

queue<string> CMessenger::m_commands;

void CMessenger::sendMessage(int c_sockfd, const char *message, int mlen = 0) const
{
    if(mlen == 0)
        mlen = strlen(message);
    if (send(c_sockfd, message, mlen, 0) == -1) {
        perror("write error");
        throw CommunicationError();
    }
}

int CMessenger::receiveMessage(int c_sockfd, size_t expectedLen)
{
    if(!m_commands.empty())
        return (int)m_commands.front().length();

    int mlen = 0;
    string tmpContainer;
    size_t foundPos;
    bool foundBool = false;

    if ((mlen = recv(c_sockfd, m_buffer, BUFFSIZE, 0)) == -1)
    {
        perror("read error");
        throw CommunicationError();
    }

    tmpContainer.append(m_buffer, mlen);

    while(true) {
        cout << "buffer: "<< m_buffer << endl;
//        printf("buffer: %s\n", m_buffer);

//        validateMessage(tmpContainer, expectedLen);
        foundPos = tmpContainer.find(POSTFIX);

        if (foundPos != string::npos) {
            foundBool = true;
            string tmpCommand(tmpContainer, 0, foundPos);
//            if(tmpCommand.length() > expectedLen)
//                throw
            m_commands.push(tmpCommand);
            tmpContainer.erase(0, foundPos + 2);
            if (!tmpContainer.empty())
                continue;
            break;
        } else {

            cout << "not found" << endl;
            if (!foundBool && tmpContainer.size() > expectedLen){
                throw SyntaxError();
            }

            if (send(c_sockfd, m_buffer, mlen, 0) == -1) {
                perror("write error");
                throw CommunicationError();
            }

            if ((mlen = recv(c_sockfd, m_buffer, BUFFSIZE, 0)) == -1) {
                perror("read error");
                throw CommunicationError();
            }

            if (mlen == 0)
                throw CommunicationError();

            tmpContainer.append(m_buffer, mlen);
        }
    }
    cout << "successfully" << endl;
    return (int)m_commands.front().length();
}

void CMessenger::validateMessage(const string & message, const size_t expLen) const
{
    switch(expLen)
    {
        case CLIENT_USERNAME_LEN: {
//            if()
        }
    }
}

class CRobot : public CMessenger {
private:
    int m_sockfd;
    bool m_reached;
    pair<int, int> m_prevPosition;
    pair<int, int> m_position;
    EDirection m_requiredDirHor;
    EDirection m_requiredDirVert;
    EDirection m_direction;
    EDirection m_tmpDir;
public:
    CRobot(int c_sockfd);
    void move();
    void extractCoords(const string & str);
    void determineMyDirection();
    void determineRequiredDirection();
    void checkIfReached();
    void turnLeft();
    void turnRight();
    void turnToProperDirection();
    void print() { printf("curX: %d\ncurY: %d\nprevX: %d\nprevY: %d\ndir: %d\nreached: %d\n",
                          m_position.first, m_position.second,
                          m_prevPosition.first, m_prevPosition.second,
                          m_direction, m_reached); }
};

void CRobot::move()
{
    cout << "Robot moves\n";
    do {
        sendMessage(m_sockfd, SERVER_MOVE);
        receiveMessage(m_sockfd, CLIENT_OK_LEN);
        extractCoords(m_commands.front());
        m_commands.pop();
    } while(m_position == m_prevPosition);
    checkIfReached();
    print();
}

CRobot::CRobot(int c_sockfd)
        :m_sockfd(c_sockfd),
         m_position(make_pair(INT32_MAX, INT32_MAX)),
         m_prevPosition(make_pair(0,0)),
         m_reached(false),
         m_direction(Default),
         m_requiredDirHor(Default),
         m_requiredDirVert(Default),
         m_tmpDir(Default)
{
    move();
    if(m_reached)
        return;
    move();
    if(m_reached)
        return;
    determineMyDirection();
    determineRequiredDirection();
    turnToProperDirection();
    print();


}

void CRobot::extractCoords(const string & str)
{
    stringstream ss;
    vector<int> tmpCoords;

    /* Storing the whole string into string stream */
    ss << str;

    /* Running loop till the end of the stream */
    string temp;
    int found;
    while (!ss.eof()) {

        /* extracting word by word from stream */
        ss >> temp;

        /* Checking the given word is integer or not */
        if (stringstream(temp) >> found) {
            tmpCoords.push_back(found);
        }
    }
    m_prevPosition = m_position;
    m_position = make_pair(tmpCoords[0], tmpCoords[1]);
}

void CRobot::determineMyDirection()
{
    int x = m_position.first - m_prevPosition.first;
    int y = m_position.second - m_prevPosition.second;

    if(x != 0)
        if(x < 0) m_direction = Left;//left
        else m_direction = Right;//right
    else
        if(y < 0) m_direction = Down;//down
        else m_direction = Up;//up

    cout << "My direction is " << m_direction << endl;
}

void CRobot::checkIfReached()
{
    int x = m_position.first;
    int y = m_position.second;
    if(x <= 2 && x >= -2 && y <= 2 && y >= -2)
        m_reached = true;
}

void CRobot::determineRequiredDirection()
{
    int x = 0 - m_position.first;
    int y = 0 - m_position.second;

    if(x < 0 && x < -2)
        m_requiredDirHor = Left;

    if(x > 0 && x > 2)
        m_requiredDirHor = Right;

    if(y < 0 && y < -2)
        m_requiredDirVert = Down;

    if(y > 0 && y > 2)
        m_requiredDirVert = Up;
}

void CRobot::turnLeft()
{
    cout << "tmpDir before: " << m_tmpDir << endl;

//    m_tmpDir = m_direction;
    switch(m_tmpDir) {
        case Up: { m_tmpDir = Left; break; }
        case Down: { m_tmpDir = Right; break; }
        case Right: { m_tmpDir = Up; break; }
        case Left: { m_tmpDir = Down; break; }
    }

    cout << "tmpDir after: " << m_tmpDir << endl;
}

void CRobot::turnRight()
{
    cout << "tmpDir before: " << m_tmpDir << endl;

//    m_tmpDir = m_direction;
    switch(m_tmpDir) {
        case Up: { m_tmpDir = Right; break; }
        case Down: { m_tmpDir = Left; break; }
        case Right: { m_tmpDir = Down; break; }
        case Left: { m_tmpDir = Up; break; }
    }

    cout << "tmpDir after: " << m_tmpDir << endl;
}

void CRobot::turnToProperDirection()
{
    cout << "ReqDirV: " << m_requiredDirVert << "\nReqDirH: " << m_requiredDirHor << endl;

    m_tmpDir = m_direction;
    if(m_tmpDir == m_requiredDirVert || m_tmpDir == m_requiredDirHor)
        return;

    turnLeft();
    if(m_tmpDir == m_requiredDirHor || m_tmpDir == m_requiredDirVert)
    {
        m_direction = m_tmpDir;
        sendMessage(m_sockfd, SERVER_TURN_LEFT);
        receiveMessage(m_sockfd, CLIENT_OK_LEN);
    } else {
        m_tmpDir = m_direction;
        turnRight();
        if(m_tmpDir == m_requiredDirHor || m_tmpDir == m_requiredDirVert)
        {
            m_direction = m_tmpDir;
            sendMessage(m_sockfd, SERVER_TURN_RIGHT);
            receiveMessage(m_sockfd, CLIENT_OK_LEN);
        }else {
            m_tmpDir = m_direction;

            turnRight();
            sendMessage(m_sockfd, SERVER_TURN_RIGHT);
            receiveMessage(m_sockfd, CLIENT_OK_LEN);
            m_direction = m_tmpDir;

            turnRight();
            sendMessage(m_sockfd, SERVER_TURN_RIGHT);
            receiveMessage(m_sockfd, CLIENT_OK_LEN);

            m_direction = m_tmpDir;
        }
    }

}


class CServer : public CMessenger
{
public:
    CServer(int port)
            : ECHOPORT(port)
    {
        setUpSocket();
        run();
    }

private:
    // const int ECHOPORT = 5599;
    int ECHOPORT;
    const unsigned short CLIENT_KEY = 45328;
    const unsigned short SERVER_KEY = 54621;
    int m_my_sockfd;
    sockaddr_in m_my_addr;
    sockaddr_in m_rem_addr;




    void setUpSocket();
    void run();
    void clientRoutine(int c_sockfd);
    void authenticate(int c_sockfd);
    unsigned short makeHash();
    int putCodeIntoBuffer(unsigned short code);
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

    try {
        authenticate(c_sockfd);
        sendMessage(c_sockfd, SERVER_OK);
    }
    catch (SyntaxError e) {
        cout << "Syntax error\n";
        sendMessage(c_sockfd, SERVER_SYNTAX_ERROR);
        close(c_sockfd);
        return;
    }
    catch(CommunicationError e) {
        cout << "Communication error\n";
        sendMessage(c_sockfd, SERVER_LOGIN_FAILED);
        close(c_sockfd);
        return;
    }
    catch(LoginError e) {
        cout << "Login error\n";
        sendMessage(c_sockfd, SERVER_LOGIN_FAILED);
        close(c_sockfd);
        return;
    }


    //robot creation here

    cout << "Authentication was successful\n";

//
    try {
        CRobot robot(c_sockfd);

//        if(robot.m_direction == )
    }
    catch(CommunicationError e) {
        cout << "Communication error\n";
        close(c_sockfd);
        return;
    }

    cout << "The End" << endl;

   /* while (1) {

        try {
            mlen = receiveMessage(c_sockfd, CLIENT_USERNAME_LEN);
        }
        catch (CommunicationError e) {
            break;
        }


        if(mlen == 0)
            break;

        printf("Client #%d: %s\n", c_sockfd, m_buffer);

        try {
            sendMessage(c_sockfd, m_buffer, mlen);
        }
        catch (CommunicationError e) {
            break;
        }

        printf("sending %i bytes back to the client\n", mlen);
    }*/
    close(c_sockfd);
}

void CServer::authenticate(int c_sockfd)
{
    int messLen;

    cout << "Waiting for username...\n";
    //getting username
    messLen = receiveMessage(c_sockfd, CLIENT_USERNAME_LEN);

    unsigned short hash = makeHash();
    cout << "hash: " << hash << endl;
    unsigned short confirmationCode = (hash + SERVER_KEY) % 65536;
    cout << "confirmation code: " << confirmationCode << endl;
    int codeLen = putCodeIntoBuffer(confirmationCode);

    //sending code to the client
    sendMessage(c_sockfd, m_buffer, codeLen);

    unsigned short expected = (hash + CLIENT_KEY) % 65536;
    cout << "expecting code: " << expected << endl;

    //getting client's code
    messLen = receiveMessage(c_sockfd, CLIENT_CONFIRMATION_LEN);

    string response(m_commands.front());
    m_commands.pop();

    unsigned short clientConfirmationCode = (unsigned short)stoi(response);
    cout << "client confirmation code: " << clientConfirmationCode << endl;

    if(expected != clientConfirmationCode) {
        throw LoginError();
    }
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

int CServer::putCodeIntoBuffer(unsigned short code)
{

    string tmpStr = to_string(code);
    tmpStr += POSTFIX;
    for (int i = 0; i < tmpStr.length(); ++i) {
        m_buffer[i] = tmpStr[i];
    }

    return (int)tmpStr.length();
}


int main(int argc, char const *argv[])
{
    if(argc != 2) {
        cout << "Usage: <port>\n";
        return 1;
    }
    int port = atoi(argv[1]);

    CServer server(port);

    return 0;
}
