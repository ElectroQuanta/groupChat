#include "../inc/tcpServer.h"
#include <netdb.h>
#include <cstring> //memset
#include <stdexcept>
#include <pthread.h>
#include <iostream>

using std::runtime_error;
using std::cout;
using std::endl;

tcpServer::tcpServer(const string &port){
    // get server's IP
    if ( !isdigit(port.at(0)) )
    {
        struct servent *srv = getservbyname(port.c_str(), "tcp");
        if ( srv == NULL )
        {
            this->port = -1; // Signal error
            this->status = servState::ERROR;
            string s = "[Server]: Invalid port ";
            s += port;
            s += " - ";
            s += string( strerror(errno) );
            throw runtime_error(s);
            return;
        }

        //printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
        this->port = srv->s_port;
    }
    else
        this->port = htons(atoi(port.c_str()));

    /*---Create socket and connect to server---*/
    this->sd = socket(PF_INET, SOCK_STREAM, 0);        /* create socket */
    if ( this->sd < 0 )
    {
        this->status = servState::ERROR;
        string s = "[Server]: Failed to create socket - ";
        s += string( strerror(errno) );
        throw runtime_error(s);
        return;
    }
    this->status = servState::CREATED;

    /**< Bind port/addr to socket */
    memset(&this->addr, 0, sizeof(this->addr));       /* create & zero struct */
    this->addr.sin_family = AF_INET;        /* select internet protocol */
    this->addr.sin_port = this->port;                       /* set the port # */
    this->addr.sin_addr.s_addr = INADDR_ANY;  /* any interface */
    
    if ( bind(this->sd, (struct sockaddr*)&this->addr, sizeof(this->addr)) != 0)
    {
        this->status = servState::ERROR;
        string s = "[Server]: Failed to bind - ";
        s += string( strerror(errno) );
        throw runtime_error(s);
        return;
    }

    /*--- make into listener with 10 slots ---*/
    #define MAX_CLIENTS 32
    if ( listen(this->sd, MAX_CLIENTS) != 0 )
    {
        this->status = servState::ERROR;
        string s = "[Server]: Failed to listen - ";
        s += string( strerror(errno) );
        throw runtime_error(s);
        return;
    }
    this->status = servState::ONLINE;


  /*dynamic initialization of mutex before trying to use it*/
  pthread_mutex_init(&cliVectMutex,NULL);
  pthread_mutex_init(&msgMutex,NULL);
}

/* Event loop */
int tcpServer::run(){
    pthread_t accept_th;

    pthread_create(&accept_th, 0, &this->accept_fcn, this);       /* start thread */

    /* Wait for threads to finish */
    //pthread_join();
    pthread_join(accept_th, NULL);

    return 0; // success
}

void *tcpServer::accept_fcn(void *arg)
{
    tcpServer *obj = (tcpServer *)&arg;
    while (1)                         /* process all incoming clients */
    {
        unsigned int n = sizeof(obj->addr);
        int sd = accept(obj->sd, (struct sockaddr*)&obj->addr, &n);     /* accept connection */
        if( sd != -1)
        {
            /* Add client to vector */
            pthread_mutex_lock (&obj->cliVectMutex);
            obj->clients.push_back(sd);
            pthread_mutex_unlock (&obj->cliVectMutex);
            
            pthread_t rx, tx;
            cout << "New connection accepted from " << std::to_string(sd)
                 << endl;
            pthread_create(&rx, 0, recv_fcn, &obj);       /* start thread */
            pthread_create(&tx, 0,send_fcn, &obj);       /* start thread */
            pthread_detach(rx);                      /* don't track it */
            pthread_detach(tx);                      /* don't track it */
        }
    }
    
    return 0;
}

/**
 * @brief Receives string from client
 * @param arg: contains the serverData; last sd is the client connected
 * @return ret
 *
 * detailed
 */
void *tcpServer::recv_fcn(void *arg){
    tcpServer *obj = (tcpServer *)&arg;
    pthread_mutex_lock (&obj->cliVectMutex);
    int sd = obj->clients.back(); // get client descriptor
    pthread_mutex_unlock (&obj->cliVectMutex);
    char buffer[256];

    /* Receive and print */
    recv(sd,buffer,sizeof(buffer),0);
    pthread_mutex_lock (&obj->msgMutex);
    obj->msgRx = string(buffer);
    pthread_mutex_unlock (&obj->msgMutex);
    
    cout << "Client " << std::to_string(sd) <<
        ">: " << obj->msgRx << endl;

    //pthread_mutex_lock (&obj->cliVectMutex);
    //for (vector<int>::const_iterator iter = obj->clients.begin();
    //     iter != obj->clients.end(); ++iter)
    //{
    //    pthread_t broadcast;
    //    obj->cli_sd = *iter;

    //    pthread_create(&broadcast, 0,send_fcn, &obj);       /* start thread */
    //    pthread_detach(broadcast);                      /* don't track it */
    //}
    //pthread_mutex_unlock (&obj->cliVectMutex);

    for (vector<int>::const_iterator iter = obj->clients.begin();
         iter != obj->clients.end(); ++iter)
    {
        pthread_t broadcast;
        pthread_mutex_lock (&obj->cliVectMutex);
        obj->cli_sd = *iter;
        pthread_mutex_unlock (&obj->cliVectMutex);

        pthread_create(&broadcast, 0,send_fcn, &obj);       /* start thread */
        pthread_detach(broadcast);                      /* don't track it */
    }


    return 0;
}

/**
 * @brief Broadcasts recv string from all clients
 * @param arg: contains the serverData; last sd is the client connected
 * @return ret
 *
 * detailed
 */
void *tcpServer::send_fcn(void *arg){
    tcpServer *obj = (tcpServer *)&arg;
    pthread_mutex_lock (&obj->cliVectMutex);
    int sd = obj->cli_sd; // get client descriptor
    string msg = obj->msgRx;
    pthread_mutex_unlock (&obj->cliVectMutex);

    /* Send_msg */
    send(sd,msg.c_str(),msg.size(),0);
    return 0;
}
