/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vsavolai <vsavolai@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/09 12:16:12 by vsavolai          #+#    #+#             */
/*   Updated: 2024/09/10 16:35:02 by vsavolai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPPARSER_HPP
# define HTTPPARSER_HPP

# include <iostream>
# include <map>
# include <vector>
# include <sstream>
# include <string>
# include <algorithm>

class HttpParser {
    public:
        HttpParser(void);
        HttpParser(const std::string buffer);
        ~HttpParser(void);
        
        bool parseRequest(const std::string buffer);
        
        std::string getMethod() const;
        std::string getResourcePath() const;
        std::string getQueryString() const;
        std::string getHttpVersion() const;
        std::string getrequestBody() const;
        std::map<std::string, std::string> getHeaders() const;


    private:
        std::string							method;
		std::string							resourcePath;
		std::string							queryString;
		std::string							httpVersion;
        std::string                         requestBody;
		std::map<std::string, std::string>	headers = {};
};

#endif