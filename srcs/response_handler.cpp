/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:15:49 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 20:41:04 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response_handler.hpp"
#include "request_parser.hpp"

extern "C" void sigchld_handler(int sig);

void register_sigchld_handler()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

bool ResponseHandler::doesFileExist(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}


std::string getPartialFileContent(const std::string &filePath, off_t offset, size_t length)
{
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1) throw std::runtime_error("Error opening file: " + filePath);

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

    return (content);
}


off_t fileSize(const std::string &filePath)
{
    struct stat stat_buf;
    if (stat(filePath.c_str(), &stat_buf) != 0) throw std::runtime_error("Error getting file size: " + filePath);
    return (stat_buf.st_size);
}

extern std::map<int, CGIProcess> cgiProcesses; 

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
            result += s[i];
    }
    return (result);
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
    return (std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
}

std::string ResponseHandler::getContentType(const std::string &path)
{
    std::string extension = getFileExtension(path);
    if (extension == "html" || extension == "htm")
        return ("text/html");
    else if (extension == "css")
        return ("text/css");
    else if (extension == "js")
        return ("application/javascript");
    else if (extension == "jpg" || extension == "jpeg")
        return ("image/jpeg");
    else if (extension == "png")
        return ("image/png");
    else if (extension == "gif")
        return ("image/gif");
    else if (extension == "svg")
        return ("image/svg+xml");
    else if (extension == "ico")
        return ("image/x-icon");
    else
        return ("application/octet-stream");
}

std::string ResponseHandler::getFileExtension(const std::string &path)
{
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos)
        return ("");
    return (path.substr(pos + 1));
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
                content += "<li><a href=\"" + name + "\">" + name + "</a></li>";
        }
        closedir(dir);
    }
    else throw std::runtime_error("Could not open directory");

    content += "</ul></body></html>";
    return (content);
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
    std::string requestedPath = "." + _request.url; 

    size_t queryPos = requestedPath.find('?');
    std::string queryString = "";
    if (queryPos != std::string::npos)
    {
        queryString = requestedPath.substr(queryPos + 1);
        requestedPath = requestedPath.substr(0, queryPos);
    }
    if (getFileExtension(requestedPath) == "py")
    {
        std::cout << "Executing CGI script: " << requestedPath << std::endl;
        executeCGI(requestedPath, queryString);
        return ;
    }

    if (isDirectory(requestedPath))
    {
        std::string indexPath = requestedPath + "/index.html";
        if (doesFileExist(indexPath)) requestedPath = indexPath;
        else
        {
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
                return ;
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
                return ;
            }
        }
    }

    if (doesFileExist(requestedPath))
    {
        try
        {
            std::string contentType = getContentType(requestedPath);
            std::map<std::string, std::string>::const_iterator it = _request.headers.find("Range");

            if (it != _request.headers.end())
            {
                std::string rangeHeader = it->second;

                
                if (rangeHeader.compare(0, 6, "bytes=") == 0)
                {
                    
                    size_t dashPos = rangeHeader.find('-');
                    std::string startStr = rangeHeader.substr(6, dashPos - 6);                                  
                    std::string endStr = (dashPos == std::string::npos) ? "" : rangeHeader.substr(dashPos + 1); 

                    size_t start = startStr.empty() ? 0 : atoi(startStr.c_str()); 
                    size_t filesize = fileSize(requestedPath);
                    size_t end = endStr.empty() ? filesize - 1 : atoi(endStr.c_str()); 

                    if (end >= filesize)
                        end = filesize - 1;

                    size_t contentLength = end - start + 1;

                    std::string content = getPartialFileContent(requestedPath, start, contentLength);

                    _response.statusCode = 206; 
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

void ResponseHandler::parseMultipartFormData(const std::string &body, const std::string &boundary, std::map<std::string, std::string> &fields)
{
    std::string delimiter = "--" + boundary;
    size_t pos = 0;
    size_t end;

    while ((pos = body.find(delimiter, pos)) != std::string::npos)
    {
        pos += delimiter.length();
        if (body.substr(pos, 2) == "--")
            break; 
        if (body.substr(pos, 2) == "\r\n")
            pos += 2;
        end = body.find(delimiter, pos);
        std::string part;
        if (end != std::string::npos)
            part = body.substr(pos, end - pos);
        else
            part = body.substr(pos);

        size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            continue; 
        std::string headers = part.substr(0, headerEnd);
        std::string content = part.substr(headerEnd + 4);

        std::string fieldName;
        std::string fileName;
        std::istringstream headerStream(headers);
        std::string headerLine;
        while (std::getline(headerStream, headerLine))
        {
            if (!headerLine.empty() && headerLine[headerLine.size() - 1] == '\r')
                headerLine = headerLine.substr(0, headerLine.size() - 1);
            size_t colonPos = headerLine.find(':');
            if (colonPos != std::string::npos)
            {
                std::string headerName = trim(headerLine.substr(0, colonPos));
                std::string headerValue = trim(headerLine.substr(colonPos + 1));
                if (headerName == "Content-Disposition")
                {
                    size_t namePos = headerValue.find("name=\"");
                    if (namePos != std::string::npos)
                    {
                        namePos += 6;
                        size_t nameEnd = headerValue.find("\"", namePos);
                        fieldName = headerValue.substr(namePos, nameEnd - namePos);
                    }
                    size_t filenamePos = headerValue.find("filename=\"");
                    if (filenamePos != std::string::npos)
                    {
                        filenamePos += 10;
                        size_t filenameEnd = headerValue.find("\"", filenamePos);
                        fileName = headerValue.substr(filenamePos, filenameEnd - filenamePos);
                    }
                }
            }
        }

        if (!fieldName.empty())
        {
            if (!fileName.empty())
            {
                std::string uploadDir = "./uploads";
                struct stat st;
                if (stat(uploadDir.c_str(), &st) != 0)
                {
                    if (mkdir(uploadDir.c_str(), 0777) != 0)
                    {
                        std::cerr << "Error creating directory: " << uploadDir << std::endl;
                        continue;
                    }
                }
                std::string filePath = "./uploads/" + fileName;
                std::ofstream ofs(filePath.c_str(), std::ios::binary);
                if (ofs)
                {
                    ofs.write(content.c_str(), content.size() - 2); 
                    ofs.close();
                    fields[fieldName] = fileName;
                }
            }
            else fields[fieldName] = content.substr(0, content.size() - 2);
        }
        pos = end;
    }
}

void ResponseHandler::handlePOST()
{
    std::string requestedPath = "." + _request.url;
    if (getFileExtension(requestedPath) == "py")
    {
        executeCGI(requestedPath, _request.body);
        return ;
    }
    std::string contentType = _request.headers.at("Content-Type");
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos != std::string::npos)
        {
            std::string boundary = contentType.substr(boundaryPos + 9);
            std::map<std::string, std::string> formData;
            parseMultipartFormData(_request.body, boundary, formData);
            
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
            return ;
        }
        else
        {
            _response.statusCode = 400;
            _response.statusMessage = "Bad Request";
            _response.body = "Missing boundary in Content-Type header.";
            _response.headers["Content-Type"] = "text/plain";

            std::ostringstream oss;
            oss << _response.body.size();
            _response.headers["Content-Length"] = oss.str();
            return ;
        }
    }
    else if (contentType == "application/x-www-form-urlencoded")
    {
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
            return ;
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
            return ;
        }
    }
    else
    {
        _response.statusCode = 415;
        _response.statusMessage = "Unsupported Media Type";
        _response.body = "Content-Type not supported.";
        _response.headers["Content-Type"] = "text/plain";

        std::ostringstream oss;
        oss << _response.body.size();
        _response.headers["Content-Length"] = oss.str();
        return ;
    }
}

void ResponseHandler::handleDELETE()
{
    std::string filePath = "." + _request.url;

    if (doesFileExist(filePath))
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
        return ;
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
        return ;
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
        return ;
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
        _response.statusCode = 0; 
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
        return ;
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
