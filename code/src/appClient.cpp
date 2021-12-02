#include "../inc/tcpClient.h"
#include <iostream>
#include <stdexcept>

using std::cout;
using std::endl;
using std::string;
using std::runtime_error;


int main( int argc, char *argv[] ){
    if(argc != 4){
        cout << "./" << string(argv[0])
             << "serv_addr serv_port welcome_msg" << endl;
    }
    try{
        /* Create client and send welcome message */
        tcpClient tcpCli(string(argv[1]), string(argv[2]));
        tcpCli.Send(string(argv[3]));
        tcpCli.run();
    }catch(runtime_error &e)
    {
        cout << e.what() << endl;
        return 1;
    }
}
