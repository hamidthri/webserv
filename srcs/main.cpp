// #include <iostream>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <cstring>
// #include <cstdlib>
// #include <sys/event.h>
// #include <errno.h>
// #include "lexer.hpp"
// #include "parser.hpp"
// #include "request_parser.hpp"
// #include "response_handler.hpp"
// #include <csignal>
// #include <sys/stat.h>
// #include <string>
// #include <stdexcept>

// #define MAX_EVENTS 10
// #define BUFFER_SIZE 1024

// class WebServer
// {
// private:
//     int server_fd;
//     struct sockaddr_in address;
//     socklen_t addrlen;
//     int kq;

//     void setNonBlocking(int sock)
//     {
//         int flags = fcntl(sock, F_GETFL, 0);
//         if (flags == -1)
//         {
//             perror("fcntl F_GETFL");
//             exit(EXIT_FAILURE);
//         }
//         if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
//         {
//             perror("fcntl F_SETFL O_NONBLOCK");
//             exit(EXIT_FAILURE);
//         }
//     }

// public:
//     WebServer(int port)
//     {
//         // Create socket file descriptor
//         if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
//         {
//             perror("socket failed");
//             exit(EXIT_FAILURE);
//         }

//         // Set socket options
//         int opt = 1;
//         if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
//         {
//             perror("setsockopt");
//             exit(EXIT_FAILURE);
//         }

//         // Initialize address structure
//         memset(&address, 0, sizeof(address));
//         address.sin_family = AF_INET;
//         address.sin_addr.s_addr = INADDR_ANY;
//         address.sin_port = htons(port);

//         // Bind the socket to the network address and port
//         if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
//         {
//             perror("bind failed");
//             exit(EXIT_FAILURE);
//         }

//         // Start listening for connections
//         if (listen(server_fd, SOMAXCONN) < 0)
//         {
//             perror("listen");
//             exit(EXIT_FAILURE);
//         }

//         addrlen = sizeof(address);
//         std::cout << "Server listening on port " << port << std::endl;

//         // Create kqueue instance
//         kq = kqueue();
//         if (kq == -1)
//         {
//             perror("kqueue");
//             exit(EXIT_FAILURE);
//         }

//         // Add server socket to kqueue
//         struct kevent ev;
//         EV_SET(&ev, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
//         if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1)
//         {
//             perror("kevent: server_fd");
//             exit(EXIT_FAILURE);
//         }

//         setNonBlocking(server_fd);
//     }

//     void run()
//     {
//         struct kevent events[MAX_EVENTS];
//         while (true)
//         {
//             int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
//             if (nev == -1)
//             {
//                 perror("kevent wait");
//                 exit(EXIT_FAILURE);
//             }

//             for (int i = 0; i < nev; i++)
//             {
//                 if (events[i].ident == (uintptr_t)server_fd)
//                 {
//                     // New connection
//                     int client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
//                     if (client_socket == -1)
//                     {
//                         perror("accept");
//                         continue;
//                     }
//                     setNonBlocking(client_socket);
//                     struct kevent ev;
//                     EV_SET(&ev, client_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
//                     if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1)
//                     {
//                         perror("kevent: client_socket");
//                         close(client_socket);
//                     }
//                 }
//                 else
//                 {
//                     // Existing connection - handle data
//                     handleClient(events[i].ident);
//                 }
//             }
//         }
//     }
//     void handleClient(int client_socket)
//     {
//         char buffer[BUFFER_SIZE];
//         std::string requestData;
//         ssize_t bytes_read;

//         while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE)) > 0)
//         {
//             requestData.append(buffer, bytes_read);
//         }

//         if (bytes_read == -1 && errno != EAGAIN)
//         {
//             perror("read");
//             close(client_socket);
//             return;
//         }

//         if (!requestData.empty())
//         {
//             try
//             {
//                 // Parse the HTTP request
//                 RequestParser parser(requestData);
//                 HttpRequest request = parser.parse();

//                 // Handle the request and get the response
//                 ResponseHandler handler(request, client_socket);
//                 HttpResponse response = handler.handleRequest();

//                 // Prepare the response headers
//                 std::string responseStr = "HTTP/1.1 " + std::to_string(response.statusCode) + " " + response.statusMessage + "\r\n";
//                 for (std::map<std::string, std::string>::const_iterator it = response.headers.begin(); it != response.headers.end(); ++it)
//                 {
//                     responseStr += it->first + ": " + it->second + "\r\n";
//                 }
//                 responseStr += "\r\n";

//                 // Send headers
//                 send(client_socket, responseStr.c_str(), responseStr.length(), 0);

//                 // If the response is serving a file, send the file content
//                 if (response.isFile)
//                 {
//                     int file_fd = open(response.filePath.c_str(), O_RDONLY);
//                     if (file_fd != -1)
//                     {
//                         off_t offset = 0;
//                         off_t len = fileSize(response.filePath); // Get the file size

//                         while (len > 0)
//                         {
//                             if (sendfile(file_fd, client_socket, offset, &len, NULL, 0) == -1)
//                             {
//                                 perror("sendfile");
//                                 break;
//                             }
//                         }
//                         close(file_fd);
//                     }
//                 }
//                 else
//                 {
//                     // For non-file responses, send the body content directly
//                     send(client_socket, response.body.c_str(), response.body.length(), 0);
//                 }
//             }
//             catch (const std::exception &e)
//             {
//                 std::cerr << "Error processing request: " << e.what() << std::endl;
//                 std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n\r\nAn error occurred while processing your request.";
//                 send(client_socket, errorResponse.c_str(), errorResponse.length(), 0);
//             }
//         }

//         // Remove client from kqueue and close the socket
//         struct kevent ev;
//         EV_SET(&ev, client_socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
//         kevent(kq, &ev, 1, NULL, 0, NULL);
//         close(client_socket);
//     }
//     ~WebServer()
//     {
//         close(server_fd);
//         close(kq);
//     }
// };

// void signalHandler(int signal)
// {
//     (void)signal;
//     std::cout << "Shutting down server..." << std::endl;
//     exit(0);
// }

// int main(int argc, char const *argv[])
// {
//     signal(SIGINT, signalHandler); // Handle Ctrl+C interrupt signal

//     if (argc != 2)
//     {
//         std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
//         return 1;
//     }

//     try
//     {
//         // Parse the configuration file
//         Lexer lexer(argv[1]);
//         Parser parser(lexer.Tokenize());
//         std::vector<ServerBlock> serverBlocks = parser.Parse();

//         // print all server blocks' ports
//         for (size_t i = 0; i < serverBlocks.size(); i++)
//         {
//             ServerBlock serverBlock = serverBlocks[i];
//             int port = serverBlock.directives["listen"].empty() ? 8080 : std::stoi(serverBlock.directives["listen"]);
//             std::cout << "detected server block with port: " << port << std::endl;
//         }

//         // for each server block, create a web server
//         for (size_t i = 0; i < serverBlocks.size(); i++)
//         {
//             ServerBlock serverBlock = serverBlocks[i];
//             int port = serverBlock.directives["listen"].empty() ? 8080 : std::stoi(serverBlock.directives["listen"]);
//             WebServer server(port);
//             server.run();
//         }
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Error: " << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }

// main.cpp

#include "lexer.hpp"
#include "parser.hpp"
#include "request_parser.hpp"
#include "response_handler.hpp"
#include "server_block.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>

// Function to set a socket to non-blocking mode
bool setNonBlocking(int sockfd)
{
    int flags;
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
        return false;
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1)
        return false;
    return true;
}

// Global SIGCHLD handler to reap child processes
extern "C" void sigchld_handler(int sig)
{
    (void)sig; // Unused parameter
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}
std::map<int, CGIProcess> cgiProcesses; 

// Function to register the SIGCHLD handler
void registerSigchldHandler()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

// Function to create and bind a listening socket
int createListeningSocket(int port)
{
    int listenSock;
    struct sockaddr_in serverAddr;

    listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock < 0)
    {
        perror("socket");
        return -1;
    }

    // Allow address reuse
    int opt = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(listenSock);
        return -1;
    }

    // Bind to the specified port on all interfaces
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    serverAddr.sin_port = htons(port);

    if (bind(listenSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind");
        close(listenSock);
        return -1;
    }

    // Start listening
    if (listen(listenSock, SOMAXCONN) < 0)
    {
        perror("listen");
        close(listenSock);
        return -1;
    }

    // Set socket to non-blocking mode
    if (!setNonBlocking(listenSock))
    {
        perror("fcntl");
        close(listenSock);
        return -1;
    }

    std::cout << "Listening on port " << port << std::endl;

    return listenSock;
}

// Function to accept a new client connection
int acceptNewClient(int listenSock)
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientSock = accept(listenSock, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientSock < 0)
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            perror("accept");
        }
        return -1;
    }

    // Set client socket to non-blocking
    if (!setNonBlocking(clientSock))
    {
        perror("fcntl");
        close(clientSock);
        return -1;
    }

    std::cout << "Accepted connection from " << inet_ntoa(clientAddr.sin_addr)
              << ":" << ntohs(clientAddr.sin_port) << " on socket " << clientSock << std::endl;

    return clientSock;
}

// Function to read the entire content of a file into a string
std::string readFileToString(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file)
    {
        throw std::runtime_error("Failed to open config file: " + filePath);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file.conf>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string configFile = argv[1];

    try
    {
        // Step 2: Tokenize the configuration file using Lexer
        Lexer lexer(configFile);
        std::vector<Token> tokens = lexer.tokenize();

        // Step 3: Parse the tokens into ServerBlocks using Parser
        Parser parser(tokens);
        std::vector<ServerBlock> serverBlocks = parser.Parse();

        if (serverBlocks.empty())
        {
            std::cerr << "No server blocks found in configuration file." << std::endl;
            return EXIT_FAILURE;
        }

        // Step 4: Create listening sockets for each ServerBlock
        std::vector<int> listenSockets;
        for (size_t i = 0; i < serverBlocks.size(); ++i)
        {
            std::map<std::string, std::string>::iterator it = serverBlocks[i].directives.find("listen");
            if (it == serverBlocks[i].directives.end())
            {
                std::cerr << "Server block " << i + 1 << " missing 'listen' directive." << std::endl;
                continue;
            }

            // Parse the listen directive (assuming it's a port number)
            int port;
            try
            {
                port = atoi(it->second.c_str());
                if (port <= 0 || port > 65535)
                {
                    throw std::out_of_range("Invalid port number.");
                }
            }
            catch (...)
            {
                std::cerr << "Invalid 'listen' directive in server block " << i + 1 << ": " << it->second << std::endl;
                continue;
            }

            int listenSock = createListeningSocket(port);
            if (listenSock >= 0)
            {
                listenSockets.push_back(listenSock);
            }
            else
            {
                std::cerr << "Failed to create listening socket for port " << port << std::endl;
            }
        }

        if (listenSockets.empty())
        {
            std::cerr << "No valid listening sockets created. Exiting." << std::endl;
            return EXIT_FAILURE;
        }

        // Step 5: Register SIGCHLD handler to reap child processes
        registerSigchldHandler();

        // Step 6: Initialize master and temporary sets for select()
        fd_set masterSet, readSet;
        FD_ZERO(&masterSet);
        int maxFd = -1;

        // Add listening sockets to the master set
        for (size_t i = 0; i < listenSockets.size(); ++i)
        {
            FD_SET(listenSockets[i], &masterSet);
            if (listenSockets[i] > maxFd)
            {
                maxFd = listenSockets[i];
            }
        }
        // Step 7: Main server loop
        while (true)
        {
            readSet = masterSet; // Copy master set to read set

            // Set a timeout for select (optional, here it's set to 5 seconds)
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            int activity = select(maxFd + 1, &readSet, NULL, NULL, &timeout);
            if (activity < 0)
            {
                if (errno == EINTR)
                {
                    continue; // Interrupted by signal, restart loop
                }
                perror("select");
                break;
            }

            if (activity == 0)
            {
                // Timeout occurred, no activity
                // Handle CGI timeouts
                time_t currentTime = time(NULL);
                for (std::map<int, CGIProcess>::iterator it = cgiProcesses.begin(); it != cgiProcesses.end();)
                {
                    if (difftime(currentTime, it->second.startTime) > 20)
                    { // 10 seconds timeout
                        // CGI script timed out
                        kill(it->second.pid, SIGKILL);
                        waitpid(it->second.pid, NULL, 0);

                        // Send 504 Gateway Timeout response to client
                        std::string timeoutResponse = "HTTP/1.1 504 Gateway Timeout\r\n"
                                                      "Content-Length: 0\r\n\r\n";
                        send(it->second.clientSock, timeoutResponse.c_str(), timeoutResponse.size(), 0);
                        close(it->second.clientSock);
                        FD_CLR(it->second.clientSock, &masterSet);

                        // Close pipe and remove from master set
                        close(it->second.pipeFd);
                        FD_CLR(it->second.pipeFd, &masterSet);

                        // Remove from the map
                        cgiProcesses.erase(it++);
                    }
                    else
                    {
                        ++it;
                    }
                }
                continue;
            }

            // Iterate through all possible file descriptors to check for data
            for (int sock = 0; sock <= maxFd; ++sock)
            {
                if (FD_ISSET(sock, &readSet))
                {
                    // Check if it's a listening socket
                    bool isListening = false;
                    for (size_t j = 0; j < listenSockets.size(); ++j)
                    {
                        if (sock == listenSockets[j])
                        {
                            isListening = true;
                            break;
                        }
                    }
                    if (isListening)
                    {
                        // Accept new client
                        int clientSock = acceptNewClient(sock);
                        if (clientSock >= 0)
                        {
                            FD_SET(clientSock, &masterSet);
                            if (clientSock > maxFd)
                            {
                                maxFd = clientSock;
                            }
                        }
                        continue;
                    }

                    // Check if it's a CGI pipe
                    std::map<int, CGIProcess>::iterator cgiIt = cgiProcesses.find(sock);
                    if (cgiIt != cgiProcesses.end())
                    {
                        // Read from the CGI pipe
                        char buffer[4096];
                        ssize_t bytesRead = read(sock, buffer, sizeof(buffer));
                        if (bytesRead > 0)
                        {
                            cgiIt->second.buffer.append(buffer, bytesRead);
                        }
                        else if (bytesRead == 0)
                        {
                            // End of output from CGI script
                            // Send the accumulated response to the client
                            // Parse the CGI output
                            std::string cgiOutput = cgiIt->second.buffer;

                            // Construct the HTTP response
                            size_t headerEnd = cgiOutput.find("\r\n\r\n");
                            if (headerEnd == std::string::npos)
                            {
                                headerEnd = cgiOutput.find("\n\n");
                            }
                            if (headerEnd != std::string::npos)
                            {
                                std::string headers = cgiOutput.substr(0, headerEnd);
                                std::string body = cgiOutput.substr(headerEnd + ((headerEnd == cgiOutput.find("\r\n\r\n")) ? 4 : 2));

                                // Build the HTTP response
                                std::ostringstream responseStream;
                                responseStream << "HTTP/1.1 200 OK\r\n";
                                responseStream << headers << "\r\n\r\n";
                                responseStream << body;

                                std::string responseStr = responseStream.str();

                                // Send the response
                                send(cgiIt->second.clientSock, responseStr.c_str(), responseStr.size(), 0);
                            }
                            else
                            {
                                // Invalid CGI output
                                std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                                            "Content-Length: 0\r\n\r\n";
                                send(cgiIt->second.clientSock, errorResponse.c_str(), errorResponse.size(), 0);
                            }

                            // Close client socket
                            close(cgiIt->second.clientSock);
                            FD_CLR(cgiIt->second.clientSock, &masterSet);

                            // Close pipe and remove from master set
                            close(cgiIt->second.pipeFd);
                            FD_CLR(cgiIt->second.pipeFd, &masterSet);

                            // Wait for the child process
                            waitpid(cgiIt->second.pid, NULL, WNOHANG);

                            // Remove from the map
                            cgiProcesses.erase(cgiIt);
                        }
                        else
                        {
                            if (errno != EAGAIN && errno != EWOULDBLOCK)
                            {
                                perror("read from CGI pipe");
                            }
                        }
                        continue;
                    }

                    // It's a client socket
                    // Read data from client socket
                    char buffer[4096];
                    ssize_t bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                    if (bytesRead < 0)
                    {
                        if (errno != EWOULDBLOCK && errno != EAGAIN)
                        {
                            perror("recv");
                            close(sock);
                            FD_CLR(sock, &masterSet);
                        }
                        continue;
                    }
                    else if (bytesRead == 0)
                    {
                        // Connection closed by client
                        std::cout << "Connection closed on socket " << sock << std::endl;
                        close(sock);
                        FD_CLR(sock, &masterSet);
                        continue;
                    }

                    // Accumulate the request data
                    std::string requestData(buffer, bytesRead);

                    // Parse the HTTP request
                    RequestParser requestParser(requestData);
                    HttpRequest request;
                    try
                    {
                        request = requestParser.parse();
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error parsing request: " << e.what() << std::endl;
                        // Send 400 Bad Request
                        std::string badRequest = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                        send(sock, badRequest.c_str(), badRequest.size(), 0);
                        close(sock);
                        FD_CLR(sock, &masterSet);
                        continue;
                    }

                    // Handle the request using ResponseHandler
                    ResponseHandler handler(request, sock);
                    HttpResponse response = handler.handleRequest();

                    // If the response status code is 0, it means the response will be sent later (for CGI)
                    if (response.statusCode == 0)
                    {
                        // The CGI process has been initiated
                        // Add the pipe file descriptor to the master set
                        int pipeFd = cgiProcesses.rbegin()->first; // Get the last added pipeFd
                        FD_SET(pipeFd, &masterSet);
                        if (pipeFd > maxFd)
                        {
                            maxFd = pipeFd;
                        }
                        continue;
                    }

                    // Construct the HTTP response
                    std::ostringstream responseStream;
                    responseStream << "HTTP/1.1 " << response.statusCode << " "
                                   << response.statusMessage << "\r\n";
                    for (std::map<std::string, std::string>::iterator it = response.headers.begin();
                         it != response.headers.end(); ++it)
                    {
                        responseStream << it->first << ": " << it->second << "\r\n";
                    }
                    responseStream << "\r\n"
                                   << response.body;

                    std::string responseStr = responseStream.str();

                    // Send the response
                    ssize_t bytesSent = send(sock, responseStr.c_str(), responseStr.size(), 0);
                    if (bytesSent < 0)
                    {
                        perror("send");
                    }

                    // Close the connection after sending the response
                    close(sock);
                    FD_CLR(sock, &masterSet);
                }
            }
        }
        // Close all listening sockets before exiting
        for (size_t i = 0; i < listenSockets.size(); ++i)
        {
            close(listenSockets[i]);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error while running server: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
