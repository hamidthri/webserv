/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_parser.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 15:02:26 by htaheri           #+#    #+#             */
/*   Updated: 2024/07/27 17:03:27 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
# define REQUEST_PARSER_HPP

# include <iostream>
#include "request.hpp"
#include <vector>

class RequestParser
{
    private:
       std::string _requestData;
       size_t _pos;

    public:
        RequestParser(const std::string &requestData);
        // ~RequestParser();
        std::string readLine();
        std::string readUntil(char delim);
        RequestMethod parseMethod(const std::string &methodStr);
        void parseHeaders(HttpRequest &request);
        HttpRequest parse();
};

#endif