/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response_handler.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:22:48 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/07 19:57:07 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HANDLER_HPP
# define RESPONSE_HANDLER_HPP

# include "request.hpp"
#include <string>
# include "fstream"
#include <map>

off_t fileSize(const std::string &filePath);
class ResponseHandler
{
    private:
        HttpResponse        _response;
        const HttpRequest   &_request;
        std::map<std::string, std::string> _dataStore;


    public:
        ResponseHandler(const HttpRequest &request);
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
        void parseCGIResponse(const std::string &cgiOutput);
        void handleFileUpload(const std::string &uploadDir, const std::string &fileData, const std::string &fileName);
};

#endif