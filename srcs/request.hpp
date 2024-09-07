/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmomeni <mmomeni@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 14:58:54 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/06 21:00:07 by mmomeni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <string>
#include <map>
#include <fstream>

enum RequestMethod
{
    GET,
    POST,
    DELETE,
    HTTP_UNKNOWN
};

struct HttpRequest
{
    RequestMethod method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse
{
    int statusCode;
    std::string statusMessage;
    std::string Date;
    std::map<std::string, std::string> headers;
    std::string body;
    
    bool isFile;          // New flag to indicate if the response is a file
    std::string filePath;  // New field to store the file path

    HttpResponse() : statusCode(200), statusMessage("OK"), isFile(false) {}
};

#endif