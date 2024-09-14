/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 15:00:59 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/14 19:01:32 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.hpp"

Lexer::Lexer(const std::string &filename) : _inputFile(filename), _pos(0) {
    if (!_inputFile.is_open())
        throw std::runtime_error("Failed to open file " + filename);
    std::stringstream buffer;
    buffer << _inputFile.rdbuf();
    _input = buffer.str();
    _inputFile.close();
}

char Lexer::currentChar() {
    return _pos < _input.size() ? _input[_pos] : '\0';
}

void Lexer::advance() {
    if (_pos < _input.size())
        _pos++;
}

void Lexer::skipWhitespace() {
    while (std::isspace(currentChar()))
        advance();
}

void Lexer::skipComment() {
    if (currentChar() == '#') {
        while (currentChar() != '\n' && currentChar() != '\0')
            advance();
    }
}

Token Lexer::parseKeyword() {
    std::string value;
    while (std::isalnum(currentChar()) || currentChar() == '_') {
        value += currentChar();
        advance();
    }
    return Token(KEYWORD, value);
}

Token Lexer::parseValue() {
    std::string value;
    while (std::isalnum(currentChar()) || currentChar() == '_' || currentChar() == '.' || currentChar() == '/') {
        value += currentChar();
        advance();
    }
    return Token(VALUE, value);
}

Token Lexer::parseNumericValue() {
    std::string value;
    bool hasDecimal = false;
    while (std::isdigit(currentChar()) || currentChar() == '.') {
        if (currentChar() == '.') {
            if (hasDecimal)
                throw std::runtime_error("Invalid numeric format");
            hasDecimal = true;
        }
        value += currentChar();
        advance();
    }
    return Token(NUMERIC, value);
}

Token Lexer::parseSemicolon() {
    advance();
    return Token(SEMICOLON, ";");
}

Token Lexer::parseOpenBracket() {
    advance();
    return Token(OPEN_BRACKET, "{");
}

Token Lexer::parseCloseBracket() {
    advance();
    return Token(CLOSE_BRACKET, "}");
}

Token Lexer::parseUnknown() {
    std::string value;
    value += currentChar();
    advance();
    return Token(UNKNOWN, value);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (currentChar() != '\0') {
        if (std::isspace(currentChar()))
            skipWhitespace();
        else if (currentChar() == '#')
            skipComment();
        else if (std::isalnum(currentChar()) || currentChar() == '_')
            tokens.push_back(parseKeyword());
        else if (std::isdigit(currentChar()) || currentChar() == '.')
            tokens.push_back(parseNumericValue());
        else if (currentChar() == '/')
            tokens.push_back(parseValue());
        else if (currentChar() == ';')
            tokens.push_back(parseSemicolon());
        else if (currentChar() == '{')
            tokens.push_back(parseOpenBracket());
        else if (currentChar() == '}')
            tokens.push_back(parseCloseBracket());
        else
            tokens.push_back(parseUnknown());
    }
    return tokens;
}

