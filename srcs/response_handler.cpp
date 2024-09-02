/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:15:49 by htaheri           #+#    #+#             */
/*   Updated: 2024/07/30 15:26:26 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "response_handler.hpp"
# include <sstream>

ResponseHandler::ResponseHandler(const HttpRequest &request) : _request(request)
{
}

bool ResponseHandler::isFileExists(const std::string &path)
{
    std::ifstream file(path.c_str());
    return file.good();
}

std::string ResponseHandler::getFileContent(const std::string &path)
{
    std::ifstream file(path.c_str());
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

std::string ResponseHandler::getContentType(const std::string &path)
{
    std::string extension = getFileExtension(path);
    if (extension == "html")
        return "text/html";
    else if (extension == "css")
        return "text/css";
    else if (extension == "js")
        return "text/javascript";
    else if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    else if (extension == "png")
        return "image/png";
    else if (extension == "gif")
        return "image/gif";
    else if (extension == "bmp")
        return "image/bmp";
    else if (extension == "ico")
        return "image/x-icon";
    else if (extension == "svg")
        return "image/svg+xml";
    else if (extension == "json")
        return "application/json";
    else if (extension == "pdf")
        return "application/pdf";
    else if (extension == "zip")
        return "application/zip";
    else if (extension == "tar")
        return "application/x-tar";
    else if (extension == "txt")
        return "text/plain";
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
    std::string filePath = "." + _request.url;
    if (isFileExists(filePath))
    {
        std::string content = getFileContent(filePath);
        std::string extension = getFileExtension(filePath);
        std::string contentType = getContentType(filePath);
        
        _response.statusCode = 200;
        _response.statusMessage = "OK";
        _response.body = content;
        _response.Date = "Date: " ;
        _response.headers["Content-Type"] = getContentType(filePath);
        _response.headers["Content-Length"] = std::to_string(_response.body.size());
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


