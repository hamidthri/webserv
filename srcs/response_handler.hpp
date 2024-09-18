/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/18 20:00:23 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 20:00:24 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HANDLER_HPP
#define RESPONSE_HANDLER_HPP

#include "request.hpp"
#include <string>
#include <map>

off_t fileSize(const std::string &filePath);

struct CGIProcess {
    pid_t pid;                 
    int pipeFd;                
    int clientSock;            
    std::string buffer;        
    time_t startTime;          
};


class ResponseHandler
{
    private:
        HttpResponse        _response;
        const HttpRequest   &_request;
        int                 _clientSocket; 
        std::map<std::string, std::string> _dataStore;

    public:
        
        ResponseHandler(const HttpRequest &request, int clientSocket);
        HttpResponse handleRequest();
        void handleGET();
        void handlePOST();
        void handleDELETE();

        bool doesFileExist(const std::string &path);
        std::string getFileContent(const std::string &path);
        std::string getContentType(const std::string &path);
        std::string getFileExtension(const std::string &path);
        void parseFormData(const std::string &formData, std::map<std::string, std::string> &fields);
        bool isDirectory(const std::string &path);
        std::string generateDirectoryListing(const std::string &path);
        void executeCGI(const std::string &scriptPath, const std::string &queryString);
        void handleFileUpload(const std::string &uploadDir, const std::string &fileData, const std::string &fileName);
        void parseMultipartFormData(const std::string &body, const std::string &boundary, std::map<std::string, std::string> &fields);
};

#endif
