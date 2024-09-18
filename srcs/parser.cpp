/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 18:51:59 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 19:57:58 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.hpp"
#include <sstream>

static const char* mandatoryDirectivesArray[] = {"listen", "server_name", "root"};
static const size_t mandatoryDirectivesCount = sizeof(mandatoryDirectivesArray) / sizeof(mandatoryDirectivesArray[0]);

Parser::Parser(const std::vector<Token>& tokens): _tokens(tokens), _pos(0) {}

Token Parser::currentToken() {
    if (_pos < _tokens.size())
        return _tokens[_pos];
    else
        return Token(UNKNOWN, "");
}

void Parser::advance() {if (_pos < _tokens.size())
        _pos++;
}

void Parser::expect(TokenType type, const std::string &value) {
    Token ct = currentToken();
    if (ct.type != type || (!value.empty() && ct.value != value)) {
        std::ostringstream oss;
        oss << "Expected token '" << value << "' but found '" << ct.value << "'";
        throw std::runtime_error(oss.str().c_str());
    }
    advance();
}

bool Parser::isTokenType(TokenType type) {
    return currentToken().type == type;
}

std::vector<ServerBlock> Parser::Parse() {
    std::vector<ServerBlock> serverBlocks;
    while (_pos < _tokens.size()) {
        if (isTokenType(KEYWORD) && currentToken().value == "server") {
            serverBlocks.push_back(parseServerBlock());
        } else {
            std::ostringstream oss;
            oss << "Unexpected token '" << currentToken().value << "'";
            throw std::runtime_error(oss.str().c_str());
        }
    }
    return serverBlocks;
}

ServerBlock Parser::parseServerBlock() {
    ServerBlock serverBlock;
    expect(KEYWORD, "server");
    expect(OPEN_BRACKET, "{");
    while (!isTokenType(CLOSE_BRACKET) && _pos < _tokens.size()) {
        if (isTokenType(KEYWORD) && currentToken().value == "location") {
            serverBlock.locations.push_back(parseLocationBlock());
        } else {
            std::map<std::string, std::string> directives = parseDirectives();
            serverBlock.directives.insert(directives.begin(), directives.end());
        }
    }
    expect(CLOSE_BRACKET, "}");
    validateServerBlock(serverBlock);
    return serverBlock;
}

LocationBlock Parser::parseLocationBlock() {
    LocationBlock locationBlock;
    expect(KEYWORD, "location");
    expect(VALUE, "");
    expect(OPEN_BRACKET, "{");
    while (!isTokenType(CLOSE_BRACKET) && _pos < _tokens.size()) {
        std::map<std::string, std::string> directives = parseDirectives();
        locationBlock.directives.insert(directives.begin(), directives.end());
    }
    expect(CLOSE_BRACKET, "}");
    return locationBlock;
}

std::map<std::string, std::string> Parser::parseDirectives() {
    std::map<std::string, std::string> directives;
    std::string key = currentToken().value;
    expect(KEYWORD);

    std::string value;
    while (!isTokenType(SEMICOLON) && _pos < _tokens.size()) {
        value += currentToken().value;
        advance();
        if (!isTokenType(SEMICOLON)) {
            value += " ";
        }
    }

    if (!value.empty() && value[value.size() - 1] == ' ') value.erase(value.size() - 1);

    if (!isTokenType(SEMICOLON)) {
        throw std::runtime_error("Expected ';' at the end of directive value but found '" + currentToken().value + "'");
    }
    advance();

    directives[key] = value;
    return directives;
}


void Parser::validateServerBlock(const ServerBlock& serverBlock) {
    for (size_t i = 0; i < mandatoryDirectivesCount; ++i) {
        if (serverBlock.directives.find(mandatoryDirectivesArray[i]) == serverBlock.directives.end()) {
            std::ostringstream oss;
            oss << "Syntax error: Missing mandatory directive '" << mandatoryDirectivesArray[i] << "' in server block.";
            throw std::runtime_error(oss.str().c_str());
        }
    }
}

