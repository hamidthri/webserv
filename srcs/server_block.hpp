/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_block.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htaheri <htaheri@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 18:34:12 by htaheri           #+#    #+#             */
/*   Updated: 2024/07/26 19:10:59 by htaheri          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_BLOCK_HPP
# define SERVER_BLOCK_HPP

# include <string>
# include <vector>
# include <map>

struct LocationBlock
{
    std::map<std::string, std::string>    directives;
};

struct ServerBlock
{
    std::map<std::string, std::string>  directives;
    std::vector<LocationBlock>          locations;
};

#endif