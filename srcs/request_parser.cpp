/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_parser.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/18 20:00:07 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 20:35:18 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request_parser.hpp"

std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    size_t last = str.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last == std::string::npos)
        return "";
    return str.substr(first, last - first + 1);
}

std::string RequestParser::readUntil(char delim)
{
    std::string result;
    while (_pos < _requestData.size() && _requestData[_pos] != delim)
        result += _requestData[_pos++];
    if (_pos < _requestData.size())
        _pos++;
    return result;
}

std::string RequestParser::readLine()
{
    std::string line = readUntil('\n');
    if (!line.empty() && line[line.size() - 1] == '\r')
        line = line.substr(0, line.size() - 1);
    return line;
}

RequestMethod RequestParser::parseMethod(const std::string &methodStr)
{
    if (methodStr == "GET")
        return GET;
    else if (methodStr == "POST")
        return POST;
    else if (methodStr == "DELETE")
        return DELETE;
    return HTTP_UNKNOWN;
}

void RequestParser::parseHeaders(HttpRequest &request)
{
    std::string line;
    while (!(line = readLine()).empty())
    {
        size_t pos = line.find(':');
        if (pos != std::string::npos)
        {
            std::string headerName = trim(line.substr(0, pos));
            std::string headerValue = trim(line.substr(pos + 1));
            if (!headerName.empty() && !headerValue.empty())
                request.headers[headerName] = headerValue;
        }
        else
        {
            throw std::runtime_error("Malformed header line: " + line);
        }
    }
}

HttpRequest RequestParser::parseHeadersOnly()
{
    HttpRequest request;
    std::string requestLine = readLine();
    std::istringstream requestStream(requestLine);

    std::string methodStr;
    requestStream >> methodStr;
    request.method = parseMethod(methodStr);
    if (request.method == HTTP_UNKNOWN)
        throw std::runtime_error("Unknown HTTP method: " + methodStr);

    requestStream >> request.url;
    std::string httpVersion;
    requestStream >> httpVersion;
    request.version = httpVersion;
    if (httpVersion != "HTTP/1.1" && httpVersion != "HTTP/1.0")
        throw std::runtime_error("Unsupported HTTP version: " + httpVersion);

    parseHeaders(request);
    return request;
}

HttpRequest RequestParser::parse()
{
    HttpRequest request = parseHeadersOnly();

    if (request.headers.find("Content-Length") != request.headers.end())
    {
        int contentLength = atoi(request.headers["Content-Length"].c_str());
        if (contentLength > 0)
        {
            if (_pos + contentLength > _requestData.size())
            {
                throw std::runtime_error("Incomplete request body");
            }
            request.body = _requestData.substr(_pos, contentLength);
            _pos += contentLength;
        }
    }
    return request;
}

RequestParser::RequestParser(const std::string &requestData) : _requestData(requestData), _pos(0)
{
}
