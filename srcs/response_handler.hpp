#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include "request.hpp"
#include <string>
#include <map>

off_t fileSize(const std::string &filePath);

struct CGIProcess {
    pid_t pid;                 // Process ID of the CGI script
    int pipeFd;                // Read end of the pipe from the CGI script
    int clientSock;            // Client socket to send data to
    std::string buffer;        // Buffer to accumulate output
    time_t startTime;          // Timestamp when the CGI started
};


class ResponseHandler
{
    private:
        HttpResponse        _response;
        const HttpRequest   &_request;
        int                 _clientSocket; // Client socket
        std::map<std::string, std::string> _dataStore;

    public:
        // Constructor accepts clientSocket
        ResponseHandler(const HttpRequest &request, int clientSocket);
        HttpResponse handleRequest();
        void handleGET();
        void handlePOST();
        void handleDELETE();

        bool isFileExists(const std::string &path);
        std::string getFileContent(const std::string &path);
        std::string getContentType(const std::string &path);
        std::string getFileExtension(const std::string &path);
        void parseFormData(const std::string &formData, std::map<std::string, std::string> &fields);
        bool isDirectory(const std::string &path);
        std::string generateDirectoryListing(const std::string &path);
        void executeCGI(const std::string &scriptPath, const std::string &queryString);
        void handleFileUpload(const std::string &uploadDir, const std::string &fileData, const std::string &fileName);
};

#endif
