/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/25 19:54:14 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 20:01:03 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */



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


extern "C" void sigchld_handler(int sig)
{
    (void)sig; 
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}
std::map<int, CGIProcess> cgiProcesses;


void registerSigchldHandler()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}


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

    
    int opt = 1;
    if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(listenSock);
        return -1;
    }

    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(port);

    if (bind(listenSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind");
        close(listenSock);
        return -1;
    }

    
    if (listen(listenSock, SOMAXCONN) < 0)
    {
        perror("listen");
        close(listenSock);
        return -1;
    }

    
    if (!setNonBlocking(listenSock))
    {
        perror("fcntl");
        close(listenSock);
        return -1;
    }

    std::cout << "Listening on port " << port << std::endl;

    return listenSock;
}


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


std::string readFileToString(const std::string &filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file) throw std::runtime_error("Failed to open config file: " + filePath);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}



struct ClientRequest
{
    std::string data;
    bool headersReceived;
    size_t expectedLength;
    ClientRequest() : headersReceived(false), expectedLength(0) {}
};


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
        
        Lexer lexer(configFile);
        std::vector<Token> tokens = lexer.tokenize();

        
        Parser parser(tokens);
        std::vector<ServerBlock> serverBlocks = parser.Parse();

        if (serverBlocks.empty())
        {
            std::cerr << "No server blocks found in configuration file." << std::endl;
            return EXIT_FAILURE;
        }

        
        std::vector<int> listenSockets;
        for (size_t i = 0; i < serverBlocks.size(); ++i)
        {
            std::map<std::string, std::string>::iterator it = serverBlocks[i].directives.find("listen");
            if (it == serverBlocks[i].directives.end())
            {
                std::cerr << "Server block " << i + 1 << " missing 'listen' directive." << std::endl;
                continue;
            }

            
            int port = atoi(it->second.c_str());
            if (port <= 0 || port > 65535)
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

        
        registerSigchldHandler();

        
        fd_set masterSet, readSet;
        FD_ZERO(&masterSet);
        int maxFd = -1;

        
        for (size_t i = 0; i < listenSockets.size(); ++i)
        {
            FD_SET(listenSockets[i], &masterSet);
            if (listenSockets[i] > maxFd)
            {
                maxFd = listenSockets[i];
            }
        }

        
        std::map<int, ClientRequest> clientRequests;

        
        while (true)
        {
            readSet = masterSet; 

            
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            int activity = select(maxFd + 1, &readSet, NULL, NULL, &timeout);
            if (activity < 0)
            {
                if (errno == EINTR)
                {
                    continue; 
                }
                perror("select");
                break;
            }

            if (activity == 0)
            {
                
                
                time_t currentTime = time(NULL);
                std::map<int, CGIProcess>::iterator it = cgiProcesses.begin();
                while (it != cgiProcesses.end())
                {
                    if (difftime(currentTime, it->second.startTime) > 10)
                    { 
                        
                        kill(it->second.pid, SIGKILL);
                        waitpid(it->second.pid, NULL, 0);

                        
                        std::string timeoutResponse = "HTTP/1.1 504 Gateway Timeout\r\n"
                                                      "Content-Length: 0\r\n\r\n";
                        send(it->second.clientSock, timeoutResponse.c_str(), timeoutResponse.size(), 0);
                        close(it->second.clientSock);
                        FD_CLR(it->second.clientSock, &masterSet);

                        
                        close(it->second.pipeFd);
                        FD_CLR(it->second.pipeFd, &masterSet);

                        
                        cgiProcesses.erase(it++);
                    }
                    else
                    {
                        ++it;
                    }
                }
                continue;
            }

            
            for (int sock = 0; sock <= maxFd; ++sock)
            {
                if (FD_ISSET(sock, &readSet))
                {
                    
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
                        
                        int clientSock = acceptNewClient(sock);
                        if (clientSock >= 0)
                        {
                            FD_SET(clientSock, &masterSet);
                            if (clientSock > maxFd)
                            {
                                maxFd = clientSock;
                            }
                            
                            clientRequests[clientSock] = ClientRequest();
                        }
                        continue;
                    }

                    
                    std::map<int, CGIProcess>::iterator cgiIt = cgiProcesses.find(sock);
                    if (cgiIt != cgiProcesses.end())
                    {
                        
                        char buffer[4096];
                        ssize_t bytesRead = read(sock, buffer, sizeof(buffer));
                        if (bytesRead > 0) cgiIt->second.buffer.append(buffer, bytesRead);
                        else if (bytesRead == 0)
                        {
                            
                            
                            std::string cgiOutput = cgiIt->second.buffer;

                            
                            size_t headerEnd = cgiOutput.find("\r\n\r\n");
                            if (headerEnd == std::string::npos)
                            {
                                headerEnd = cgiOutput.find("\n\n");
                            }
                            if (headerEnd != std::string::npos)
                            {
                                std::string headers = cgiOutput.substr(0, headerEnd);
                                std::string body = cgiOutput.substr(headerEnd + ((headerEnd == cgiOutput.find("\r\n\r\n")) ? 4 : 2));

                                
                                std::ostringstream responseStream;
                                responseStream << "HTTP/1.1 200 OK\r\n";
                                responseStream << headers << "\r\n\r\n";
                                responseStream << body;

                                std::string responseStr = responseStream.str();

                                
                                send(cgiIt->second.clientSock, responseStr.c_str(), responseStr.size(), 0);
                            }
                            else
                            {
                                
                                std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                                            "Content-Length: 0\r\n\r\n";
                                send(cgiIt->second.clientSock, errorResponse.c_str(), errorResponse.size(), 0);
                            }

                            
                            close(cgiIt->second.clientSock);
                            FD_CLR(cgiIt->second.clientSock, &masterSet);

                            
                            close(cgiIt->second.pipeFd);
                            FD_CLR(cgiIt->second.pipeFd, &masterSet);

                            
                            waitpid(cgiIt->second.pid, NULL, WNOHANG);

                            
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

                    
                    
                    char buffer[4096];
                    ssize_t bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                    if (bytesRead <= 0)
                    {
                        if (bytesRead == 0) std::cout << "Connection closed on socket " << sock << std::endl;
                        else
                        {
                            if (errno != EWOULDBLOCK && errno != EAGAIN)
                            {
                                perror("recv");
                            }
                        }
                        close(sock);
                        FD_CLR(sock, &masterSet);
                        clientRequests.erase(sock);
                        continue;
                    }

                    
                    clientRequests[sock].data.append(buffer, bytesRead);

                    ClientRequest &clientRequest = clientRequests[sock];

                    if (!clientRequest.headersReceived)
                    {
                        size_t headersEnd = clientRequest.data.find("\r\n\r\n");
                        if (headersEnd != std::string::npos)
                        {
                            clientRequest.headersReceived = true;
                            
                            std::string headers = clientRequest.data.substr(0, headersEnd + 4);
                            RequestParser headerParser(headers);
                            HttpRequest tempRequest;
                            try
                            {
                                tempRequest = headerParser.parseHeadersOnly();
                            }
                            catch (const std::exception &e)
                            {
                                
                                std::cerr << "Error parsing headers: " << e.what() << std::endl;
                                std::string badRequest = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                                send(sock, badRequest.c_str(), badRequest.size(), 0);
                                close(sock);
                                FD_CLR(sock, &masterSet);
                                clientRequests.erase(sock);
                                continue;
                            }
                            
                            clientRequest.expectedLength = headersEnd + 4; 
                            if (tempRequest.headers.find("Content-Length") != tempRequest.headers.end())
                            {
                                int contentLength = atoi(tempRequest.headers["Content-Length"].c_str());
                                clientRequest.expectedLength += contentLength; 
                            }
                            else if (tempRequest.headers.find("Transfer-Encoding") != tempRequest.headers.end() &&
                                     tempRequest.headers["Transfer-Encoding"] == "chunked")
                            {
                                
                                
                                std::string lengthRequired = "HTTP/1.1 411 Length Required\r\nContent-Length: 0\r\n\r\n";
                                send(sock, lengthRequired.c_str(), lengthRequired.size(), 0);
                                close(sock);
                                FD_CLR(sock, &masterSet);
                                clientRequests.erase(sock);
                                continue;
                            }
                        }
                    }

                    if (clientRequest.headersReceived)
                    {
                        if (clientRequest.data.size() >= clientRequest.expectedLength)
                        {
                            
                            std::string requestData = clientRequest.data;
                            clientRequests.erase(sock);

                            
                            std::cout << "Received " << requestData.size() << " bytes from socket " << sock << std::endl;

                            
                            RequestParser requestParser(requestData);
                            HttpRequest request;
                            try
                            {
                                request = requestParser.parse();
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << "Error parsing request: " << e.what() << std::endl;
                                
                                std::string badRequest = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                                send(sock, badRequest.c_str(), badRequest.size(), 0);
                                close(sock);
                                FD_CLR(sock, &masterSet);
                                continue;
                            }

                            
                            ResponseHandler handler(request, sock);
                            HttpResponse response = handler.handleRequest();

                            
                    if (response.statusCode == 0)
                    {
                        
                        
                        int pipeFd = cgiProcesses.rbegin()->first; 
                        FD_SET(pipeFd, &masterSet);
                        if (pipeFd > maxFd)
                        {
                            maxFd = pipeFd;
                        }
                        continue;
                    }

                            
                            std::ostringstream responseStream;
                            responseStream << "HTTP/1.1 " << response.statusCode << " "
                                           << response.statusMessage << "\r\n";
                            for (std::map<std::string, std::string>::iterator it = response.headers.begin();
                                 it != response.headers.end(); ++it) responseStream << it->first << ": " << it->second << "\r\n";
                            responseStream << "\r\n" << response.body;

                            std::string responseStr = responseStream.str();

                            
                            ssize_t bytesSent = send(sock, responseStr.c_str(), responseStr.size(), 0);
                            if (bytesSent < 0) perror("send");

                            
                            close(sock);
                            FD_CLR(sock, &masterSet);
                        }
                        else continue;
                    }
                }
            }
        }
        
        for (size_t i = 0; i < listenSockets.size(); ++i) close(listenSockets[i]);

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error while running server: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
