#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>

struct ServerConfig
{
    int listenPort;
    std::string serverName;
    std::map<std::string, std::string> locationSettings;
};

class Server
{
private:
    int serverSocket;
    std::vector<ServerConfig> configs;
    void createAndBindSocket(int port);
    void listenForConnections();
    void handleClient(int clientSocket);

public:
    Server(const std::vector<ServerConfig> &serverConfigs);
    void start();
};

#endif
