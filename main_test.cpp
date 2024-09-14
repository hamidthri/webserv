#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#define SERVER_PORT 8080
#define MAX_EVENTS 10

void execute_cgi(int client_fd, const std::string& script_path) {
    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();

    if (pid == 0) { // Child process
        close(pipefd[0]); // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        char* argv[] = {nullptr}; // No arguments
        char* envp[] = {nullptr}; // No environment variables
        execve(script_path.c_str(), argv, envp);
        exit(EXIT_FAILURE); // execve only returns on error
    } else { // Parent process
        close(pipefd[1]); // Close unused write end
        char buffer[1024];
        ssize_t nread;
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

        write(client_fd, response.c_str(), response.size());
        while ((nread = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            write(client_fd, buffer, nread);
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0); // Wait for the child to exit
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET for IPv4

if (server_fd < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
}

sockaddr_in addr = {};
addr.sin_family = AF_INET; // Make sure this matches with AF_INET
addr.sin_port = htons(SERVER_PORT);
addr.sin_addr.s_addr = INADDR_ANY; // Listen on any IP address

    int kq = kqueue();
    struct kevent ev_set;

    EV_SET(&ev_set, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &ev_set, 1, NULL, 0, NULL);

    std::cout << "Server listening on port " << SERVER_PORT << std::endl;
    while (true) {
        struct kevent ev_list[MAX_EVENTS];
        int n = kevent(kq, NULL, 0, ev_list, MAX_EVENTS, NULL);

        for (int i = 0; i < n; ++i) {
            if (ev_list[i].ident == (uint64_t)server_fd) {
                int client_fd = accept(server_fd, nullptr, nullptr);
                EV_SET(&ev_set, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &ev_set, 1, NULL, 0, NULL);
            } else {
                char buffer[1024];
                int bytes_read = read(ev_list[i].ident, buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    close(ev_list[i].ident);
                } else {
                    std::string request(buffer, bytes_read);
                    if (request.find("GET /cgi-bin/") != std::string::npos) {
                        size_t start = request.find("/cgi-bin/") + 9;
                        size_t end = request.find(" ", start);
                        std::string script_path = "./cgi-bin" + request.substr(start, end - start);
                        execute_cgi(ev_list[i].ident, script_path);
                    }
                    close(ev_list[i].ident);
                }
            }
        }
    }
    close(server_fd);
    return 0;
}
