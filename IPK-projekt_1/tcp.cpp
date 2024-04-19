#include "tcp.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>
#include <algorithm> 
#include <atomic>    
#include <poll.h>
#include <regex>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <string>

using namespace std;

TCP::TCP() : currentState(START), sockClose(sock) {}

void TCP::startCommunication(int sock, string &username, string &secret, string &displayName){
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
        else if (command != "/auth")
        {
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
                sendAuthentication(sock, username, secret, displayName);
            }
        }
    } else {
        cerr << "EOF detected. Terminating." << endl;
        currentState = END;
    }
}

void TCP::sendAuthentication(int sock, const string &username, const string &secret, const string &displayName){
    string authMessage = "AUTH " + username + " AS " + displayName + " USING " + secret + "\r\n";
    send(sock, authMessage.c_str(), authMessage.length(), 0);
}

void TCP::sendJoin(int sock, const string &channelID, const string &displayName){
    string joinMessage = "JOIN " + channelID + " AS " + displayName + "\r\n";
    send(sock, joinMessage.c_str(), joinMessage.length(), 0);
}

void TCP::sendERR(int sock, const string &content, const string &displayName){
    string errMessage = "ERR FROM " + displayName + " IS " + content + "\r\n";
    send(sock, errMessage.c_str(), errMessage.length(), 0);
}

void TCP::sendMSG(int sock, const string &content, const string &displayName){
    string msgMessage = "MSG FROM " + displayName + " IS " + content + "\r\n";
    send(sock, msgMessage.c_str(), msgMessage.length(), 0);
}

void TCP::sendBYE(int sock){
    string bye = "BYE\r\n";
    if(send(sock, bye.c_str(), bye.length(), 0) < 0) {
        cerr << "Failed to send BYE message" << endl;
    }
}

void TCP::sendingFromClient(int sock, string &content, string &displayName){
    string input;
    getline(cin, input);
    if (input.empty()){
        cout << currentState << endl;
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
                sendMSG(sock, content, displayName);
            }
        }
    }
}

void TCP::sendingJoin(int sock, string &content, string &displayName, string &channelID){
    sendJoin(sock, channelID, displayName);
    while(true){
        // server receive
        char buffer[1500];
        ssize_t bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            cerr << "Connection closed by server" << endl;
            break;
        }

        // client receive
        buffer[bytesRead] = '\0';
        string serverResponse(buffer);
        stringstream ss(serverResponse);
        string firstWord, secondWord, thirdWord, fourthWord;
        ss >> firstWord >> secondWord >> thirdWord >> fourthWord;
        // find third space position
        size_t firstSpacePos = serverResponse.find_first_of(" \t\r\n");
        if (firstSpacePos != string::npos) {
            size_t secondSpacePos = serverResponse.find_first_of(" \t\r\n", firstSpacePos + 1);
            if (secondSpacePos != string::npos) {
                size_t thirdSpacePos = serverResponse.find_first_of(" \t\r\n", secondSpacePos + 1);
                // read everything after third space
                if (thirdSpacePos != string::npos) {
                    content = serverResponse.substr(thirdSpacePos + 1);
                } else {
                    content = "";
                }
            }
        }
        if (firstWord == "REPLY" && secondWord == "OK" && thirdWord == "IS"){
            cerr << "Success: " << content;
            break;
        }
        else if(firstWord == "REPLY" && secondWord == "NOK" && thirdWord == "IS"){
            cerr << "Failure: " << content;
        }
        else if (firstWord == "MSG" && secondWord == "FROM" && fourthWord == "IS"){
            string content = serverResponse.substr(serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n") + 1)) + 1) + 1));
            cout << thirdWord << ":" << content << endl;
        }
        else{
            currentState = END;
            break;
        }
    }
}

State TCP::nextState(State currentState, const string &serverResponse, int sock){
    switch (currentState){
    case START:
        return AUTH;
    case AUTH:
    {
        stringstream ss(serverResponse);
        string firstWord, secondWord, thirdWord;
        ss >> firstWord >> secondWord >> thirdWord;
        if (firstWord == "REPLY" && secondWord == "OK" && thirdWord == "IS"){
            string content = serverResponse.substr(serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n") + 1) + 1));
            cerr << "Success:" << content;
            return OPEN;
        }
        else if (firstWord == "REPLY" && secondWord == "NOK" && thirdWord == "IS"){
            string content = serverResponse.substr(serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n") + 1) + 1));
            cerr << "Failure:" << content << endl;
            startCommunication(sock, username, secret, displayName);
            return AUTH;
        }
        else if(firstWord == "ERR" && secondWord == "FROM"){
            string content = serverResponse.substr(serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n") + 1)) + 1) + 1));
            cerr << "ERR FROM " << thirdWord << ":" << content << endl;
            return END;
        }
    }
    case OPEN:
    {
        stringstream ss(serverResponse);
        string firstWord, secondWord, DNAME, fourthWord;
        ss >> firstWord >> secondWord >> DNAME >> fourthWord;
        if (serverResponse == "BYE\r\n"){
            return END;
        }
        else if (firstWord == "ERR" && secondWord == "FROM"){
            string content = serverResponse.substr(serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n") + 1)) + 1) + 1));
            cerr << "ERR FROM " << DNAME << ":" << content << endl;
            sendBYE(sock);
            return END;
        }
        else if (firstWord == "MSG" && secondWord == "FROM" && fourthWord == "IS"){
            string content = serverResponse.substr(serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n", serverResponse.find_first_of(" \t\r\n") + 1)) + 1) + 1));
            cout << DNAME << ":" << content << endl;
            return OPEN;
        }
        else{
            cerr << "ERR: invalid message from server" << endl;
            string content = "Invalid message from server";
            cerr << serverResponse << endl;
            sendERR(sock, content, displayName);
            return END;
        }
    }
    case ERROR:
    {
        return END;
    }
    case END:
        return END;
    default:
        return START;
    }
}

void TCP::printState(State state){
    switch (state){
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
    }
}

TCP::~TCP() {
    sendBYE(sockClose);
    if (sockClose != -1) {
        close(sockClose);
        //cout << "Socket closed." << endl;
    }
}