#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_


//#include <stdio.h>
//#include <sys/socket.h>
//#include <sys/types.h>
//#include <resolv.h>
#include <netdb.h> // sockaddr_in
//#include <stdlib.h>
//#include <string.h>
#include <string>

using std::string;

struct tcpserv{
    string IP;
    string port;
};

enum cliState{
    CREATED = 0,
    //CONNECTED,
    ONLINE,
    AFK,
    CLOSED,
    ERROR
};

class tcpClient
{
public:
    tcpClient(const string &server_IP, const string &server_port);
    ~tcpClient();

    int run();
    int Send(const string &msg);
    int Recv(string &msg);

private:
    struct hostent* host;
    struct sockaddr_in addr;
    int sd, port;

    tcpserv server;
    enum cliState status;

    /* Thread functions */
    static void *recv_fcn(void *arg);
    static void *send_fcn(void *arg);
    static void *user_input_fcn(void *arg);
    string userMsg;
    //pthread_mutex_t cliVectMutex;
    pthread_mutex_t msgMutex;
    pthread_mutex_t sdMutex;
    pthread_cond_t msgRx_cond;
    //pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;
};

#endif // _TCP_CLIENT_H__H
