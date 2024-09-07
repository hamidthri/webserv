/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmomeni <mmomeni@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:15:49 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/07 18:39:22 by mmomeni          ###   ########.fr       */
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
    if (queryPos != std::string::npos)
    {
        requestedPath = requestedPath.substr(0, queryPos);
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
