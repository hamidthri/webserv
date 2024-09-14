/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 18:38:51 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/14 17:02:55 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "server_block.hpp"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

class Parser {
private:
    std::vector<Token> _tokens;
    size_t _pos;

public:
    Parser(const std::vector<Token>& tokens);
    std::vector<ServerBlock> Parse();

    Token currentToken();
    void advance();
    void expect(TokenType type, const std::string& value = "");
    bool isTokenType(TokenType type);

    ServerBlock parseServerBlock();
    LocationBlock parseLocationBlock();
    std::map<std::string, std::string> parseDirectives();
    void validateServerBlock(const ServerBlock& serverBlock);
};

#endif
