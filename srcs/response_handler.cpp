/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:15:49 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/07 21:21:36 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response_handler.hpp"
#include <sstream>
#include <dirent.h>
#include <sys/stat.h> // Include this for 'struct stat' and related functions
#include <fstream>    // Include this for file handling
#include <fcntl.h>    // For open()
#include <unistd.h>   // For pread()
#include <stdexcept>
#include <string>
#include <sys/wait.h>

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

ResponseHandler::ResponseHandler(const HttpRequest &request) : _request(request)
{
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
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
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
        break;
        // ensure content-length is set
        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
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
                _response.headers["Content-Length"] = std::to_string(content.size());
                return;
            }
            catch (const std::exception &e)
            {
                _response.statusCode = 500;
                _response.statusMessage = "Internal Server Error";
                _response.body = "Error generating directory listing: " + std::string(e.what());
                _response.headers["Content-Type"] = "text/plain";
                _response.headers["Content-Length"] = std::to_string(_response.body.size());
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

                    size_t start = startStr.empty() ? 0 : std::stoul(startStr); // If no start, assume 0
                    size_t filesize = fileSize(requestedPath);
                    size_t end = endStr.empty() ? filesize - 1 : std::stoul(endStr); // If no end, assume the end of file

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
                    _response.headers["Content-Length"] = std::to_string(contentLength); // Use contentLength, not content.size()
                    _response.headers["Content-Range"] = "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" + std::to_string(filesize);
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
                _response.headers["Content-Length"] = std::to_string(content.size());
                _response.headers["Accept-Ranges"] = "bytes";
            }
        }
        catch (const std::exception &e)
        {
            _response.statusCode = 500;
            _response.statusMessage = "Internal Server Error";
            _response.body = "Error reading file: " + std::string(e.what());
            _response.headers["Content-Type"] = "text/plain";
            _response.headers["Content-Length"] = std::to_string(_response.body.size());
        }
    }
    else
    {
        _response.statusCode = 404;
        _response.statusMessage = "Not Found";
        _response.body = "The requested URL was not found on this server.";
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = std::to_string(_response.body.size());
    }
}

void ResponseHandler::handlePOST()
{
    std::string requestedPath = "." + _request.url;
    
    if (getFileExtension(requestedPath) == "py")
    {
        executeCGI(requestedPath, "");
        return;
    }
    
    std::map<std::string, std::string> formData;
    parseFormData(_request.body, formData);

    if (formData.empty())
    {
        _response.statusCode = 400;
        _response.statusMessage = "Bad Request";
        _response.body = "No form data submitted.";
        _response.Date = "Date: " + _request.headers.at("Date");
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = std::to_string(_response.body.size());
        return;
    }
    else
    {
        for (std::map<std::string, std::string>::const_iterator it = formData.begin(); it != formData.end(); ++it)
            _dataStore[it->first] = it->second;

        _response.statusCode = 200;
        _response.statusMessage = "OK";
        _response.body = "Form data submitted successfully.";
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = std::to_string(_response.body.size());

        // for (std::map<std::string, std::string>::const_iterator it = _dataStore.begin(); it != _dataStore.end(); ++it)
        //     std::cout << it->first << ": " << it->second << std::endl;
    }

    _response.headers["Content-Type"] = "text/plain";
    _response.headers["Content-Length"] = std::to_string(_response.body.size());
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
            _response.headers["Content-Length"] = std::to_string(_response.body.size());
        }
        else
        {
            _response.statusCode = 200;
            _response.statusMessage = "OK";
            _response.body = "File deleted: " + filePath;
            _response.headers["Content-Type"] = "text/plain";
            _response.headers["Content-Length"] = std::to_string(_response.body.size());
        }
    }
    else
    {
        _response.statusCode = 404;
        _response.statusMessage = "Not Found";
        _response.body = "The requested URL was not found on this server.";
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = std::to_string(_response.body.size());
    }
}
void ResponseHandler::executeCGI(const std::string &scriptPath, const std::string &queryString)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        _response.statusCode = 500;
        _response.statusMessage = "Internal Server Error";
        _response.body = "Error creating pipe for CGI script.";
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = std::to_string(_response.body.size());
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        _response.statusCode = 500;
        _response.statusMessage = "Internal Server Error";
        _response.body = "Error forking process for CGI script.";
        _response.headers["Content-Type"] = "text/plain";
        _response.headers["Content-Length"] = std::to_string(_response.body.size());
        return;
    }

    if (pid == 0)
    {
        // Child process
        // Set up environment variables
        setenv("REQUEST_METHOD", _request.method == GET ? "GET" : "POST", 1);
        setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
        setenv("QUERY_STRING", queryString.c_str(), 1);
        setenv("CONTENT_LENGTH", std::to_string(_request.body.length()).c_str(), 1);
        setenv("PATH_INFO", _request.url.c_str(), 1);
        
        // Set Content-Type as per your modification
        if (_request.headers.find("Content-Type") == _request.headers.end())
            setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        else
            setenv("CONTENT_TYPE", _request.headers.at("Content-Type").c_str(), 1);

        // Redirect stdout and stderr to pipe
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        
        // Change to the server's root directory
        chdir(".");  // Assuming the server is started from the correct directory
        
        // Execute the script with Python interpreter
        char *argv[] = {const_cast<char *>("python3"), const_cast<char *>(scriptPath.c_str()), nullptr};
        execv("/usr/bin/env", argv);
        
        // If execv returns, there was an error
        perror("execv failed");
        exit(1);
    }
    else
    {
        // Parent process
        close(pipefd[1]);

        // If it's a POST request, write the request body to the child's stdin
        if (_request.method == POST)
        {
            write(pipefd[1], _request.body.c_str(), _request.body.length());
            close(pipefd[1]);  // Close write end after writing
        }

        char buffer[4096];
        std::string cgiOutput;
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0)
        {
            cgiOutput.append(buffer, bytesRead);
        }
        close(pipefd[0]);
        
        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);
        
        // Parse the CGI output
        parseCGIResponse(cgiOutput);
        
        if (status != 0)
        {
            _response.statusCode = 500;
            _response.statusMessage = "Internal Server Error";
            _response.body = "Error executing CGI script. Output: " + cgiOutput;
            _response.headers["Content-Type"] = "text/plain";
            _response.headers["Content-Length"] = std::to_string(_response.body.size());
        }
    }
}
void ResponseHandler::parseCGIResponse(const std::string &cgiOutput)
{
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = cgiOutput.find("\n\n");
        if (headerEnd == std::string::npos) {
            _response.statusCode = 500;
            _response.statusMessage = "Internal Server Error";
            _response.body = "Invalid CGI response. response: " + cgiOutput;
            _response.headers["Content-Type"] = "text/plain";
            _response.headers["Content-Length"] = std::to_string(_response.body.size());
            return;
        }
    }
    
    std::string headerPart = cgiOutput.substr(0, headerEnd);
    std::string bodyPart = cgiOutput.substr(headerEnd + (headerEnd == cgiOutput.find("\r\n\r\n") ? 4 : 2));
    
    // parse headers
    std::istringstream headerStream(headerPart);
    std::string headerLine;

    while (std::getline(headerStream, headerLine) && !headerLine.empty())
    {
        // Remove any trailing \r
        if (!headerLine.empty() && headerLine[headerLine.length()-1] == '\r')
            headerLine.erase(headerLine.length()-1);

        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            // Trim leading whitespace from value
            value.erase(0, value.find_first_not_of(" \t"));
            _response.headers[key] = value;
        }
    }

    _response.body = bodyPart;
    if (_response.headers.find("Status") != _response.headers.end())
    {
        std::string statusLine = _response.headers["Status"];
        size_t spacePos = statusLine.find(' ');
        if (spacePos != std::string::npos)
        {
            _response.statusCode = std::stoi(statusLine.substr(0, spacePos));
            _response.statusMessage = statusLine.substr(spacePos + 1);
        }
        else
        {
            _response.statusCode = std::stoi(statusLine);
            _response.statusMessage = "OK";
        }
        _response.headers.erase("Status");
    }
    else
    {
        _response.statusCode = 200;
        _response.statusMessage = "OK";
    }
    
    if (_response.headers.find("Content-Type") == _response.headers.end())
    {
        _response.headers["Content-Type"] = "text/plain";
    }

    _response.headers["Content-Length"] = std::to_string(_response.body.size());
}

void ResponseHandler::handleFileUpload(const std::string &uploadDir, const std::string &fileData, const std::string &fileName) {
    std::string filePath = uploadDir + "/" + fileName;

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) {
        _response.statusCode = 500;
        _response.statusMessage = "Internal Server Error";
        _response.body = "Failed to save uploaded file.";
        return;
    }

    outFile.write(fileData.c_str(), fileData.size());
    outFile.close();

    _response.statusCode = 200;
    _response.statusMessage = "OK";
    _response.body = "File uploaded successfully.";
    _response.headers["Content-Type"] = "text/plain";
    _response.headers["Content-Length"] = std::to_string(_response.body.size());
}
