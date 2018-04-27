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
#include <regex>

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

#define CLIENT_USERNAME_LEN 12
#define CLIENT_CONFIRMATION_LEN 7
#define CLIENT_OK_LEN 12
#define CLIENT_RECHARGING_LEN 12
#define CLIENT_FULL_POWER_LEN 12
#define CLIENT_MESSAGE_LEN 100

#define POSTFIX "\a\b"

#define TIMEOUT 1
#define TIMEOUT_RECHARGING 5


class CommunicationError{};
class SyntaxError{};
class LoginError{};

enum EDirection { Default, Up, Right, Down, Left };
enum EMessageType
{
    Client_default,
    Client_username,
    Client_confirmation,
    Client_ok,
    Client_recharging,
    Client_full_power,
    Client_message
};

class CMessenger
{
public:
    CMessenger() = default;
    CMessenger(const queue<string> & src) : m_commands(src) {}
protected:
    const unsigned short CLIENT_KEY = 45328;
    const unsigned short SERVER_KEY = 54621;
    char m_buffer[1000];
    const size_t BUFFSIZE = 1000;
    queue<string> m_commands;

    void sendMessage(int c_sockfd, const char *message, int mlen) const;
    int receiveMessage(int c_sockfd, EMessageType type);
    bool isValid(const string &message, EMessageType type) const;
};


void CMessenger::sendMessage(int c_sockfd, const char *message, int mlen = 0) const
{
    cout << "Sending..." << endl;
    if(mlen == 0)
        mlen = strlen(message);
    if (send(c_sockfd, message, mlen, 0) == -1) {
        perror("write error");
        throw CommunicationError();
    }
}

int CMessenger::receiveMessage(int c_sockfd, EMessageType type)
{
    if(!m_commands.empty())
    {
        if(isValid(m_commands.front(), type))
            return (int)m_commands.front().length();
        else
            throw SyntaxError();
    }

    int mlen = 0;
    string tmpContainer;
    size_t foundPos;

    if ((mlen = (int)recv(c_sockfd, m_buffer, BUFFSIZE, 0)) == -1)
    {
        perror("read error");
        throw CommunicationError();
    }

    tmpContainer.append(m_buffer, mlen);

    while(true) {
        cout << "buffer: " << tmpContainer << endl;

        foundPos = tmpContainer.find(POSTFIX);

        if (foundPos != string::npos) {
            string tmpCommand(tmpContainer, 0, foundPos);

            if(!isValid(tmpCommand, type))
                throw SyntaxError();
            else
                type = Client_default;

            m_commands.push(tmpCommand);
            tmpContainer.erase(0, foundPos + 2);
            if (!tmpContainer.empty())
                continue;
            break;
        } else {

            cout << "not found" << endl;

            if(!isValid(tmpContainer, type))
                throw SyntaxError();

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

bool CMessenger::isValid(const string &message, EMessageType type) const
{
    switch(type)
    {
        case Client_username:
        {
//            if(message.length() <= 10)
//                cout << "Name len is less than or equal to 10" << endl;
//            else cout << "No" << endl;
//            cout << message.length() << endl;
            return message.size() <= CLIENT_USERNAME_LEN - 2;
        }
        case Client_confirmation:
        {
            cout << "Code validation" << endl;
            regex confirmation("^[0-9]{1,5}$");
            return regex_match(message, confirmation);
        }
        case Client_ok: { return true; }
        case Client_recharging:
        {
            if(message.length() < CLIENT_RECHARGING_LEN)
                return true;
            string recharging("RECHARGING");
            return message == recharging;
        }
        case Client_full_power:
        {
            string full_power("FULL POWER");
            return message == full_power;
        }
        case Client_message:
        {
            return message.length() <= CLIENT_MESSAGE_LEN - 2;
        }
        default: return true;
    }
}


class CRobot : public CMessenger
{
private:
    int m_sockfd;
    bool m_reached;
    pair<int, int> m_prevPosition;
    pair<int, int> m_position;
    EDirection m_requiredDirHor;
    EDirection m_requiredDirVert;
    EDirection m_direction;
    EDirection m_tmpDir;

    void move();
    void extractCoords(const string & str);
    void determineMyDirection();
    void determineRequiredDirection();
    void checkIfReached();
    void turnLeft();
    void turnRight();
    void turnToProperDirection();
    void changeDirection();
    void authenticate();
    unsigned short makeHash();
    int putCodeIntoBuffer(const unsigned short);
    void print() { printf("curX: %d\ncurY: %d\nprevX: %d\nprevY: %d\ndir: %d\nreached: %d\n",
                          m_position.first, m_position.second,
                          m_prevPosition.first, m_prevPosition.second,
                          m_direction, m_reached); }
public:
    CRobot(int c_sockfd);

};

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
    timeval timer{TIMEOUT, 0};
    setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(struct timeval));

    authenticate();
    sendMessage(c_sockfd, SERVER_OK);

    cout << "Authentication succeed" << endl;

    move();
    move();
    if(m_reached)
        return;
    determineMyDirection();
    determineRequiredDirection();
    turnToProperDirection();
    print();

//    switch(m_direction){
//        case Up: {
//            while(m_position.second < -2)
//                move();
//            break;
//        }
//        case Down: {
//            while(m_position.second > 2)
//                move();
//            break;
//        }
//        case Right: {
//            while(m_position.first < -2)
//                move();
//            break;
//        }
//        case Left: {
//            while(m_position.first > 2)
//                move();
//            break;
//        }
//    } cout << "End of switch" << endl;
//
//
//    if(!m_reached) {
//        changeDirection();
//
//        while(!m_reached)
//            move();
//    }
//
//    print();
//    cout << "Reached bro!" << endl;
}

void CRobot::move()
{
    cout << "Robot moves\n";
    do {
        sendMessage(m_sockfd, SERVER_MOVE);
        receiveMessage(m_sockfd, Client_ok);
        extractCoords(m_commands.front());
        m_commands.pop();
    } while(m_position == m_prevPosition);
    checkIfReached();
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
        receiveMessage(m_sockfd, Client_ok);
        m_commands.pop();
    } else {
        m_tmpDir = m_direction;
        turnRight();
        if(m_tmpDir == m_requiredDirHor || m_tmpDir == m_requiredDirVert)
        {
            m_direction = m_tmpDir;
            sendMessage(m_sockfd, SERVER_TURN_RIGHT);
            receiveMessage(m_sockfd, Client_ok);
            m_commands.pop();
        }else {
            m_tmpDir = m_direction;

            turnRight();
            sendMessage(m_sockfd, SERVER_TURN_RIGHT);
            receiveMessage(m_sockfd, Client_ok);
            m_commands.pop();

            turnRight();
            sendMessage(m_sockfd, SERVER_TURN_RIGHT);
            receiveMessage(m_sockfd, Client_ok);
            m_commands.pop();

            m_direction = m_tmpDir;
        }
    }

}

void CRobot::changeDirection()
{
    m_tmpDir = m_direction;
    turnLeft();
    if(m_tmpDir == m_requiredDirVert || m_tmpDir == m_requiredDirHor)
    {
        m_direction = m_tmpDir;
        sendMessage(m_sockfd, SERVER_TURN_LEFT);
        receiveMessage(m_sockfd, Client_ok);
        m_commands.pop();
    }else {
        m_tmpDir = m_direction;
        turnRight();
        m_direction = m_tmpDir;
        sendMessage(m_sockfd, SERVER_TURN_RIGHT);
        receiveMessage(m_sockfd, Client_ok);
        m_commands.pop();
    }
}

void CRobot::authenticate()
{
    int messLen;

    cout << "Waiting for username...\n";
    //getting username
    messLen = receiveMessage(m_sockfd, Client_username);

    unsigned short hash = makeHash();
    cout << "hash: " << hash << endl;
    unsigned short confirmationCode = (hash + SERVER_KEY) % 65536;
    cout << "confirmation code: " << confirmationCode << endl;
    int codeLen = putCodeIntoBuffer(confirmationCode);

    //sending code to the client
    sendMessage(m_sockfd, m_buffer, codeLen);

    unsigned short expected = (hash + CLIENT_KEY) % 65536;
    cout << "expecting code: " << expected << endl;

    //getting client's code
    messLen = receiveMessage(m_sockfd, Client_confirmation);

    string response(m_commands.front());
    m_commands.pop();

    unsigned short clientConfirmationCode = (unsigned short)stoi(response);
    cout << "client confirmation code: " << clientConfirmationCode << endl;

    if(expected != clientConfirmationCode) {
        throw LoginError();
    }
}

unsigned short CRobot::makeHash()
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

int CRobot::putCodeIntoBuffer(const unsigned short code)
{
    string tmpStr = to_string(code);
    tmpStr += POSTFIX;
    for (int i = 0; i < tmpStr.length(); ++i) {
        m_buffer[i] = tmpStr[i];
    }

    return (int)tmpStr.length();
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

    try {
        CRobot robot(c_sockfd);
    }
    catch (SyntaxError e) {
        cout << "Syntax error\n";
        sendMessage(c_sockfd, SERVER_SYNTAX_ERROR);
        close(c_sockfd);
        return;
    }
    catch(CommunicationError e) {
        cout << "Communication error\n";
        close(c_sockfd);
        return;
    }
    catch(LoginError e) {
        cout << "Login error\n";
        sendMessage(c_sockfd, SERVER_LOGIN_FAILED);
        close(c_sockfd);
        return;
    }


    cout << "Authentication was successful\n";



    cout << "The End" << endl;

    close(c_sockfd);
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
