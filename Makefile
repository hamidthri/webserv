# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mmomeni <mmomeni@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/07/26 15:17:25 by htaheri           #+#    #+#              #
#    Updated: 2024/09/07 18:37:12 by mmomeni          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCs = srcs/parser.cpp srcs/lexer.cpp srcs/main.cpp srcs/response_handler.cpp srcs/request_parser.cpp

all: $(NAME)

$(NAME): $(SRCs) $(INC)
	$(CC) $(CFLAGS) $(SRCs) -o $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all

.PHONY: all clean fclean re