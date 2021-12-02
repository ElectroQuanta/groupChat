#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <netdb.h> // sockaddr_in
#include <vector>
#include <string>

using std::string;
using std::vector;

enum servState{
    CREATED = 0,
    //CONNECTED,
    ONLINE,
    AFK,
    CLOSED,
    ERROR
};


class tcpServer{
public:
    tcpServer(const string &port);
    ~tcpServer();
    int run();
private:
    enum servState status;
    vector<int> clients;
    int port;
    int sd;
    struct sockaddr_in addr;
    /* Thread functions */
    static void *user_input_fcn(void *arg);
    static void *recv_fcn(void *arg);
    static void *send_fcn(void *arg);
    pthread_mutex_t cliVectMutex;
    pthread_mutex_t msgMutex;
    string msgRx;
    int cli_sd;
};


#endif // _TCPSERVER_H__H
