/**
* @file udp.hpp
* @brief Header file for the UDP class
*/
#ifndef UDP_HPP
#define UDP_HPP

#include <iostream>
#include <string>
#include <regex>
#include <sstream>
#include <vector>
#include <netinet/in.h>
#include <chrono>
#include <thread>

/**
* @brief Enumeration representing possible states of the UDP communication
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
* @brief Structure representing information about a message
*/
struct MessageInfo {
    int retries; /**< Number of retries for the message */
    int messageID; /**< ID of the message */
    std::vector<unsigned char> content; /**< Content of the message */
    std::chrono::steady_clock::time_point timer; /**< Timer for the message */
    bool confirm; /**< Flag indicating if the message has been confirmed */
};

/**
* @class UDP
* @brief Class representing UDP communication functionality
*/
class UDP {
private: 
    int sock; /**< Socket descriptor */
public:
    std::vector<MessageInfo> sentMessages; /**< Vector of sent messages */
    std::string username; /**< Username for authentication */
    std::string secret; /**< Secret for authentication */
    std::string displayName; /**< Display name */
    State currentState; /**< Current state of the communication */
    int sockClose; /**< Socket descriptor for final bye message */
    int messageID; /**< Current message ID */
    int refMessageID; /**< Reference message ID */
    int r; /**< Number of retries */
    int d;
    struct sockaddr_in serverAddr; /**< Server address */
    std::vector<int> messageIDsFromServer; /**< Message IDs received from the server */
    bool byeSent = false;

    /**
    * @brief Constructor for the UDP class
    */
    UDP();
    /**
    * @brief Sets the server address and port
    * @param serverAddress The server address
    * @param port The port number
    */
    void setServerAddress(const std::string& serverAddress, uint16_t port);

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
    * @brief Creates and sends a confirmation message to the server.
    * 
    * This function constructs a confirmation message with the provided reference message ID
    * and sends it to the server using the specified socket.
    * 
    * @param sock The socket for communication with the server.
    * @param refMessageID The reference message ID to confirm.
    */
    void createConfirmMessage(int sock, int refMessageID);

    /**
    * @brief Creates an authentication message.
    * 
    * This function constructs an authentication message to be sent over UDP.
    * The message format consists of the AUTH command followed by message ID,
    * username, display name, and secret, each separated by null terminators.
    * 
    * @param sock The socket to send the message through.
    * @param username The username for authentication.
    * @param displayName The display name associated with the user.
    * @param secret The secret key for authentication.
    * @param messageID The unique message ID associated with the message.
    */
    void createAuthMessage(int sock, const std::string& username, const std::string& displayName, const std::string& secret, int messageID);

    /**
    * @brief Creates and sends a JOIN message to the server.
    * 
    * This function constructs a JOIN message with the provided channel ID, display name,
    * and message ID, and sends it to the server using the specified socket.
    * 
    * @param sock The socket for communication with the server.
    * @param channelID The ID of the channel to join.
    * @param displayName The display name of the user joining the channel.
    * @param messageID The unique ID of the message.
    */
    void createJoinMessage(int sock, std::string& channelID, const std::string& displayName, int messageID);

    /**
    * @brief Creates and sends a MSG message to the server.
    * 
    * This function constructs a MSG message with the provided message contents, display name,
    * and message ID, and sends it to the server using the specified socket.
    * 
    * @param sock The socket for communication with the server.
    * @param MessageContents The contents of the message to be sent.
    * @param displayName The display name of the user sending the message.
    * @param messageID The unique ID of the message.
    */
    void createMsgMessage(int sock, std::string& MessageContents, const std::string& displayName, int messageID);

    /**
    * @brief Creates and sends an ERR message to the server.
    * 
    * This function constructs an ERR message with the provided message contents, display name,
    * and message ID, and sends it to the server using the specified socket.
    * 
    * @param sock The socket for communication with the server.
    * @param MessageContents The contents of the error message to be sent.
    * @param displayName The display name of the user sending the error message.
    * @param messageID The unique ID of the error message.
    */
    void createErrMessage(int sock, std::string& MessageContents, const std::string& displayName, int messageID);

    /**
    * @brief Creates and sends a BYE message to the server.
    * 
    * This function constructs a BYE message with the provided message ID and sends it to
    * the server using the specified socket.
    * 
    * @param sock The socket for communication with the server.
    * @param messageID The unique ID of the BYE message.
    */
    void createByeMessage(int sock, int messageID); 

    /**
    * @brief Sends a message to the server and records it for tracking.
    * 
    * This function sends the provided message to the server using the specified socket.
    * It also records information about the sent message for tracking purposes, including
    * the number of retries, the timestamp for retry, and the message ID, etc.
    * 
    * @param sock The socket for communication with the server.
    * @param message The message to be sent.
    * 
    */
    void send(int sock, const std::vector<unsigned char>& message);

    /**
    * @brief Resends a message to the server.
    * 
    * This function resends a previously sent message to the server using the provided socket.
    * It is typically used when a message needs to be resent due to a lack of response or an error.
    * 
    * @param sock The socket for communication with the server.
    * @param message The message to be resent.
    *
    */
    void sendAgain(int sock, const std::vector<unsigned char>& message);

    /**
    * @brief Handles the reception and processing of a message from the server.
    * 
    * This function processes a message received from the server.
    * It extracts the sender's display name and the message content from the received buffer
    * and prints them to the standard output.
    * 
    * @param buffer server buffer.
    * 
    * The function first checks if the message ID is already present in the list of message IDs
    * received from the server. If the message ID is found, the function skips further processing.
    * 
    * The display name and message content are then extracted from the buffer and printed
    * to the standard output in the format "DisplayName: MessageContent".
    */
    void handleMsg(char* buffer);

    /**
    * @brief Handles the reception and processing of an error message from the server.
    * 
    * This function processes an error message received from the server.
    * It extracts the server name and the error message content from the received buffer
    * and prints them to the standard error output.
    * 
    * @param buffer server buffer.
    * 
    * The function first checks if the message ID is already present in the list of message IDs
    * received from the server. If the message ID is found, the function skips further processing.
    * 
    * The server name and error message content are then extracted from the buffer and printed
    * to the standard error output in the format "ERR FROM ServerName: ErrorMessage".
    */
    void handleErr(char* buffer);

    /**
    * @brief Handles the reception and processing of a reply message from the server.
    * 
    * The function first checks if the message ID is already present in the list of message IDs
    * received from the server. If the message ID is found, the function skips further processing.
    * 
    * The result, reference message ID, and message contents are then extracted from the buffer.
    * If the result is a success (1), the function prints the message contents to the standard error output
    * with the "Success" prefix and updates the confirmation status of the corresponding sent message
    * in the list of sent messages. If the result is a failure (0), the function prints the message contents
    * to the standard error output with the "Failure" prefix and removes the corresponding sent message
    * from the list of sent messages.
    * 
    * @param buffer server buffer.
    * @return true if the message is successfully handled, false otherwise.
    *
    */
    bool handleReply(char* buffer);

    /**
    * @brief Handles the confirmation of message receipt from the server.
    * 
    * The function extracts the reference message ID from the received buffer
    * and iterates through the list of sent messages to find the corresponding
    * message entry. If the message entry is found, the function updates
    * its confirmation status. If the confirmation is received for the first time,
    * the confirmation flag is set to true. If the confirmation is received again,
    * indicating a replying and confirmation was received, and removes the corresponding sent message
    * from the list of sent messages.
    * 
    * @param buffer server buffer.
    * 
    */
    void handleConfirm(char* buffer);

    /**
    * @brief Manages message sending by the client to the server.
    *
    * This function reads input from the console and performs corresponding actions based on the current communication state.
    * Possible commands include /auth for authentication, /join for joining a channel, /rename for changing the displayed name,
    * and sending any arbitrary message as input text, if the input is empty, sets currentState to END.
    *
    * @param sock Socket for communication with the server.
    * @param content Content of the message to be sent.
    * @param displayName Display name of the client.
    */
    void sendingFromClient(int sock, std::string &content, std::string &displayName);

    /**
    * @brief Sends a join message to the server and handles server responses.
    *
    * This function sends a join message to the server, indicating the client's intention to join a specific channel.
    * It then waits for a response from the server, handling confirmation or rejection accordingly.
    *
    * @param sock Socket for communication with the server.
    * @param content Content of the join message (unused).
    * @param displayName Display name of the client (unused).
    * @param channelID ID of the channel the client wants to join.
    */
    void sendingJoin(int sock, std::string &content, std::string &displayName, std::string &channelID);

    /**
    * @brief Determines the next state based on the current state and server response.
    *
    * This function implements a finite state machine to determine the next state of the client based on the current state and the type of message received from the server.
    * The client transitions between states according to predefined rules, responding to server messages appropriately.
    *
    * @param currentState The current state of the client, defined by the finite state machine.
    * @param responseBuffer Buffer containing the server response.
    * @param sock Socket for communication with the server.
    * @return The next state of the client, as determined by the finite state machine.
    */
    State nextState(State currentState, char* responseBuffer, int sock);

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
    * @brief Waits for confirmation from the server.
    *
    * This function uses poll to wait for incoming messages on the specified socket.
    * It processes incoming messages and handles confirmations accordingly. 
    * Additionally, it handles periodic resending of messages and removes messages 
    * from the sentMessages vector if the maximum number of retries is reached.
    *
    * @param sock The socket to wait for confirmation on.
    */
    
    void waitForConfirmation(int sock);
    /**
    * @brief Destructor for the UDP class
    *
    * Close the sockClose and sends the last bye message to the server.
    */
    ~UDP();
};


#endif /* UDP_HPP */
