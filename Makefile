# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mmomeni <mmomeni@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/07/26 15:17:25 by htaheri           #+#    #+#              #
<<<<<<< HEAD
#    Updated: 2024/09/07 19:52:27 by htaheri          ###   ########.fr        #
=======
#    Updated: 2024/09/07 18:37:12 by mmomeni          ###   ########.fr        #
>>>>>>> 022345d90fa5353b3ccad869617fc0c9cebaf169
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