#include "udp.hpp"
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
#include <vector>
#include <string>
#include <chrono>

using namespace std;

UDP::UDP() : currentState(START),sockClose(sock), messageID(0), refMessageID(messageID){}

void UDP::setServerAddress(const string& serverAddress, uint16_t port) {
    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    struct hostent *server;
    server = gethostbyname(serverAddress.c_str());
    if (server == NULL) {
        cerr << "Hostname resolution failed" << endl;
        //currentState = END;
        return;
    }
    
    bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
}

void UDP::startCommunication(int sock, string &username, string &secret, string &displayName){
    currentState = AUTH;
    string input;
    if (getline(cin, input)) {
        stringstream ss(input);
        string command, param1, param2, param3;
        ss >> command >> param1 >> param2 >> param3;

        if (command == "/help"){
            cout << "To authorize, use:" << endl;
            cout << "/auth username secret displayName" << endl;
            startCommunication(sock, username, secret, displayName);
        }
        else if (command != "/auth"){
            cerr << "ERR: Invalid command. Expected '/auth' or '/help'." << endl;
            startCommunication(sock, username, secret, displayName);
        }
        else {
            // Check constraints for username, secret, and displayName
            regex usernameRegex("[A-Za-z0-9\\-]{1,20}");
            regex secretRegex("[A-Za-z0-9\\-]{1,128}");
            regex displayNameRegex("[\\x21-\\x7E]{1,20}");

            if (!regex_match(param1, usernameRegex) || !regex_match(param2, secretRegex) || !regex_match(param3, displayNameRegex)) {
                cerr << "ERR: Invalid input format. Use /auth username secret displayName" << endl;
                startCommunication(sock, username, secret, displayName);
            } else {
                username = param1;
                secret = param2;
                displayName = param3;
                createAuthMessage(sock, username, displayName, secret, messageID);
            }
        }
    } else {
        // If input is empty (EOF detected)
        cerr << "EOF detected. Terminating." << endl;
        currentState = END;
    }
}

void UDP::createConfirmMessage(int sock, int refMessageID) { 
    vector<unsigned char> message;
    
    //(confirm 0x00)
    message.push_back(0x00);

    message.push_back((refMessageID >> 8) & 0xFF);
    message.push_back(refMessageID & 0xFF);

    int bytesSent = sendto(sock, message.data(), message.size(), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (bytesSent < 0) {
        cerr << "Sendto confirm failed" << endl;
    }
}

void UDP::createAuthMessage(int sock, const string& username, const string& displayName, const string& secret, int messageID) {
    vector<unsigned char> message;

    //(AUTH 0x02)
    message.push_back(0x02);

    // Add messageID (2 bytes)
    message.push_back((messageID >> 8) & 0xFF);
    message.push_back(messageID & 0xFF);

    for (char c : username) {
        message.push_back(c);
    }
    message.push_back(0);

    for (char c : displayName) {
        message.push_back(c);
    }
    message.push_back(0);

    for (char c : secret) {
        message.push_back(c);
    }
    message.push_back(0);

    send(sock, message);
}

void UDP::createJoinMessage(int sock, string& channelID, const string& displayName, int messageID) {
    vector<unsigned char> message;

    //(JOIN 0x03)
    message.push_back(0x03);

    // Add messageID (2 bytes)
    message.push_back((messageID >> 8) & 0xFF);
    message.push_back(messageID & 0xFF);

    for (char c : channelID) {
        message.push_back(c);
    }
    message.push_back(0);

    for (char c : displayName) {
        message.push_back(c);
    }
    message.push_back(0);

    send(sock, message);
}

void UDP::createMsgMessage(int sock, string& MessageContents, const string& displayName, int messageID) {
    vector<unsigned char> message;

    //(MSG 0x04)
    message.push_back(0x04);

    // Add messageID (2 bytes)
    message.push_back((messageID >> 8) & 0xFF);
    message.push_back(messageID & 0xFF);

    for (char c : displayName) {
        message.push_back(c);
    }
    message.push_back(0);

    for (char c : MessageContents) {
        message.push_back(c);
    }
    message.push_back(0);

    send(sock, message);
}

void UDP::createErrMessage(int sock, string& MessageContents, const string& displayName, int messageID) { 
    vector<unsigned char> message;

    //(ERR 0xFE)
    message.push_back(0xFE);

    // Add messageID (2 bytes)
    message.push_back((messageID >> 8) & 0xFF);
    message.push_back(messageID & 0xFF);

    for (char c : displayName) {
        message.push_back(c);
    }
    message.push_back(0);

    for (char c : MessageContents) {
        message.push_back(c);
    }
    message.push_back(0);

    send(sock, message);
}

void UDP::createByeMessage(int sock, int messageID) { 
    vector<unsigned char> message;

    //(BYE 0xFF)
    message.push_back(0xFF);

    // Add messageID (2 bytes)
    message.push_back((messageID >> 8) & 0xFF);
    message.push_back(messageID & 0xFF);

    send(sock, message);
}

void UDP::send(int sock, const vector<unsigned char>& message) {
    int bytesSent = sendto(sock, message.data(), message.size(), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (bytesSent < 0) {
        cerr << "Sendto failed" << endl;
    } else {
        MessageInfo messageSent;
        messageSent.timer = chrono::steady_clock::now();
        messageSent.retries = r;
        messageSent.messageID = messageID;
        messageSent.content = message;
        messageSent.confirm = false;
        sentMessages.push_back(messageSent);
        messageID++;
    }
}

void UDP::sendAgain(int sock, const vector<unsigned char>& message){
   int bytesSent = sendto(sock, message.data(), message.size(), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (bytesSent < 0) {
        cerr << "Sendto again failed" << endl;
    } 
}

void UDP::handleMsg(char* buffer) {
    //cout << "mesg" << endl;
    uint16_t SmessageID = (buffer[1] << 8) | buffer[2];

    // Check if messageID is in messageIDsFromServer vector
    if (find(messageIDsFromServer.begin(), messageIDsFromServer.end(), SmessageID) != messageIDsFromServer.end()) {
        // Message ID found in messageIDsFromServer vector, skipping functionality
        return;
    }

    // Find the end of DisplayName field
    int displayNameEnd = 3; // Starting from index 3
    while (buffer[displayNameEnd] != '\0') {
        ++displayNameEnd;
    }

    // Extract DisplayName field
    string displayName(buffer + 3, buffer + displayNameEnd);

    // Find the end of MessageContents field
    int messageContentStart = displayNameEnd + 1;
    int messageContentEnd = messageContentStart; // Start from displayNameEnd + 1
    while (buffer[messageContentEnd] != '\0') {
        ++messageContentEnd;
    }

    // Extract MessageContents field
    string messageContent(buffer + messageContentStart, buffer + messageContentEnd);

    cout << displayName << ": " << messageContent << endl; 
}

void UDP::handleErr(char* buffer) {
    uint16_t SmessageID = (buffer[1] << 8) | buffer[2];

    // Check if messageID is in messageIDsFromServer vector
    if (find(messageIDsFromServer.begin(), messageIDsFromServer.end(), SmessageID) != messageIDsFromServer.end()) {
        // Message ID found in messageIDsFromServer vector, skipping functionality
        return;
    }

    // Find the end of DisplayName field
    int serverNameEnd = 3; // Starting from index 3
    while (buffer[serverNameEnd] != '\0') {
        ++serverNameEnd;
    }

    // Extract DisplayName field
    string serverName(buffer + 3, buffer + serverNameEnd);

    // Find the end of MessageContents field
    int messageContentStart = serverNameEnd + 1;
    int messageContentEnd = messageContentStart; // Start from displayNameEnd + 1
    while (buffer[messageContentEnd] != '\0') {
        ++messageContentEnd;
    }

    // Extract MessageContents field
    string messageContent(buffer + messageContentStart, buffer + messageContentEnd);

    cerr << "ERR FROM " << serverName << ": " <<  messageContent << endl; 
}

bool UDP::handleReply(char* buffer) {
    //cout << "reply" << endl;
    uint16_t SmessageID = (buffer[1] << 8) | buffer[2];

    // Check if messageID is in messageIDsFromServer vector
    if (find(messageIDsFromServer.begin(), messageIDsFromServer.end(), SmessageID) != messageIDsFromServer.end()) {
        // Message ID found in messageIDsFromServer vector, skipping functionality
        return true;
    }

    uint8_t result = buffer[3];
    uint16_t refMessageID = (buffer[4] << 8) | buffer[5];
    char* messageContents = buffer + 6;

    size_t contentLength = strlen(messageContents);

    if (strchr(messageContents, '\0') != nullptr) {
        ++contentLength;
    }

    string messageContentString(messageContents, contentLength);

    if (result == 1) {
        cerr << "Success: " << messageContents << endl;
        for (size_t i = 0; i < sentMessages.size(); ++i) {
            if (sentMessages[i].messageID == refMessageID) {
                if (!sentMessages[i].confirm) {
                    sentMessages[i].confirm = true; 
                } else {
                    sentMessages.erase(sentMessages.begin() + i); 
                }
                return true;
            }
            /*else{
                exit(-1);
            }*/
        }
        return false;
    } else {
        cerr << "Failure: " << messageContents << endl;
        for (size_t i = 0; i < sentMessages.size(); ++i) {
            if (sentMessages[i].messageID == refMessageID) {
                sentMessages.erase(sentMessages.begin() + i);
            }
        }
        return false;
    }
}

void UDP::handleConfirm(char* buffer){
    //cerr << "confirm" << endl;
    uint16_t refMessageID = (buffer[1] << 8) | buffer[2];
    for (size_t i = 0; i < sentMessages.size(); ++i) {
        if (sentMessages[i].messageID == refMessageID) {

            if (!sentMessages[i].confirm) {
                sentMessages[i].confirm = true;
            } else {
                sentMessages.erase(sentMessages.begin() + i); 
            }
            break;
        }
    }
}

void UDP::sendingFromClient(int sock, string &content, string &displayName){
    string input;
    if (!getline(cin, input)) {
        currentState = END;
        return;
    }

    if (input.empty()){
        currentState = END;
    }
    else{
        stringstream ss(input);
        string command;
        ss >> command;
        if (currentState == OPEN){
            if (command == "/auth"){
                cerr << "ERR: You are already authorized. This command cannot be used." << endl;
            }
            else if (command == "/join") {
                string channelID;
                if (ss >> channelID && ss.eof()) {
                    sendingJoin(sock, content, displayName, channelID);
                } else {
                    cerr << "ERR: Invalid usage of /join command. Usage: /join <channelID>" << endl;
                }
            }
            else if (command == "/rename") {
                string newName;
                if (ss >> newName && ss.eof()) {
                    displayName = newName;
                } else {
                    cerr << "ERR: Invalid usage of /rename command. Usage: /rename <newName>" << endl;
                }
            }
            else if (command == "/help"){
                cout << "To join channels, use:" << endl;
                cout << "/join channelID" << endl;
                cout << "To change your display name, use:" << endl;
                cout << "/rename newName" << endl;
            }
            else{
                content = input;
                createMsgMessage(sock, content, displayName, messageID);
            }
        }
    }
}

void UDP::sendingJoin(int sock, string &content, string &displayName, string &channelID) {
    createJoinMessage(sock, channelID, displayName, messageID);
    bool result = false;
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    while (true) {
        int ret = poll(fds, 1, 1000);
        if (ret == -1) {
            cerr << "poll() failed" << endl;
            byeSent = true;
            break;
        }
        if (fds[0].revents & POLLIN) {
            char responseBuffer[1500];
            struct sockaddr_in serverResponseAddr;
            socklen_t serverResponseLen = sizeof(serverResponseAddr);

            ssize_t responseBytesReceived = recvfrom(sock, responseBuffer, sizeof(responseBuffer), 0, (struct sockaddr *)&serverResponseAddr, &serverResponseLen);
            if (responseBytesReceived < 0) {
                cerr << "Error in receiving response from server" << endl;
                break;
            }

            responseBuffer[responseBytesReceived] = '\0';

            uint8_t messageType = responseBuffer[0];
            uint16_t messageID = (responseBuffer[1] << 8) | responseBuffer[2];

            switch (messageType) {
                case 0x01:
                    createConfirmMessage(sock, messageID);
                    result = handleReply(responseBuffer);
                    break; 
                case 0x00:
                    handleConfirm(responseBuffer);
                    break;
                case 0x04:
                    createConfirmMessage(sock, messageID);
                    handleMsg(responseBuffer);
                    break; 
            }
            if (result) {
                break;
            }
        }
        for (auto it = sentMessages.begin(); it != sentMessages.end();) {
            auto& msg = *it;

            auto currentTime = std::chrono::steady_clock::now();
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - msg.timer);

            if (timeDiff.count() > d) {
                if (msg.retries > 0) {
                    if (!msg.confirm) {
                        sendAgain(sock, msg.content);
                        msg.retries--;
                        msg.timer = std::chrono::steady_clock::now();
                    } else {
                        ++it;
                        continue;
                    }
                } else {
                    it = sentMessages.erase(it);
                    currentState = END;
                    result = true;
                    break;
                }
            }
            ++it;
        }
        if (result) {
            break;
        }
    }
}


State UDP::nextState(State currentState, char* responseBuffer, int sock) {
    uint8_t messageType = responseBuffer[0];
    uint16_t messageID = (responseBuffer[1] << 8) | responseBuffer[2];

    bool result = false;
    switch (currentState){
        case START:
            return AUTH;
        case AUTH:
            switch (messageType) {
                case 0x00:
                    //cout << "CONFIRM" << endl;
                    handleConfirm(responseBuffer);
                    return AUTH;
                case 0x01:
                    //cout << "REPLY" << endl;
                    createConfirmMessage(sock, messageID);
                    result = handleReply(responseBuffer);
                    messageIDsFromServer.push_back(messageID);
                    if(result){
                        return OPEN;
                    }
                    else{
                        startCommunication(sock, username, secret, displayName);
                        return AUTH;
                    }
                case 0xFE:
                    //cout << "ERR" << endl;
                    createConfirmMessage(sock, messageID);
                    handleErr(responseBuffer);
                    messageIDsFromServer.push_back(messageID);
                    return END;
                default:
                    return END;
                }
        case OPEN:
            switch (messageType) {
                case 0x00:
                    //cout << "CONFIRM" << endl;
                    handleConfirm(responseBuffer);
                    return OPEN;
                case 0x01:
                    //cout << "REPLY" << endl;
                    createConfirmMessage(sock, messageID);
                    result = handleReply(responseBuffer);
                    messageIDsFromServer.push_back(messageID);
                    if(result){
                        return OPEN;
                    }
                    else{
                        return AUTH;
                    }
                case 0x04:
                    //cout << "MSG" << endl;
                    createConfirmMessage(sock, messageID);
                    handleMsg(responseBuffer);
                    messageIDsFromServer.push_back(messageID);
                    return OPEN;
                case 0xFE:
                    //cout << "ERR" << endl;
                    createConfirmMessage(sock, messageID);
                    handleErr(responseBuffer);
                    messageIDsFromServer.push_back(messageID);
                    return END;
                case 0xFF:
                    //cout << "BYE" << endl;
                    createConfirmMessage(sock, messageID);
                    return END;
                default:
                    cerr << "ERR: invalid message from server" << endl;
                    string content = "Ivalid message from server";
                    createErrMessage(sock, content, displayName, messageID);
                    return END;
                }
        case ERROR:
            return END;
        default:
            return START;
    }
}

void UDP::printState(State state) {
    switch(state) {
        case START:
            cout << "Current state: START" << endl;
            break;
        case AUTH:
            cout << "Current state: AUTH" << endl;
            break;
        case OPEN:
            cout << "Current state: OPEN" << endl;
            break;
        case END:
            cout << "Current state: END" << endl;
            break;
        case ERROR:
            cout << "Current state: ERROR" << endl;
            break;
        default:
            cout << "Unknown state" << endl;
            break;
    }
}

void UDP::waitForConfirmation(int sock) {
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    while (true) {
        int ret = poll(fds, 1, 1000);
        if (ret == -1){
            cerr << "poll() failed" << endl;
            byeSent = true;
            break;
        }

        if (fds[0].revents & POLLIN){
            char responseBuffer[1500];
            struct sockaddr_in serverResponseAddr;
            socklen_t serverResponseLen = sizeof(serverResponseAddr);

            ssize_t responseBytesReceived = recvfrom(sock, responseBuffer, sizeof(responseBuffer), 0, (struct sockaddr *)&serverResponseAddr, &serverResponseLen);
            if (responseBytesReceived < 0) {
                cerr << "Error in receiving response from server" << endl;
                return;
            }

            responseBuffer[responseBytesReceived] = '\0';

            uint8_t messageType = responseBuffer[0];

            switch (messageType) {
                case 0x00:
                    handleConfirm(responseBuffer);
                    byeSent = true;
                    break;
                default:
                    cerr << "Unexpected message type while waiting for confirmation" << endl;
                    break;
            }
        }
        for (auto it = sentMessages.begin(); it != sentMessages.end();) {
            auto& msg = *it;

            auto currentTime = std::chrono::steady_clock::now();
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - msg.timer);

            if (timeDiff.count() > d) {
                if (msg.retries > 0) {
                    if (!msg.confirm) {
                        sendAgain(sock, msg.content);
                        msg.retries--;
                        msg.timer = std::chrono::steady_clock::now(); 
                    } else {
                        ++it;
                        continue;
                    }
                } else {
                    it = sentMessages.erase(it);
                    byeSent = true;
                    break;
                }
            }
            ++it;
        }
        if (byeSent) {
            break;
        }
    }
}

UDP::~UDP() {
    createByeMessage(sockClose, messageID);
    if (!byeSent) {
        waitForConfirmation(sockClose);
    }
    if (sockClose != -1) {
        close(sockClose);
        //cout << "Socket closed." << endl;
    }
}