/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 14:42:41 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/18 19:33:02 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <stdexcept>

enum TokenType {
    WHITESPACE,
    KEYWORD,
    COMMENT,
    VALUE,
    NUMERIC,
    SEMICOLON,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;

    Token(TokenType type, const std::string &value): type(type), value(value) {}
};

class Lexer {
private:
    std::ifstream _inputFile;
    std::string _input;
    size_t _pos;

public:
    Lexer(const std::string &filename);
    std::vector<Token> tokenize();

    char currentChar();
    void advance();
    void skipWhitespace();
    void skipComment();

    Token parseKeyword();
    Token parseValue();
    Token parseNumericValue();
    Token parseSemicolon();
    Token parseOpenBracket();
    Token parseCloseBracket();
    Token parseUnknown();
};

#endif
