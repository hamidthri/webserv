/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_parser.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:02:26 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 18:24:31 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "request.hpp"
#include <string>

std::string trim(const std::string &str);


class RequestParser {
private:
    std::string _requestData;
    size_t _pos;

public:
    RequestParser(const std::string &requestData);
    std::string readLine();
    std::string readUntil(char delim);
    RequestMethod parseMethod(const std::string &methodStr);
    void parseHeaders(HttpRequest &request);
    HttpRequest parse();
    HttpRequest parseHeadersOnly();
};

#endif

