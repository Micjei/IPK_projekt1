/**
* @file udp.hpp
* @brief Header file for the TCP class
*/
#ifndef TCP_HPP
#define TCP_HPP

#include <iostream>
#include <string>
#include <regex>
#include <sstream>
#include <vector>

/**
* @brief Enumeration representing possible states of the TCP communication
*/
#ifndef STATE
#define STATE
enum State {
    START, /**< Initial state */
    AUTH,  /**< Authentication state */
    OPEN,  /**< Communication open state */
    END,   /**< End of communication state */
    ERROR  /**< Error state */
};
#endif // STATE

/**
* @class TCP
* @brief Class representing TCP communication functionality
*/
class TCP {
private: 
    int sock; /**< Socket descriptor */
public:
    std::string username; /**< Username for authentication */
    std::string secret; /**< Secret for authentication */
    std::string displayName; /**< Display name */
    State currentState; /**< Current state of the communication */
    int sockClose; /**< Socket descriptor for final bye message */

    /**
    * @brief Constructor for the TCP class
    */
    TCP();

    /**
    * @brief Initiates communication with the server (first message).
    * 
    * This function handles the initial communication with the server. It prompts the user
    * for input, expecting an authentication command (/auth) followed by username, secret, and
    * display name. If the input matches the expected format, an authentication message is
    * created and sent to the server. If the input is invalid or EOF (end-of-file) is detected,
    * the function terminates the communication.
    * 
    * @param sock The socket for communication with the server.
    * @param username Reference to the username variable to be set based on user input.
    * @param secret Reference to the secret variable to be set based on user input.
    * @param displayName Reference to the display name variable to be set based on user input.
    */
    void startCommunication(int sock, std::string &username, std::string &secret, std::string &displayName);

    /**
    * @brief Sends an authentication message over TCP.
    *
    * This function constructs an authentication message with the provided username, secret, and display name,
    * and sends it over the TCP socket specified by @p sock.
    *
    * @param sock The socket over which to send the message.
    * @param username The username for authentication.
    * @param secret The secret for authentication.
    * @param displayName The display name associated with the user.
    */
    void sendAuthentication(int sock, const std::string &username, const std::string &secret, const std::string &displayName);

    /**
    * @brief Sends a join message over TCP.
    *
    * This function constructs a join message with the provided channel ID and display name,
    * and sends it over the TCP socket specified by @p sock.
    *
    * @param sock The socket over which to send the message.
    * @param channelID The ID of the channel to join.
    * @param displayName The display name associated with the user.
    */
    void sendJoin(int sock, const std::string &channelID, const std::string &displayName);

    /**
    * @brief Sends an error message over TCP.
    *
    * This function constructs an error message with the provided content and display name,
    * and sends it over the TCP socket specified by @p sock.
    *
    * @param sock The socket over which to send the message.
    * @param content The content of the error message.
    * @param displayName The display name associated with the user.
    */
    void sendERR(int sock, const std::string &content, const std::string &displayName);

    /**
    * @brief Sends a message over TCP.
    *
    * This function constructs a message with the provided content and display name,
    * and sends it over the TCP socket specified by @p sock.
    *
    * @param sock The socket over which to send the message.
    * @param content The content of the message.
    * @param displayName The display name associated with the user.
    */
    void sendMSG(int sock, const std::string &content, const std::string &displayName);

    /**
    * @brief Sends a BYE message over TCP.
    *
    * This function sends a BYE message over the TCP socket specified by @p sock.
    *
    * @param sock The socket over which to send the message.
    */
    void sendBYE(int sock);

    /**
    * @brief Handles input from the client and sends appropriate messages over TCP.
    *
    * This function reads input from the standard input (stdin) and processes it based on the current state of the client.
    * If the input is empty, it sets the current state to END and exits.
    * If the input is a command, it executes the corresponding action based on the command and the current state.
    * Supported commands include:
    *   - /auth: Displays an error message.
    *   - /join <channelID>: Sends a join message to the server.
    *   - /rename <newName>: Changes the display name of the client.
    * If the input is not a command, it treats it as a message and sends it to the server as a regular message.
    *
    * @param sock The socket over which to send messages.
    * @param content A reference to the content/message to be sent.
    * @param displayName A reference to the display name of the client.
    */
    void sendingFromClient(int sock, std::string &content, std::string &displayName);

    /**
    * @brief Sends a join message to the server and handles the server response.
    *
    * This function sends a join message to the server with the specified channel ID and display name.
    * It then enters a loop to receive and process the server's response. If the response indicates success (REPLY OK IS),
    * it prints a success message and exits the loop. Otherwise, it prints a failure message.
    *
    * @param sock The socket over which to send and receive messages.
    * @param content A reference to the content/message received from the server.
    * @param displayName A reference to the display name of the client.
    * @param channelID A reference to the ID of the channel to join.
    */
    void sendingJoin(int sock, std::string &content, std::string &displayName, std::string &channelID);

    /**
    * @brief Determines the next state based on the current state and server response.
    *
    * This function implements a finite state machine to determine the next state of the client based on the current state and the type of message received from the server.
    * The client transitions between states according to predefined rules, responding to server messages appropriately.
    *
    * @param currentState The current state of the client, defined by the finite state machine.
    * @param serverResponse Buffer containing the server response.
    * @param sock Socket for communication with the server.
    * @return The next state of the client, as determined by the finite state machine.
    */
    State nextState(State currentState, const std::string &serverResponse, int sock);

    /**
    * @brief Prints the current state of the client.
    *
    * This function is a utility for tracking the state of the client within the finite state machine.
    * It prints out the current state of the client to the standard output stream.
    * 
    * This function was primarily used for testing and debugging purposes.
    *
    * @param state The current state of the client.
    */
    void printState(State state);

    /**
    * @brief Destructor for the TCP class
    *
    * Close the sockClose and sends the last bye message to the server.
    */
    ~TCP();
};


#endif /* TCP_HPP */
