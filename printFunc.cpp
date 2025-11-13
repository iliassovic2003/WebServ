#include "WebServer.h"

void printServerConfig(Server& server, int index)
{
    std::cout << "╔═══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                  🚀 Server " << index << "                      ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  📍 Port:        ";
    std::ostringstream portStream;
    for (size_t i = 0; i < server._serverPort.size(); i++) {
        if (i > 0) portStream << ", ";
        portStream << server._serverPort[i];
    }
    std::string portStr = portStream.str();
    std::cout << std::left << std::setw(33) << portStr << "║" << std::endl;
    std::cout << "║  🌐 IP Addr:     " << std::setw(33) << server._serverIp << "║" << std::endl;  
    std::cout << "║  🔖 Name:        " << std::setw(33) << server._serverName << "║" << std::endl;
    std::cout << "║  📁 Root:        " << std::setw(33) << server._serverRoot << "║" << std::endl;
    std::cout << "║  💾 Max Body:     " << std::setw(32) << server._maxBodySize.str << "║" << std::endl;
    std::cout << "║  ⏱️  Req Timeout:  " << std::setw(32) << server._reqTimeout.str  << "║" << std::endl;
    std::cout << "║  ⚡ CGI Timeout:  " << std::setw(32) << server._cgiTimeout.str  << "║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
}

void printInstructions()
{
    std::cout << "\n";
    std::cout << "==========================================================\n";
    std::cout << "                    WebServ HTTP Server                   \n";
    std::cout << "==========================================================\n\n";
    
    std::cout << "USAGE:\n";
    std::cout << "  ./webserv [configuration_file]\n\n";
    
    std::cout << "DESCRIPTION:\n";
    std::cout << "  A lightweight HTTP/1.0 web server implementation.\n";
    std::cout << "  If no configuration file is provided, all the blame on you\n\n";
    
    std::cout << "CONFIGURATION FILE:\n";
    std::cout << "  The configuration file defines server behavior including:\n";
    std::cout << "    - Server Ip ports\n";
    std::cout << "    - Document root directories\n";
    std::cout << "    - Allowed HTTP methods (GET, POST, DELETE)\n";
    std::cout << "    - Error page locations\n";
    std::cout << "    - CGI script handling\n";
    std::cout << "    - File upload settings\n";
    std::cout << "    - Directory auto-indexing\n";
    std::cout << "    - URL redirections\n\n";
    
    std::cout << "IMPORTANT NOTICE:\n";
    std::cout << "  These are important instructions that rules the webserv.\n";
    std::cout << "  Neglecting any of this will end in crash:\n";
    std::cout << "    - swp directory must be always present.\n\n";
    std::cout << "    - config file is highly sensitive to any characters\n";
    std::cout << "    * * No additional white spaces.\n";
    std::cout << "    * * Comments are not handled.\n\n";
    std::cout << "    - Uploading a file with the same name as an old file.\n";
    std::cout << "    * * Results in the loss of the old data.\n\n";
    
    std::cout << "EXIT:\n";
    std::cout << "  Press Ctrl+C to stop the server.\n\n";
    
    std::cout << "==========================================================\n\n";
}
