#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <getopt.h>
#include <algorithm> 
#include <atomic>    
#include <poll.h>
#include <regex>
#include <csignal>
#include <cstdlib>
#include <functional>
#include "tcp.hpp"
#include "udp.hpp"

using namespace std;

// Global pointer to an instance of the UDP class
UDP* clientUDP = nullptr;
TCP* clientTCP = nullptr;

// Function to handle the SIGINT signal
void signalHandler(int signum) {
    cout << "Caught signal " << signum << endl;

    if (clientUDP != nullptr) {
        delete clientUDP;
    }
    if (clientTCP != nullptr){
        delete clientTCP;
    }

    exit(signum);
}

void cleanupAndExitUDP(int sock) {
    if (clientUDP != nullptr) {
        delete clientUDP;
    }
    //close(sock);
    exit(0);
}

void cleanupAndExitTCP(int sock) {
    if (clientTCP != nullptr) {
        delete clientTCP;
    }
    //close(sock);
    exit(0);
}

int main(int argc, char *argv[])
{
    int opt;
    string transportProtocol;
    string serverAddress;
    uint16_t port = 4567; // Default port

    // Parse command line arguments
    bool helpRequested = false;
    bool retTime = false;
    int d = 250; // milliseconds
    int r = 3;

    while ((opt = getopt(argc, argv, "t:s:d:r:p:h")) != -1) {
        int parsedPort; // Define variable here
        switch (opt) {
            case 't':
                transportProtocol = optarg;
                break;
            case 's':
                serverAddress = optarg;
                break;
            case 'p':
                for (size_t i = 0; optarg[i] != '\0'; ++i) {
                    if (!isdigit(optarg[i])) {
                        cerr << "Invalid parameter: " << optarg << ". Please provide a valid numerical value." << endl;
                        exit(-1);
                    }
                }
                parsedPort = atoi(optarg);
                if (parsedPort < 0 || parsedPort > UINT16_MAX) {
                    cerr << "Invalid port number. Please provide a valid uint16_t value." << endl;
                    exit(-1);
                }
                port = static_cast<uint16_t>(parsedPort);
                break;
            case 'd':
                for (size_t i = 0; optarg[i] != '\0'; ++i) {
                    if (!isdigit(optarg[i])) {
                        cerr << "Invalid parameter: " << optarg << ". Please provide a valid numerical value." << endl;
                        exit(-1);
                    }
                }
                parsedPort = atoi(optarg);
                if (parsedPort < 0 || parsedPort > UINT16_MAX) {
                    cerr << "Invalid time number. Please provide a valid uint16_t value." << endl;
                    exit(-1);
                }
                d = static_cast<uint16_t>(parsedPort);
                retTime = true;
                break;
            case 'r':
                for (size_t i = 0; optarg[i] != '\0'; ++i) {
                    if (!isdigit(optarg[i])) {
                        cerr << "Invalid parameter: " << optarg << ". Please provide a valid numerical value." << endl;
                        exit(-1);
                    }
                }
                parsedPort = atoi(optarg);
                if (parsedPort < 0 || parsedPort > UINT8_MAX) {
                    cerr << "Invalid retries number. Please provide a valid uint8_t value." << endl;
                    exit(-1);
                }
                r = static_cast<uint8_t>(parsedPort);
                retTime = true;
                break;
            case 'h':
                helpRequested = true;
                break;
            default:
                cerr << "For help use -h" << endl;
                exit(-1);
        }
    }

    if (helpRequested) {
        cout << "Usage: ./ipk24chat-client -t [tcp/udp] -s [server address] -d [timer] -r [retries] -p [port]" << endl;
        cout << endl;
        cout << "Running UDP:" << endl;
        cout << "   ./ipk24chat-client -t udp -s serverAddress" << endl;
        cout << "     - `serverAddress`: IP address or server name" << endl;
        cout << endl;
        cout << "   Additional optional parameters:" << endl;
        cout << "     - `-p port`: port number (uint16, default value 4567)" << endl;
        cout << "     - `-d timer`: timer (uint16, default value 250 ms)" << endl;
        cout << "     - `-r retries`: number of retries (uint8, default value 3)" << endl;
        cout << "     - `-h`: help" << endl;
        cout << endl;
        cout << "Running TCP:" << endl;
        cout << "   ./ipk24chat-client -t tcp -s serverAddress" << endl;
        cout << "     - `serverAddress`: IP address or server name" << endl;
        cout << endl;
        cout << "   Additional optional parameters:" << endl;
        cout << "     - `-p port`: port number (uint16, default value 4567)" << endl;
        cout << "     - `-h`: help" << endl;
        cout << "     - you cannot use another parameter with tcp protocol" << endl;
        exit(0);
    }

    if (transportProtocol.empty() || serverAddress.empty())
    {
        cerr << "For help use -h" << endl;
        return -1;
    }

    // Create socket
    int sock = 0;
    if (transportProtocol == "tcp") {
        if(retTime){
            cerr << "cannot combinate -d or -r with tcp" << endl;
            return -1;
        }
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cerr << "TCP socket creation error" << endl;
            return -1;
        }
        // Enable SO_REUSEADDR option
        int enable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        {
            cerr << "setsockopt(SO_REUSEADDR) failed" << endl;
            return -1;
        }
    }
    else if (transportProtocol == "udp") {
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            cerr << "UDP socket creation error" << endl;
            return -1;
        }
    }

    struct pollfd fds[2]; // for sock and for stdin
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    fds[1].fd = STDIN_FILENO; // stdin
    fds[1].events = POLLIN;

    // Hostname to IP address
    struct hostent *server;
    server = gethostbyname(serverAddress.c_str());
    if (server == NULL)
    {
        cerr << "Hostname resolution failed" << endl;
        return -1;
    }


    signal(SIGINT, signalHandler);

    // Connect to server
    if (transportProtocol == "tcp") {  
        // Set server address and port
        struct sockaddr_in serv_addr;
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(port);
                
        clientTCP = new TCP();    
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            cerr << "Connection Failed" << endl;
            //cleanupAndExitTCP(sock);
            return -1;
        }
        clientTCP->sockClose = sock;
        cout << "Authorize yourself, please. If you're unsure how, type /help." << endl;
        clientTCP->startCommunication(sock, clientTCP->username, clientTCP->secret, clientTCP->displayName);

        clientTCP->currentState = clientTCP->nextState(clientTCP->currentState, "", sock);

        if(clientTCP->currentState == END){
            cleanupAndExitTCP(sock);
        }
        while (true)
        {
            int ret = poll(fds, 2, -1); // unlimit timeout
            if (ret == -1)
            {
                cerr << "poll() failed" << endl;
                cleanupAndExitTCP(sock);
            }

            if (fds[0].revents & POLLIN){
                char buffer[1500];
                ssize_t bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead <= 0)
                {
                    cerr << "Connection closed by server" << endl;
                    cleanupAndExitTCP(sock);
                }

                bool hasNonWhitespace = false;
                for (int i = 0; i < bytesRead; ++i)
                {
                    if (!isspace(buffer[i]))
                    {
                        hasNonWhitespace = true;
                        break;
                    }
                }

                if (hasNonWhitespace)
                {
                    clientTCP->currentState = clientTCP->nextState(clientTCP->currentState, buffer, sock);
                    if (clientTCP->currentState == END)
                    {
                        cleanupAndExitTCP(sock);
                    }
                }
            }

            if (fds[1].revents & POLLHUP) {
                cout << "EOF detected on stdin" << endl;
                cleanupAndExitTCP(sock);
            }
            
            // receive from clientTCP
            if (fds[1].revents & POLLIN)
            {   
                clientTCP->sendingFromClient(sock, clientTCP->username, clientTCP->displayName);
                if(clientTCP->currentState == END){
                    cleanupAndExitTCP(sock);
                }
            }
        }
    }
    else if (transportProtocol == "udp") {
        clientUDP = new UDP();
        clientUDP->r = r; // retries
        clientUDP->d = d;
        clientUDP->sockClose = sock;
        clientUDP->setServerAddress(serverAddress, port);
        cout << "Authorize yourself, please. If you're unsure how, type /help." << endl;
        clientUDP->startCommunication(sock, clientUDP->username, clientUDP->secret, clientUDP->displayName);

        if(clientUDP->currentState == END){
            cleanupAndExitUDP(sock);
        }
        // Receive response from the server
        while(true){
            int ret = poll(fds, 2, 1000);
            if (ret == -1)
            {
                cerr << "poll() failed" << endl;
                cleanupAndExitUDP(sock);
            }

            if (fds[0].revents & POLLIN){
                char responseBuffer[1500];
                struct sockaddr_in serverResponseAddr;
                socklen_t serverResponseLen = sizeof(serverResponseAddr);

                ssize_t responseBytesReceived = recvfrom(sock, responseBuffer, sizeof(responseBuffer), 0, (struct sockaddr *)&serverResponseAddr, &serverResponseLen);
                if (responseBytesReceived < 0) {
                    cerr << "Error in receiving response from server" << endl;
                    cleanupAndExitUDP(sock);
                }

                responseBuffer[responseBytesReceived] = '\0';

                clientUDP->currentState = clientUDP->nextState(clientUDP->currentState, responseBuffer, sock);
                if(clientUDP->currentState == END){
                    cleanupAndExitUDP(sock);
                }
            }
            
            // If revents is set to POLLHUP, it means stdin was closed by the client
            if (fds[1].revents & POLLHUP) {
                cout << "EOF detected on stdin" << endl;
                cleanupAndExitUDP(sock);
            }
            
            if (fds[1].revents & POLLIN){
                clientUDP->sendingFromClient(sock, clientUDP->username, clientUDP->displayName);
                if(clientUDP->currentState == END){
                    cleanupAndExitUDP(sock);
                }
            }
            for (auto it = clientUDP->sentMessages.begin(); it != clientUDP->sentMessages.end(); ) {
                auto& msg = *it;

                auto currentTime = std::chrono::steady_clock::now();
                auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - msg.timer);

                // Wait for a certain time (500 milliseconds)
                if (timeDiff.count() > d) {
                    // Check if `retries` for the current message is not zero
                    if (msg.retries > 0) {
                        // Check if confirm is true
                        if (!msg.confirm) {
                            //cout << "Message with messageID " << msg.messageID << " has not been acknowledged for more than 5 seconds." << endl;
                            clientUDP->sendAgain(sock, msg.content);
                            msg.retries--; // Decrement one retry
                            msg.timer = std::chrono::steady_clock::now(); // Set timer to current time
                        } else {
                            // If confirm is true, skip this message
                            ++it;
                            continue;
                        }
                    }
                    else {
                        // If the number of retries is 0, remove the message from the vector
                        it = clientUDP->sentMessages.erase(it);
                        cleanupAndExitUDP(sock);
                    }
                }
                ++it;
            }
        }
    }
    close(sock);

    return 0;
}
