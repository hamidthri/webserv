/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/27 14:58:54 by htaheri           #+#    #+#             */
/*   Updated: 2024/07/30 15:23:41 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <string>
# include <map>


enum RequestMethod
{
    GET,
    POST,
    DELETE,
    HTTP_UNKNOWN
};

struct HttpRequest
{
    RequestMethod   method;
    std::string     url;
    std::string     version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse
{
    int             statusCode;
    std::string     statusMessage;
    std::string     Date;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse() : statusCode(200), statusMessage("OK") {}
};

#endif