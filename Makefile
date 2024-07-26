# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/07/26 15:17:25 by htaheri           #+#    #+#              #
#    Updated: 2024/07/26 15:19:28 by htaheri          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCs = srcs/parsing.cpp srcs/main.cpp
INC = srcs/parsing.hpp

all: $(NAME)

$(NAME): $(SRCs) $(INC)
	$(CC) $(CFLAGS) $(SRCs) -o $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all

.PHONY: all clean fclean re