# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/07/26 15:17:25 by htaheri           #+#    #+#              #
#    Updated: 2024/07/30 15:58:19 by htaheri          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCs = srcs/parser.cpp srcs/lexer.cpp srcs/main.cpp \
		
INC = srcs/lexer.hpp srcs/parser.hpp srcs/server_block.hpp

all: $(NAME)

$(NAME): $(SRCs) $(INC)
	$(CC) $(CFLAGS) $(SRCs) -o $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all

.PHONY: all clean fclean re