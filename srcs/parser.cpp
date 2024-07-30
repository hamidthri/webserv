/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 18:51:59 by htaheri           #+#    #+#             */
/*   Updated: 2024/07/27 14:32:50 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.hpp"

static const char *mandatoryDirectivesArray[] = {
    "listen",
    "server_name",
    "root"
};

Parser::Parser(const std::vector<Token> &tokens): _tokens(tokens), _pos(0)
{
}

Token Parser::currentToken()
{
    // std::cout << "token: " << _tokens[_pos].value;
    // std::cout << " type: " << _tokens[_pos].type << std::endl;
    if (_pos < _tokens.size())
        return _tokens[_pos];
    else
        return Token(UNKNOWN, "");
}

void Parser::advance()
{
    if (_pos < _tokens.size())
        _pos++;
}

void Parser::expect(TokenType type, const std::string &value)
{
    if (currentToken().type != type || (value != "" && currentToken().value != value))
    {
        throw std::runtime_error("Expected token " + value + " but found " + currentToken().value);
    }
    advance();
}

bool Parser::isTokenType(TokenType type)
{
    return currentToken().type == type;
}

std::vector<ServerBlock> Parser::Parse()
{
    std::vector<ServerBlock> serverBlocks;
    while (_pos < _tokens.size())
    {
        if (isTokenType(KEYWORD) && currentToken().value == "server")
        {
            ServerBlock serverBlock = parseServerBlock();
            validateServerBlock(serverBlock);
            serverBlocks.push_back(serverBlock);
        }
        else
            throw std::runtime_error("Unexpected token " + currentToken().value);
    }
    return serverBlocks;
}


ServerBlock Parser::parseServerBlock()
{
    ServerBlock serverBlock;
    expect(KEYWORD, "server");
    expect(OPEN_BRACKET, "{");
    while (!isTokenType(CLOSE_BRACKET) && _pos < _tokens.size())
    {
        if (isTokenType(KEYWORD) && currentToken().value == "location")
        {
            serverBlock.locations.push_back(parseLocationBlock());
        }
        else
        {
            std::map<std::string, std::string> directives = parseDirectives();
            if (directives.begin()->first.empty())
                throw std::runtime_error("Syntax error: Expected a directive inside server block.");
            serverBlock.directives.insert(directives.begin(), directives.end());
        }
    }
    expect(CLOSE_BRACKET, "}");
    return serverBlock;
}

LocationBlock Parser::parseLocationBlock()
{
    LocationBlock locationBlock;
    expect(KEYWORD, "location");
    expect(VALUE, "");
    expect(OPEN_BRACKET, "{");
    while (!isTokenType(CLOSE_BRACKET) && _pos < _tokens.size())
    {
        std::map<std::string, std::string> directives = parseDirectives();
        if (directives.begin()->first.empty())
            throw std::runtime_error("Syntax error: Expected a directive inside location block.");
        locationBlock.directives.insert(directives.begin(), directives.end());
    }
    expect(CLOSE_BRACKET, "}");
    return locationBlock;
}

std::map<std::string, std::string> Parser::parseDirectives()
{
    std::map<std::string, std::string> directives;
    std::string key = currentToken().value;
    expect(KEYWORD, "");
    
    std::string value;
    while (!isTokenType(SEMICOLON) && _pos < _tokens.size())
    {
        if (isTokenType(OPEN_BRACKET) || isTokenType(CLOSE_BRACKET))
            throw std::runtime_error("Unexpected token " + currentToken().value);
        value += currentToken().value + " ";
        advance();
    }
    if (!isTokenType(SEMICOLON))
        throw std::runtime_error("Expected token ; but found " + currentToken().value);
    advance();
    value.substr(0, value.size() - 1);
    if (value.empty())
        throw std::runtime_error("Syntax error: Expected a value for directive " + key);

    
    directives[key] = value;
    return directives;
}

void Parser::validateServerBlock(const ServerBlock& serverBlock) {
    // Check for mandatory directives
    for (size_t i = 0; i < sizeof(mandatoryDirectivesArray) / sizeof(mandatoryDirectivesArray[0]); ++i) {
        if (serverBlock.directives.find(mandatoryDirectivesArray[i]) == serverBlock.directives.end()) {
            std::ostringstream oss;
            oss << "Syntax error: Missing mandatory directive '" << mandatoryDirectivesArray[i] << "' in server block.";
            throw std::runtime_error(oss.str());
        }
    }
}