#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <sys/event.h>
#include <errno.h>
#include "lexer.hpp"
#include "parser.hpp"
#include "request_parser.hpp"
#include "response_handler.hpp"
#include <csignal>
#include <sys/stat.h>
#include <string>
#include <stdexcept>


#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

class WebServer
{
private:
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen;
    int kq;

    void setNonBlocking(int sock) {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl F_GETFL");
            exit(EXIT_FAILURE);
        }
        if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl F_SETFL O_NONBLOCK");
            exit(EXIT_FAILURE);
        }
    }

public:
    WebServer(int port)
    {
        // Create socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // Initialize address structure
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        // Bind the socket to the network address and port
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        // Start listening for connections
        if (listen(server_fd, SOMAXCONN) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        addrlen = sizeof(address);
        std::cout << "Server listening on port " << port << std::endl;

        // Create kqueue instance
        kq = kqueue();
        if (kq == -1) {
            perror("kqueue");
            exit(EXIT_FAILURE);
        }

        // Add server socket to kqueue
        struct kevent ev;
        EV_SET(&ev, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
            perror("kevent: server_fd");
            exit(EXIT_FAILURE);
        }

        setNonBlocking(server_fd);
    }

    void run()
    {
        struct kevent events[MAX_EVENTS];
        while (true)
        {
            int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
            if (nev == -1) {
                perror("kevent wait");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < nev; i++) {
                if (events[i].ident == (uintptr_t)server_fd) {
                    // New connection
                    int client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
                    if (client_socket == -1) {
                        perror("accept");
                        continue;
                    }
                    setNonBlocking(client_socket);
                    struct kevent ev;
                    EV_SET(&ev, client_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
                        perror("kevent: client_socket");
                        close(client_socket);
                    }
                } else {
                    // Existing connection - handle data
                    handleClient(events[i].ident);
                }
            }
        }
    }
  void handleClient(int client_socket)
{
    char buffer[BUFFER_SIZE];
    std::string requestData;
    ssize_t bytes_read;

    while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
        requestData.append(buffer, bytes_read);
    }

    if (bytes_read == -1 && errno != EAGAIN) {
        perror("read");
        close(client_socket);
        return;
    }

    if (!requestData.empty()) {
        try
        {
            // Parse the HTTP request
            RequestParser parser(requestData);
            HttpRequest request = parser.parse();

            // Handle the request and get the response
            ResponseHandler handler(request);
            HttpResponse response = handler.handleRequest();

            // Prepare the response headers
            std::string responseStr = "HTTP/1.1 " + std::to_string(response.statusCode) + " " + response.statusMessage + "\r\n";
            for (std::map<std::string, std::string>::const_iterator it = response.headers.begin(); it != response.headers.end(); ++it)
            {
                responseStr += it->first + ": " + it->second + "\r\n";
            }
            responseStr += "\r\n";

            // Send headers
            send(client_socket, responseStr.c_str(), responseStr.length(), 0);

            // If the response is serving a file, send the file content
            if (response.isFile) {
                int file_fd = open(response.filePath.c_str(), O_RDONLY);
                if (file_fd != -1) {
                    off_t offset = 0;
                    off_t len = fileSize(response.filePath); // Get the file size

                    while (len > 0) {
                        if (sendfile(file_fd, client_socket, offset, &len, NULL, 0) == -1) {
                            perror("sendfile");
                            break;
                        }
                    }
                    close(file_fd);
                }
            } else {
                // For non-file responses, send the body content directly
                send(client_socket, response.body.c_str(), response.body.length(), 0);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing request: " << e.what() << std::endl;
            std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n\r\nAn error occurred while processing your request.";
            send(client_socket, errorResponse.c_str(), errorResponse.length(), 0);
        }
    }

    // Remove client from kqueue and close the socket
    struct kevent ev;
    EV_SET(&ev, client_socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(kq, &ev, 1, NULL, 0, NULL);
    close(client_socket);
}
    ~WebServer()
    {
        close(server_fd);
        close(kq);
    }
};

void signalHandler(int signal)
{
    (void)signal;
    std::cout << "Shutting down server..." << std::endl;
    exit(0);
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, signalHandler);  // Handle Ctrl+C interrupt signal

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    try
    {
        // Parse the configuration file
        Lexer lexer(argv[1]);
        Parser parser(lexer.Tokenize());
        std::vector<ServerBlock> serverBlocks = parser.Parse();

        // Get the port number from the configuration
        int port = serverBlocks[0].directives["listen"].empty() ? 8080 : std::stoi(serverBlocks[0].directives["listen"]);

        // Create and run the web server
        WebServer server(port);
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
