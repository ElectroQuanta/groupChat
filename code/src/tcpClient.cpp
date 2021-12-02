#include "tcpClient.h"
#include <cstring> //memset
#include <unistd.h> // getpid
//#include <sys/types.h>         /* Socket calls compatibility */ 
//#include <sys/socket.h> /* socket calls */
//#include <fcntl.h>
#include <stdexcept>
#include <iostream>

using std::cin;
using std::cout;
using std::endl;
using std::runtime_error;

tcpClient::tcpClient(const string &server_IP, const string &server_port){
    // get server's IP
    this->host = gethostbyname(server_IP.c_str());

//    const char *serv_port = server_port.c_str();
    
    if ( !isdigit(server_port.at(0)) )
        //   if ( !isdigit(serv_port[0]) )
    {
        struct servent *srv = getservbyname(server_port.c_str(), "tcp");
        if ( srv == NULL )
        {
            string err = "[Client]: Invalid port ";
            err += server_port;
            this->port = -1; // Signal error
            throw runtime_error(err);
            this->status = cliState::ERROR;
            return;
        }

        //printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
        this->port = srv->s_port;
    }
    else
        this->port = htons(atoi(server_port.c_str()));

    server.port = this->port;
    server.IP = server_IP;


    /*---Create socket and connect to server---*/
    this->sd = socket(PF_INET, SOCK_STREAM, 0);        /* create socket */
    if ( this->sd < 0 )
    {
        throw runtime_error("[Client]: could not create socket!");
        this->status = cliState::ERROR;
        return;
    }
    this->status = cliState::CREATED;
    memset(&this->addr, 0, sizeof(this->addr));       /* create & zero struct */
    this->addr.sin_family = AF_INET;        /* select internet protocol */
    this->addr.sin_port = port;                       /* set the port # */
    this->addr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);  /* set the addr */

    /*---If connection successful, send the message and read results---*/
    if ( connect(this->sd, (struct sockaddr*)&this->addr, sizeof(this->addr)) == 0)
    {
        this->status = cliState::ONLINE;
        /** send welcome message to server */
        pid_t cliPID = getpid();
        string welcomeMsg = "[Client ";
        welcomeMsg += std::to_string(cliPID);
        
        this->Send(welcomeMsg);
    }
    else
    {
        throw runtime_error("[Client]: connection failed!");
        this->status = cliState::ERROR;
    }
    /*dynamic initialization of mutex before trying to use it*/
    pthread_mutex_init(&msgMutex,NULL);
    pthread_mutex_init(&sdMutex,NULL);
    pthread_cond_init(&msgRx_cond,NULL);
}

int tcpClient::run(){
    pthread_t user_input_th, send_th, recv_th;

    pthread_create(&user_input_th, 0, &this->user_input_fcn, this); /* user input thread */
    pthread_create(&send_th, 0, &this->send_fcn, this); /* send thread */
    pthread_create(&recv_th, 0, &this->recv_fcn, this); /* recv thread */

    /* Wait for threads to finish */
    pthread_join(user_input_th, NULL);
    pthread_join(send_th, NULL);
    pthread_join(recv_th, NULL);

    return 0; // success
}

int tcpClient::Send(const string &msg){

    if( this->status != cliState::ONLINE )
        throw runtime_error("[Client]: Socket must be online to send");

    int status = send(this->sd, msg.c_str(), msg.length(), 0);
    if(status < 0 && ( (errno != EAGAIN) || (errno != EWOULDBLOCK) ) ) 
    {
        string s = "[Socket]: Failed to send - ";
        s += string( strerror(errno) );
        throw runtime_error(s);
    }
    return status; // return nr. of bytes send or -1 (errno should be EAGAIN)

/* 
DESCRIPTION         top

       The system calls send(), sendto(), and sendmsg() are used to transmit
       a message to another socket.

       The send() call may be used only when the socket is in a connected
       state (so that the intended recipient is known).  The only difference
       between send() and write(2) is the presence of flags.  With a zero
       flags argument, send() is equivalent to write(2).  Also, the
       following call

           send(sockfd, buf, len, flags);

       is equivalent to

           sendto(sockfd, buf, len, flags, NULL, 0);

       The argument sockfd is the file descriptor of the sending socket.

       If sendto() is used on a connection-mode (SOCK_STREAM,
       SOCK_SEQPACKET) socket, the arguments dest_addr and addrlen are
       ignored (and the error EISCONN may be returned when they are not NULL
       and 0), and the error ENOTCONN is returned when the socket was not
       actually connected.  Otherwise, the address of the target is given by
       dest_addr with addrlen specifying its size.  For sendmsg(), the
       address of the target is given by msg.msg_name, with msg.msg_namelen
       specifying its size.

       For send() and sendto(), the message is found in buf and has length
       len.  For sendmsg(), the message is pointed to by the elements of the
       array msg.msg_iov.  The sendmsg() call also allows sending ancillary
       data (also known as control information).

       If the message is too long to pass atomically through the underlying
       protocol, the error EMSGSIZE is returned, and the message is not
       transmitted.

       No indication of failure to deliver is implicit in a send().  Locally
       detected errors are indicated by a return value of -1.

       When the message does not fit into the send buffer of the socket,
       send() normally blocks, unless the socket has been placed in
       nonblocking I/O mode.  In nonblocking mode it would fail with the
       error EAGAIN or EWOULDBLOCK in this case.  The select(2) call may be
       used to determine when it is possible to send more data.

   The flags argument
       The flags argument is the bitwise OR of zero or more of the following
       flags.

       MSG_CONFIRM (since Linux 2.3.15)
              Tell the link layer that forward progress happened: you got a
              successful reply from the other side.  If the link layer
              doesn't get this it will regularly reprobe the neighbor (e.g.,
              via a unicast ARP).  Valid only on SOCK_DGRAM and SOCK_RAW
              sockets and currently implemented only for IPv4 and IPv6.  See
              arp(7) for details.

       MSG_DONTROUTE
              Don't use a gateway to send out the packet, send to hosts only
              on directly connected networks.  This is usually used only by
              diagnostic or routing programs.  This is defined only for
              protocol families that route; packet sockets don't.

       MSG_DONTWAIT (since Linux 2.2)
              Enables nonblocking operation; if the operation would block,
              EAGAIN or EWOULDBLOCK is returned.  This provides similar
              behavior to setting the O_NONBLOCK flag (via the fcntl(2)
              F_SETFL operation), but differs in that MSG_DONTWAIT is a per-
              call option, whereas O_NONBLOCK is a setting on the open file
              description (see open(2)), which will affect all threads in
              the calling process and as well as other processes that hold
              file descriptors referring to the same open file description.

       MSG_EOR (since Linux 2.2)
              Terminates a record (when this notion is supported, as for
              sockets of type SOCK_SEQPACKET).

       MSG_MORE (since Linux 2.4.4)
              The caller has more data to send.  This flag is used with TCP
              sockets to obtain the same effect as the TCP_CORK socket
              option (see tcp(7)), with the difference that this flag can be
              set on a per-call basis.

              Since Linux 2.6, this flag is also supported for UDP sockets,
              and informs the kernel to package all of the data sent in
              calls with this flag set into a single datagram which is
              transmitted only when a call is performed that does not
              specify this flag.  (See also the UDP_CORK socket option
              described in udp(7).)

       MSG_NOSIGNAL (since Linux 2.2)
              Don't generate a SIGPIPE signal if the peer on a stream-
              oriented socket has closed the connection.  The EPIPE error is
              still returned.  This provides similar behavior to using
              sigaction(2) to ignore SIGPIPE, but, whereas MSG_NOSIGNAL is a
              per-call feature, ignoring SIGPIPE sets a process attribute
              that affects all threads in the process.

       MSG_OOB
              Sends out-of-band data on sockets that support this notion
              (e.g., of type SOCK_STREAM); the underlying protocol must also
              support out-of-band data.

   sendmsg()
       The definition of the msghdr structure employed by sendmsg() is as
       follows:

           struct msghdr {
               void         *msg_name;       // Optional address 
               socklen_t     msg_namelen;    // Size of address 
               struct iovec *msg_iov;        // Scatter/gather array 
               size_t        msg_iovlen;     // # elements in msg_iov 
               void         *msg_control;    // Ancillary data, see below 
               size_t        msg_controllen; // Ancillary data buffer len 
               int           msg_flags;      // Flags (unused) 
           };

       The msg_name field is used on an unconnected socket to specify the
       target address for a datagram.  It points to a buffer containing the
       address; the msg_namelen field should be set to the size of the
       address.  For a connected socket, these fields should be specified as
       NULL and 0, respectively.

       The msg_iov and msg_iovlen fields specify scatter-gather locations,
       as for writev(2).

       You may send control information using the msg_control and msg_con‐
       trollen members.  The maximum control buffer length the kernel can
       process is limited per socket by the value in /proc/sys/net/core/opt‐
       mem_max; see socket(7).

       The msg_flags field is ignored.

RETURN VALUE         top

       On success, these calls return the number of bytes sent.  On error,
       -1 is returned, and errno is set appropriately.
 */
}

/**
 * @brief Reads string from buffer
 * @param param1
 * @return ret
 *
 * detailed
 */
int tcpClient::Recv(string &s )
{
    if( this->status != cliState::ONLINE )
        throw runtime_error("[Client]: Socket must be online to receive");
    /* TODO: get the appropriate flag */
    int flags = 0; 
    char buf[1024] = {0};
    int status = recv(this->sd, (void *)buf, sizeof(buf), flags);
    if(status < 0 && ( (errno != EAGAIN) || (errno != EWOULDBLOCK) ) ) 
    {
        string s = "[Socket]: Failed to receive - ";
        s += string( strerror(errno) );
        throw runtime_error(s);
    }
    s = string(buf);
    return status; // return nr. of bytes read or -1 (errno should be EAGAIN)

/* 
DESCRIPTION         top

       The recv(), recvfrom(), and recvmsg() calls are used to receive
       messages from a socket.  They may be used to receive data on both
       connectionless and connection-oriented sockets.  This page first
       describes common features of all three system calls, and then
       describes the differences between the calls.

       The only difference between recv() and read(2) is the presence of
       flags.  With a zero flags argument, recv() is generally equivalent to
       read(2) (but see NOTES).  Also, the following call

           recv(sockfd, buf, len, flags);

       is equivalent to

           recvfrom(sockfd, buf, len, flags, NULL, NULL);

       All three calls return the length of the message on successful
       completion.  If a message is too long to fit in the supplied buffer,
       excess bytes may be discarded depending on the type of socket the
       message is received from.

       If no messages are available at the socket, the receive calls wait
       for a message to arrive, unless the socket is nonblocking (see
       fcntl(2)), in which case the value -1 is returned and the external
       variable errno is set to EAGAIN or EWOULDBLOCK.  The receive calls
       normally return any data available, up to the requested amount,
       rather than waiting for receipt of the full amount requested.

       An application can use select(2), poll(2), or epoll(7) to determine
       when more data arrives on a socket.

   The flags argument
       The flags argument is formed by ORing one or more of the following
       values:

       MSG_CMSG_CLOEXEC (recvmsg() only; since Linux 2.6.23)
              Set the close-on-exec flag for the file descriptor received
              via a UNIX domain file descriptor using the SCM_RIGHTS
              operation (described in unix(7)).  This flag is useful for the
              same reasons as the O_CLOEXEC flag of open(2).

       MSG_DONTWAIT (since Linux 2.2)
              Enables nonblocking operation; if the operation would block,
              the call fails with the error EAGAIN or EWOULDBLOCK.  This
              provides similar behavior to setting the O_NONBLOCK flag (via
              the fcntl(2) F_SETFL operation), but differs in that
              MSG_DONTWAIT is a per-call option, whereas O_NONBLOCK is a
              setting on the open file description (see open(2)), which will
              affect all threads in the calling process and as well as other
              processes that hold file descriptors referring to the same
              open file description.

       MSG_ERRQUEUE (since Linux 2.2)
              This flag specifies that queued errors should be received from
              the socket error queue.  The error is passed in an ancillary
              message with a type dependent on the protocol (for IPv4
              IP_RECVERR).  The user should supply a buffer of sufficient
              size.  See cmsg(3) and ip(7) for more information.  The
              payload of the original packet that caused the error is passed
              as normal data via msg_iovec.  The original destination
              address of the datagram that caused the error is supplied via
              msg_name.

              The error is supplied in a sock_extended_err structure:

                  #define SO_EE_ORIGIN_NONE    0
                  #define SO_EE_ORIGIN_LOCAL   1
                  #define SO_EE_ORIGIN_ICMP    2
                  #define SO_EE_ORIGIN_ICMP6   3

                  struct sock_extended_err
                  {
                      uint32_t ee_errno;   // Error number 
                      uint8_t  ee_origin;  // Where the error originated 
                      uint8_t  ee_type;    // Type 
                      uint8_t  ee_code;    // Code 
                      uint8_t  ee_pad;     // Padding 
                      uint32_t ee_info;    // Additional information 
                      uint32_t ee_data;    // Other data 
                      // More data may follow 
                  };

                  struct sockaddr *SO_EE_OFFENDER(struct sock_extended_err *);

              ee_errno contains the errno number of the queued error.
              ee_origin is the origin code of where the error originated.
              The other fields are protocol-specific.  The macro
              SOCK_EE_OFFENDER returns a pointer to the address of the net‐
              work object where the error originated from given a pointer to
              the ancillary message.  If this address is not known, the
              sa_family member of the sockaddr contains AF_UNSPEC and the
              other fields of the sockaddr are undefined.  The payload of
              the packet that caused the error is passed as normal data.

              For local errors, no address is passed (this can be checked
              with the cmsg_len member of the cmsghdr).  For error receives,
              the MSG_ERRQUEUE flag is set in the msghdr.  After an error
              has been passed, the pending socket error is regenerated based
              on the next queued error and will be passed on the next socket
              operation.

       MSG_OOB
              This flag requests receipt of out-of-band data that would not
              be received in the normal data stream.  Some protocols place
              expedited data at the head of the normal data queue, and thus
              this flag cannot be used with such protocols.

       MSG_PEEK
              This flag causes the receive operation to return data from the
              beginning of the receive queue without removing that data from
              the queue.  Thus, a subsequent receive call will return the
              same data.

       MSG_TRUNC (since Linux 2.2)
              For raw (AF_PACKET), Internet datagram (since Linux
              2.4.27/2.6.8), netlink (since Linux 2.6.22), and UNIX datagram
              (since Linux 3.4) sockets: return the real length of the
              packet or datagram, even when it was longer than the passed
              buffer.

              For use with Internet stream sockets, see tcp(7).

       MSG_WAITALL (since Linux 2.2)
              This flag requests that the operation block until the full
              request is satisfied.  However, the call may still return less
              data than requested if a signal is caught, an error or discon‐
              nect occurs, or the next data to be received is of a different
              type than that returned.  This flag has no effect for datagram
              sockets.

   recvfrom()
       recvfrom() places the received message into the buffer buf.  The
       caller must specify the size of the buffer in len.

       If src_addr is not NULL, and the underlying protocol provides the
       source address of the message, that source address is placed in the
       buffer pointed to by src_addr.  In this case, addrlen is a value-
       result argument.  Before the call, it should be initialized to the
       size of the buffer associated with src_addr.  Upon return, addrlen is
       updated to contain the actual size of the source address.  The
       returned address is truncated if the buffer provided is too small; in
       this case, addrlen will return a value greater than was supplied to
       the call.

       If the caller is not interested in the source address, src_addr and
       addrlen should be specified as NULL.

   recv()
       The recv() call is normally used only on a connected socket (see
       connect(2)).  It is equivalent to the call:

           recvfrom(fd, buf, len, flags, NULL, 0);

   recvmsg()
       The recvmsg() call uses a msghdr structure to minimize the number of
       directly supplied arguments.  This structure is defined as follows in
       <sys/socket.h>:

           struct iovec {                    // Scatter/gather array items
               void  *iov_base;              // Starting address 
               size_t iov_len;               // Number of bytes to transfer 
           };                                 
                                              
           struct msghdr {                    
               void         *msg_name;       // Optional address 
               socklen_t     msg_namelen;    // Size of address 
               struct iovec *msg_iov;        // Scatter/gather array 
               size_t        msg_iovlen;     // # elements in msg_iov 
               void         *msg_control;    // Ancillary data, see below 
               size_t        msg_controllen; // Ancillary data buffer len 
               int           msg_flags;      // Flags on received message 
           };

       The msg_name field points to a caller-allocated buffer that is used
       to return the source address if the socket is unconnected.  The call‐
       er should set msg_namelen to the size of this buffer before this
       call; upon return from a successful call, msg_namelen will contain
       the length of the returned address.  If the application does not need
       to know the source address, msg_name can be specified as NULL.

       The fields msg_iov and msg_iovlen describe scatter-gather locations,
       as discussed in readv(2).

       The field msg_control, which has length msg_controllen, points to a
       buffer for other protocol control-related messages or miscellaneous
       ancillary data.  When recvmsg() is called, msg_controllen should con‐
       tain the length of the available buffer in msg_control; upon return
       from a successful call it will contain the length of the control mes‐
       sage sequence.

       The messages are of the form:

           struct cmsghdr {
               size_t cmsg_len;    // Data byte count, including header
                                     (type is socklen_t in POSIX) 
               int    cmsg_level;  // Originating protocol 
               int    cmsg_type;   // Protocol-specific type 
           // followed by
               unsigned char cmsg_data[]; 
           };

       Ancillary data should be accessed only by the macros defined in
       cmsg(3).

       As an example, Linux uses this ancillary data mechanism to pass
       extended errors, IP options, or file descriptors over UNIX domain
       sockets.

       The msg_flags field in the msghdr is set on return of recvmsg().  It
       can contain several flags:

       MSG_EOR
              indicates end-of-record; the data returned completed a record
              (generally used with sockets of type SOCK_SEQPACKET).

       MSG_TRUNC
              indicates that the trailing portion of a datagram was dis‐
              carded because the datagram was larger than the buffer sup‐
              plied.

       MSG_CTRUNC
              indicates that some control data was discarded due to lack of
              space in the buffer for ancillary data.

       MSG_OOB
              is returned to indicate that expedited or out-of-band data was
              received.

       MSG_ERRQUEUE
              indicates that no data was received but an extended error from
              the socket error queue.

RETURN VALUE         top

       These calls return the number of bytes received, or -1 if an error
       occurred.  In the event of an error, errno is set to indicate the
       error.

       When a stream socket peer has performed an orderly shutdown, the
       return value will be 0 (the traditional "end-of-file" return).

       Datagram sockets in various domains (e.g., the UNIX and Internet
       domains) permit zero-length datagrams.  When such a datagram is
       received, the return value is 0.

       The value 0 may also be returned if the requested number of bytes to
       receive from a stream socket was 0.

 */
}


/**
 * @brief Thread worker to receive messages from server
 * @param arg: contains the tcpClient obj
 * @return 0
 *
 * detailed
 */
void *tcpClient::recv_fcn(void *arg){
    tcpClient *obj = (tcpClient *)&arg;
    char buffer[256];

    while(1){
        /**
         * Receive data and print
         *  - protect the socket descriptor (it is used to read and write)
         */
        pthread_mutex_lock (&obj->sdMutex);
        recv(obj->sd,buffer,sizeof(buffer),0);
        pthread_mutex_unlock (&obj->sdMutex);
    
        cout << string(buffer) << endl;
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
void *tcpClient::send_fcn(void *arg){
    tcpClient *obj = (tcpClient *)&arg;
    string msg;
    while(1){
        // get msg to send
        pthread_mutex_lock (&obj->msgMutex);
        pthread_cond_wait( &obj->msgRx_cond, &obj->msgMutex );
        msg = obj->userMsg;
        pthread_mutex_unlock (&obj->msgMutex);

        /**
         * Send data
         *  - protect the socket descriptor (it is used to read and write)
         */
        pthread_mutex_lock (&obj->sdMutex);
        send(obj->sd, msg.c_str(), msg.size(), 0);
        pthread_mutex_unlock (&obj->sdMutex);
        
    }
    return 0;
}


void *tcpClient::user_input_fcn(void *arg){
    tcpClient *obj = (tcpClient *)&arg;
    string line;
    while(1){
        // get user input (it blocks here)
        std::getline(cin, line);
        pthread_mutex_lock (&obj->msgMutex);
        obj->userMsg = line;
        pthread_cond_signal( &obj->msgRx_cond );
        pthread_mutex_unlock (&obj->msgMutex);
    }
    
}
