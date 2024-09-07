/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_block.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmomeni <mmomeni@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/26 18:34:12 by htaheri           #+#    #+#             */
/*   Updated: 2024/09/07 19:18:48 by mmomeni          ###   ########.fr       */
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