#include "ClientSocket.h"
#include "SocketException.h"
#include <iostream>
#include <string>

int main ( int argc, int argv[] )
{
    try {
        ClientSocket client_socket ( "192.135.33.195", 30000 );
        std::string reply;

        int i;
        for (i = 0; i < 10; i++) {
            try {
                client_socket << "Closing communication";
                client_socket >> reply;
            }
            catch ( SocketException& ) {}

            std::cout << "Closing communication" << "\n";
        }
    }
    catch ( SocketException& e ) {
        std::cout << "Exception was caught:" << e.description() << "\n";
    }

    return 0;
}
