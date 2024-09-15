/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmomeni <mmomeni@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:15:49 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/15 19:22:37 by mmomeni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response_handler.hpp"
#include <sstream>
#include <dirent.h>
#include <sys/stat.h> // For 'struct stat' and related functions
#include <fstream>    // For file handling
#include <fcntl.h>    // For open()
#include <unistd.h>   // For pread(), fork(), execv(), dup2(), etc.
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <map>
#include <iostream>
#include <vector>

// Global SIGCHLD handler to reap child processes (it is defiend in main.cpp)
extern "C" void sigchld_handler(int sig);

// Register the SIGCHLD handler (to be called once in main server)
void register_sigchld_handler()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

// Function to get a partial content of a file
std::string getPartialFileContent(const std::string &filePath, off_t offset, size_t length)
{
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        throw std::runtime_error("Error opening file: " + filePath);
    }

    char *buffer = new char[length];
    ssize_t bytesRead = pread(fd, buffer, length, offset);

    if (bytesRead == -1)
    {
        close(fd);
        delete[] buffer;
        throw std::runtime_error("Error reading file: " + filePath);
    }

    std::string content(buffer, bytesRead);

    delete[] buffer;
    close(fd);

    return content;
}

// Function to get the size of a file
off_t fileSize(const std::string &filePath)
{
    struct stat stat_buf;
    if (stat(filePath.c_str(), &stat_buf) != 0)
    {
        throw std::runtime_error("Error getting file size: " + filePath);
    }
    return stat_buf.st_size;
}

extern std::map<int, CGIProcess> cgiProcesses; // Declare the map from main.cpp

ResponseHandler::ResponseHandler(const HttpRequest &request, int clientSocket)
    : _request(request), _clientSocket(clientSocket)
{
}

std::string urlDecode(const std::string &s)
{
    std::string result;
    for (size_t i = 0; i < s.length(); ++i)
    {
        if (s[i] == '+')
        {
            result += ' ';
        }
        else if (s[i] == '%' && i + 2 < s.length() && isxdigit(s[i + 1]) && isxdigit(s[i + 2]))
        {
            int value;
            std::istringstream(s.substr(i + 1, 2)) >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        }
        else
        {
            result += s[i];
        }
    }
    return result;
}

void ResponseHandler::parseFormData(const std::string &formData, std::map<std::string, std::string> &fields)
{
    std::istringstream iss(formData);
    std::string pair;

    while (std::getline(iss, pair, '&'))
    {
        size_t pos = pair.find('=');
        if (pos != std::string::npos)
        {
            std::string key = urlDecode(pair.substr(0, pos));
            std::string value = urlDecode(pair.substr(pos + 1));
            fields[key] = value;
        }
    }
}

bool ResponseHandler::isFileExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool ResponseHandler::isDirectory(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

std::string ResponseHandler::getFileContent(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::string ResponseHandler::getContentType(const std::string &path)
{
    std::string extension = getFileExtension(path);
    if (extension == "html" || extension == "htm")
        return "text/html";
    else if (extension == "css")
        return "text/css";
    else if (extension == "js")
        return "application/javascript";
    else if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    else if (extension == "png")
        return "image/png";
    else if (extension == "gif")
        return "image/gif";
    else if (extension == "svg")
        return "image/svg+xml";
    else if (extension == "ico")
        return "image/x-icon";
    else
        return "application/octet-stream";
}

std::string ResponseHandler::getFileExtension(const std::string &path)
{
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos)
        return "";
    return path.substr(pos + 1);
}

std::string ResponseHandler::generateDirectoryListing(const std::string &path)
{
    DIR *dir;
    struct dirent *ent;
    std::string content = "<html><head><title>Directory listing</title></head><body><h1>Directory listing</h1><ul>";

    if ((dir = opendir(path.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            std::string name = ent->d_name;
            if (name != "." && name != "..")
            {
                content += "<li><a href=\"" + name + "\">" + name + "</a></li>";
            }
        }
        closedir(dir);
    }
    else
    {
        throw std::runtime_error("Could not open directory");
    }

    content += "</ul></body></html>";
    return content;
}

HttpResponse ResponseHandler::handleRequest()
{
    switch (_request.method)
    {
    case GET:
        handleGET();
        break;
    case POST:
        handlePOST();
        break;
    case DELETE:
        handleDELETE();
        break;
    default:
        _response.statusCode = 405;
        _response.statusMessage = "Method Not Allowed";
        _response.body = "Unsupported HTTP method.";
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = "24";
        break;
    }
    return _response;
}

void ResponseHandler::handleGET()
{
    std::string requestedPath = "." + _request.url; // Prepend '.' to make it relative to current directory

    // Remove query string if present
    size_t queryPos = requestedPath.find('?');
    std::string queryString = "";
    if (queryPos != std::string::npos)
    {
        queryString = requestedPath.substr(queryPos + 1);
        requestedPath = requestedPath.substr(0, queryPos);
    }

    // Check if the requested file is a CGI script
    if (getFileExtension(requestedPath) == "py")
    {
        executeCGI(requestedPath, queryString);
        return;
    }

    if (isDirectory(requestedPath))
    {
        // Check for index.html in directory
        std::string indexPath = requestedPath + "/index.html";
        if (isFileExists(indexPath))
        {
            requestedPath = indexPath;
        }
        else
        {
            // Generate directory listing
            try
            {
                std::string content = generateDirectoryListing(requestedPath);
                _response.statusCode = 200;
                _response.statusMessage = "OK";
                _response.body = content;
                _response.headers["Content-Type"] = "text/html";
                std::ostringstream oss;
                oss << content.size();
                _response.headers["Content-Length"] = oss.str();
                return;
            }
            catch (const std::exception &e)
            {
                _response.statusCode = 500;
                _response.statusMessage = "Internal Server Error";
                _response.body = "Error generating directory listing: " + std::string(e.what());
                _response.headers["Content-Type"] = "text/plain";
                std::ostringstream oss;
                oss << _response.body.size();
                _response.headers["Content-Length"] = oss.str();
                return;
            }
        }
    }

    if (isFileExists(requestedPath))
    {
        try
        {
            std::string contentType = getContentType(requestedPath);

            std::map<std::string, std::string>::const_iterator it = _request.headers.find("Range");

            if (it != _request.headers.end())
            {
                std::string rangeHeader = it->second;

                // Ensure the Range header starts with "bytes="
                if (rangeHeader.compare(0, 6, "bytes=") == 0)
                {
                    // Parse the Range header
                    size_t dashPos = rangeHeader.find('-');
                    std::string startStr = rangeHeader.substr(6, dashPos - 6);                                  // Get the start value
                    std::string endStr = (dashPos == std::string::npos) ? "" : rangeHeader.substr(dashPos + 1); // Get the end value, if present

                    size_t start = startStr.empty() ? 0 : atoi(startStr.c_str()); // If no start, assume 0
                    size_t filesize = fileSize(requestedPath);
                    size_t end = endStr.empty() ? filesize - 1 : atoi(endStr.c_str()); // If no end, assume the end of file

                    if (end >= filesize)
                    {
                        end = filesize - 1;
                    }

                    size_t contentLength = end - start + 1;

                    std::string content = getPartialFileContent(requestedPath, start, contentLength);

                    _response.statusCode = 206; // Partial Content
                    _response.statusMessage = "Partial Content";
                    _response.body = content;
                    _response.headers["Content-Type"] = contentType;

                    std::ostringstream ossLength;
                    ossLength << contentLength;
                    _response.headers["Content-Length"] = ossLength.str();

                    std::ostringstream ossRange;
                    ossRange << "bytes " << start << "-" << end << "/" << filesize;
                    _response.headers["Content-Range"] = ossRange.str();
                    _response.headers["Accept-Ranges"] = "bytes";
                }
            }

            else
            {
                // Serve the entire file if no range is specified
                std::string content = getFileContent(requestedPath);

                _response.statusCode = 200;
                _response.statusMessage = "OK";
                _response.body = content;
                _response.headers["Content-Type"] = contentType;

                std::ostringstream oss;
                oss << content.size();
                _response.headers["Content-Length"] = oss.str();
                _response.headers["Accept-Ranges"] = "bytes";
            }
        }
        catch (const std::exception &e)
        {
            _response.statusCode = 500;
            _response.statusMessage = "Internal Server Error";
            _response.body = "Error reading file: " + std::string(e.what());
            _response.headers["Content-Type"] = "text/plain";

            std::ostringstream oss;
            oss << _response.body.size();
            _response.headers["Content-Length"] = oss.str();
        }
    }
    else
    {
        _response.statusCode = 404;
        _response.statusMessage = "Not Found";
        _response.body = "The requested URL was not found on this server.";
        _response.headers["Content-Type"] = "text/plain";

        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
    }
}

void ResponseHandler::handlePOST()
{
    std::string requestedPath = "." + _request.url;

    if (getFileExtension(requestedPath) == "py")
    {
        executeCGI(requestedPath, _request.body);
        return;
    } 
    

    std::map<std::string, std::string> formData;
    parseFormData(_request.body, formData);

    if (formData.empty())
    {
        _response.statusCode = 400;
        _response.statusMessage = "Bad Request";
        _response.body = "No form data submitted.";
        _response.headers["Content-Type"] = "text/plain";

        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
        return;
    }
    else
    {
        std::string _body;
        for (std::map<std::string, std::string>::const_iterator it = formData.begin(); it != formData.end(); ++it)
        {
            _dataStore[it->first] = it->second;
            _body += it->first + ": " + it->second + "\n";
        }

        _response.statusCode = 200;
        _response.statusMessage = "OK";
        _response.body = "Form data submitted successfully.\n" + _body;
        _response.headers["Content-Type"] = "text/plain";

        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
    }
}

void ResponseHandler::handleDELETE()
{
    std::string filePath = "." + _request.url;

    if (isFileExists(filePath))
    {
        if (remove(filePath.c_str()) != 0)
        {
            _response.statusCode = 500;
            _response.statusMessage = "Internal Server Error";
            _response.body = "Failed to delete file: " + filePath;
            _response.headers["Content-Type"] = "text/plain";

            std::ostringstream oss;
            oss << _response.body.size();
            _response.headers["Content-Length"] = oss.str();
        }
        else
        {
            _response.statusCode = 200;
            _response.statusMessage = "OK";
            _response.body = "File deleted: " + filePath;
            _response.headers["Content-Type"] = "text/plain";

            std::ostringstream oss;
            oss << _response.body.size();
            _response.headers["Content-Length"] = oss.str();
        }
    }
    else
    {
        _response.statusCode = 404;
        _response.statusMessage = "Not Found";
        _response.body = "The requested URL was not found on this server.";
        _response.headers["Content-Type"] = "text/plain";

        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
    }
}

void ResponseHandler::executeCGI(const std::string &scriptPath, const std::string &queryString)
{

    struct stat buffer;
    if (stat(scriptPath.c_str(), &buffer) != 0 || !(buffer.st_mode & S_IXUSR))
    {
        std::cout << "Script not found: " << scriptPath << std::endl;
        _response.statusCode = 404;
        _response.statusMessage = "Not Found";
        _response.body = "The requested script was not found on this server.";
        _response.headers["Content-Type"] = "text/plain";
        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
        return;
    }

    std::map<std::string, std::string> envVars;
    envVars["GATEWAY_INTERFACE"] = "CGI/1.1";
    envVars["REQUEST_METHOD"] = (_request.method == POST) ? "POST" : "GET";
    envVars["QUERY_STRING"] = queryString;
    envVars["CONTENT_LENGTH"] = (_request.headers.find("Content-Length") != _request.headers.end()) ? _request.headers.at("Content-Length") : "0";
    envVars["CONTENT_TYPE"] = (_request.headers.find("Content-Type") != _request.headers.end()) ? _request.headers.at("Content-Type") : "";
    envVars["SCRIPT_NAME"] = _request.url;
    envVars["SERVER_PROTOCOL"] = _request.version;

    std::vector<std::string> envStrings;
    envStrings.reserve(envVars.size() + 1);
    for (std::map<std::string, std::string>::const_iterator it = envVars.begin(); it != envVars.end(); ++it)
        envStrings.push_back(it->first + "=" + it->second);
    std::vector<char *> envp(envStrings.size() + 1);
    for (size_t i = 0; i < envStrings.size(); ++i)
        envp[i] = const_cast<char *>(envStrings[i].c_str());
    envp[envStrings.size()] = NULL;

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        std::cout << "Error creating pipe." << std::endl;
        _response.statusCode = 500;
        _response.statusMessage = "Internal Server Error";
        _response.body = "Error creating pipe.";
        _response.headers["Content-Type"] = "text/plain";
        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cout << "Error forking process." << std::endl;
        _response.statusCode = 500;
        _response.statusMessage = "Internal Server Error";
        _response.body = "Error forking process.";
        _response.headers["Content-Type"] = "text/plain";
        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0)
    {
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        char *argv[] = {const_cast<char *>("python3"), const_cast<char *>(scriptPath.c_str()), NULL};
        execve("/usr/local/bin/python3", argv, envp.data());

        std::cout << "execve failed" << std::endl;
        perror("execve failed");
        exit(1);
    }
    else
    {
        close(pipefd[1]);

        int flags = fcntl(pipefd[0], F_GETFL, 0);
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

        CGIProcess cgiProc;
        cgiProc.pid = pid;
        cgiProc.pipeFd = pipefd[0];
        cgiProc.clientSock = _clientSocket;
        cgiProc.startTime = time(NULL);

        cgiProcesses[pipefd[0]] = cgiProc;

        _response.statusCode = 0; // the status code will be set later by the CGI program
    }
}

void ResponseHandler::handleFileUpload(const std::string &uploadDir, const std::string &fileData, const std::string &fileName)
{
    std::string filePath = uploadDir + "/" + fileName;

    std::ofstream outFile(filePath.c_str(), std::ios::binary);
    if (!outFile)
    {
        _response.statusCode = 500;
        _response.statusMessage = "Internal Server Error";
        _response.body = "Failed to open file for writing: " + filePath;
        _response.headers["Content-Type"] = "text/plain";

        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
        return;
    }

    outFile.write(fileData.c_str(), fileData.size());
    outFile.close();

    _response.statusCode = 200;
    _response.statusMessage = "OK";
    _response.body = "File uploaded successfully.";
    _response.headers["Content-Type"] = "text/plain";

    std::ostringstream oss;
    oss << _response.body.size();
    _response.headers["Content-Length"] = oss.str();
}
