#include "lexer.hpp"
#include "parser.hpp"
#include "request_parser.hpp"
#include "response_handler.hpp"
#include <iostream>
#include <string>
#include "request_parser.hpp"
#include "response_handler.hpp"

std::string sampleHttprequest =
    "GET /index.html HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: 13\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Hello, World!";


#include "request_parser.hpp"
#include "response_handler.hpp"
#include <iostream>

void processRequest(const std::string &requestData)
{
    try
    {
        // Parse the request
        RequestParser parser(requestData);
        HttpRequest request = parser.parse();

        // Handle the request
        ResponseHandler handler(request);
        HttpResponse response = handler.handleRequest();

        // Print the response
        std::cout << "HTTP/1.1 " << response.statusCode << " " << response.statusMessage << "\r\n";
        
        for (std::map<std::string, std::string>::const_iterator it = response.headers.begin(); it != response.headers.end(); ++it)
        {
            std::cout << it->first << ": " << it->second << "\r\n";
        }
        std::cout << "\r\n" << response.body << std::endl
                  << std::endl;
        std::cout << "Date: " << response.Date << "\r\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error processing request: " << e.what() << std::endl;
    }
    
}

#include "request_parser.hpp"
#include "response_handler.hpp"

// Function to simulate a GET request to the server
void testGETRequest()
{
    // Example HTTP GET request to retrieve index.html
    std::string sampleHttpGetRequest =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 29\r\n"
        "Connection: keep-alive\r\n"

        "\r\n";

    // Process the request using your server implementation
    processRequest(sampleHttpGetRequest);
}

int main()
{
    // Run the GET request test
    testGETRequest();
    return 0;
}




// int main()
// {
//     Lexer lexer("srcs/nginx.cnf");
//     std::vector<Token> tokens = lexer.Tokenize();
//     Parser parser(tokens);
//     std::vector<ServerBlock> serverBlocks = parser.Parse(); 

//     for (std::vector<ServerBlock>::const_iterator it = serverBlocks.begin(); it != serverBlocks.end(); ++it) {
//             const ServerBlock& server = *it;
//             std::cout << "Server {" << std::endl;

//             // Iterate over directives in the server block
//             for (std::map<std::string, std::string>::const_iterator dirIt = server.directives.begin(); dirIt != server.directives.end(); ++dirIt) {
//                 std::cout << "  " << dirIt->first << " " << dirIt->second << ";" << std::endl;
            
//             }

//             // Iterate over location blocks in the server block
//             for (std::vector<LocationBlock>::const_iterator locIt = server.locations.begin(); locIt != server.locations.end(); ++locIt) {
//                 const LocationBlock& location = *locIt;
//                 std::cout << "  Location {" << std::endl;

//                 // Iterate over directives in the location block
//                 for (std::map<std::string, std::string>::const_iterator locDirIt = location.directives.begin(); locDirIt != location.directives.end(); ++locDirIt) {
//                     std::cout << "    " << locDirIt->first << " " << locDirIt->second << ";" << std::endl;
//                 }
//                 std::cout << "  }" << std::endl;
//             }
//             std::cout << "}" << std::endl;
//         }

     

//     return 0;
// }