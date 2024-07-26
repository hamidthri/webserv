#include "parsing.hpp"

int main()
{
    Lexer lexer("srcs/nginx.conf");
    std::vector<Token> tokens = lexer.Tokenize();
    for (std::vector<Token>::iterator it = tokens.begin(); it != tokens.end(); it++)
    {
        std::cout << "Token type: " << it->type << " Token value: " << it->value << std::endl;
    }
}