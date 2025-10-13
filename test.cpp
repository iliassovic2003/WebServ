#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <map>
#include <ctime>
#include "favicon.h"
#include <algorithm>
#include <iomanip>
#include <dirent.h>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>

static int chIndex = 0;
static int clIndex = 0;

enum ClientState
{
    READING_HEADERS,
    SENDING_RESPONSE,
    WAITING_CGI_OUTPUT,
    FINISHED
};

struct stringNum
{
    std::string str;
    uint64_t    num;
};

class errorPage
{
    public:
        int         typeCode;
        std::string link;

        void    assign(int code, std::string lk);
};

class Location
{
    public:
        std::string url;
        std::map<std::string, std::string> directives;
        bool        auto_index;
        bool        is_redirect;
        bool        is_upload;
        bool        is_cgi;
        std::string redirect_url;
        int         redirect_code;
        
        Location(std::string u) : 
            url(u), 
            auto_index(false), 
            is_redirect(false), 
            is_upload(false),
            is_cgi(false),
            redirect_code(0) {}
        
        std::string getValue(const std::string& key) const 
        {
            std::map<std::string, std::string>::const_iterator it = directives.find(key);
            if (it != directives.end())
                return (it->second);
            return ("");
        }
};

class Server 
{
    public:
        std::vector<int>        _serverPort;
        int                     _errorIndex;
        int                     _locIndex;
        std::string             _serverName;
        std::string             _serverIp;
        std::string             _serverRoot;
        stringNum               _reqTimeout;
        stringNum               _cgiTimeout;
        stringNum               _maxBodySize;
        std::vector<errorPage>  _errorPages;
        std::vector<Location>   _locations;


        Server() : _errorIndex(0), _locIndex(0),
                 _serverPort(0), _serverIp("127.0.0.0")
                 {
                    _reqTimeout.num = 0;
                    _cgiTimeout.num = 0;
                    _maxBodySize.num = 0;
                    _reqTimeout.str = "0";
                    _cgiTimeout.str = "0";
                    _maxBodySize.str = "0";
                 };
        ~Server() {};

        void            parseConfigFile(std::string line);
        void            parseLocation(std::string map, std::string url);
        void            getConfigLine(std::string& config);
        uint64_t        parseMemorySize(const std::string& sizeStr);
        uint64_t        parseTimeout(std::string& sizeStr, int defaultValue);
        std::string     checkValue(std::string location, std::string key);
        std::string     findFile(int code);
};

class clientData
{
    public:
        int         fd;
        int         cgiPid;
        int         cgiFd;
        int         fileFd;
        int         serverIndex;
        int         serverPort;
        std::string serverIp;
        std::string clientIp;
        int         clientPort;
        ClientState state;
        std::string request_data;
        std::string response_data;
        std::string headerStr;
        std::string unchunkBody;
        std::string filename;
        size_t      content_length;
        size_t      bytes_sent;
        int         flag;
        int         ccFlag;
        int         posOffset;
        int         is_chunked;
        uint64_t    totalSize;
        bool        headers_complete;
        bool        body_complete;
        int         totalbytes;
        long        requestStartTime;
        std::string CgiBody;

        clientData() : content_length(0), flag(0), posOffset(0), is_chunked(0), 
                      totalSize(0), fd(-1), fileFd(-1), state(READING_HEADERS),
                      bytes_sent(0), headers_complete(false), body_complete(false),
                      ccFlag(1), totalbytes(0), serverIndex(0), serverPort(0),
                      requestStartTime(0), cgiFd(-1), CgiBody("") {}
        ~clientData() {};
};

class httpClientRequest
{
    public:
        std::string _method;
        std::string _path;
        std::string _httpVersion;
        std::string _host;
        std::string _connection;
        std::string _userAgent;
        std::string _accept;
        std::string _acceptLanguage;
        std::string _acceptEncoding;
        std::string _authorization;
        std::string _referer;
        std::string _cookie; // bonus
        std::string _contentType;
        std::string _contentLength;
        std::string _filename;
        std::string _body;
        bool        _mustDownload;
        bool        _conFlag;
        bool        _finishFlag;

        httpClientRequest() : _conFlag(0), _finishFlag(0), _mustDownload(0) {};
        httpClientRequest*  parseRequest(std::string& request);
        void                parseRequestV2(std::string& line);
        ~httpClientRequest() {};
};

struct ServerSocket
{
    int sockfd;
    int port;
    int serIndex;
    
    ServerSocket(int fd, int p, int idx) : sockfd(fd), port(p), serIndex(idx) {}
};

struct FileInfo
{
    std::string name;
    bool isDirectory;
    off_t size;
    time_t modTime;
    
    bool operator<(const FileInfo& other) const
    {
        if (isDirectory != other.isDirectory)
            return isDirectory > other.isDirectory;
        return name < other.name;
    }
};

void    errorPage::assign(int code, std::string lk)
{
    this->typeCode = code;
    this->link = lk;
}


std::string     Server::findFile(int code)
{
    for (int i = 0; i < this->_errorIndex; i++)
    {
        if (this->_errorPages[i].typeCode == code)
            return (this->_errorPages[i].link);
    }
    return ("");
}

void     Server::getConfigLine( std::string& config )
{
    int semiPos = config.find(";");
    int startLocationPos = config.find("location");
    int endLocationPos = config.find("}");
    
    if (semiPos != std::string::npos && startLocationPos != 0) // Fixed condition
    {
        this->parseConfigFile(config.substr(0, semiPos));
        config.erase(0, semiPos + 1);
    }
    else if (startLocationPos != std::string::npos && endLocationPos != std::string::npos)
    {
        config.erase(0, startLocationPos + 9);
        std::string locationURL = config.substr(0, config.find(" "));
        
        config.erase(0, config.find("{") + 1);
        std::string content = config.substr(0, config.find("}") - 1);
        content += ";";
        this->parseLocation(content, locationURL);
        this->_locIndex++;
        config.erase(0, config.find("}") + 1);
    }
    else
    {
        std::cout << "Bad Configuration" << std::endl;
        exit(1);   
    }
}

uint64_t    Server::parseMemorySize(const std::string& sizeStr)
{
    if (sizeStr.empty())
    {
        std::cout << "No Defined Max_Body_Size, Default: 1M." << std::endl; 
        this->_maxBodySize.str = "1M";
        return (1048576);
    }

    uint64_t value = 0;
    size_t i = 0;
    
    while (i < sizeStr.size() && std::isdigit(sizeStr[i]))
    {
        if (value > (UINT64_MAX - (sizeStr[i] - '0')) / 10)
        {
            std::cerr << "Size value too large, Default: 1M." << std::endl;
            this->_maxBodySize.str = "1M";
            return (1048576);
        }
        value = value * 10 + (sizeStr[i] - '0');
        i++;
    }
    
    while (i < sizeStr.size() && std::isspace(sizeStr[i]))
        i++;
    
    if (i < sizeStr.size())
    {
        char unit = std::tolower(sizeStr[i]);
        uint64_t multiplier = 1;
        
        switch (unit)
        {
            case '-':
            {
                std::cerr << "\033[1;31mNegative Value, Default: 1M.\033[0m" << std::endl;
                this->_maxBodySize.str = "1M";
                return (1048576);
            }
            case 'k':
                multiplier = 1024; break;
            case 'm':
                multiplier = 1024 * 1024; break;
            case 'g':
                multiplier = 1024ULL * 1024 * 1024; break;
            case 't':
                multiplier = 1024ULL * 1024 * 1024 * 1024; break;
            case 'b': case '\0':
                    break;
            default:
            {
                std::cerr << "\033[1;31mNot Allowed or Unkown Unit, Default: 1M.\033[0m" << std::endl;
                this->_maxBodySize.str = "1M";
                return (1048576);
            }
        }
        
        if (value > UINT64_MAX / multiplier)
        {
            std::cerr << "\033[1;34mSize value too large after unit conversion\033[0m" << std::endl;
            this->_maxBodySize.str = "1M";
            return (1048576);
        }
        value *= multiplier;
    }

    return (value);
}

uint64_t Server::parseTimeout(std::string& sizeStr, int defaultValue)
{
    if (sizeStr.empty())
    {
        sizeStr = std::to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    std::string numStr;
    char unit = '\0';
    bool isNegative = false;
    
    if (sizeStr[0] == '-')
    {
        isNegative = true;
        std::cerr << "\033[1;31mNegative Value, Default: " << defaultValue << "S.\033[0m" << std::endl;
        sizeStr = std::to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    for (size_t i = 0; i < sizeStr.length(); i++)
    {
        if (std::isdigit(sizeStr[i]))
            numStr += sizeStr[i];
        else if (sizeStr[i] == 'S' || 
                 sizeStr[i] == 'M')
        {
            unit = sizeStr[i];
            break;
        }
        else if (sizeStr[i] == '-')
        {
            isNegative = true;
            std::cerr << "\033[1;31mNegative Value, Default: " << defaultValue << "S.\033[0m" << std::endl;
            sizeStr = std::to_string(defaultValue) + "S";
            return (defaultValue);
        }
    }
    
    if (numStr.empty())
    {
        sizeStr = std::to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    uint64_t value = std::strtoull(numStr.c_str(), NULL, 10);
    
    if (isNegative)
    {
        std::cerr << "\033[1;31mNegative Value, Default: " << defaultValue << "S.\033[0m" << std::endl;
        sizeStr = std::to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    if (unit == 'M')
        return (value * 60);
    else if (unit == 'S')
        return (value);
    else if (unit != '\0')
    {
        std::cerr << "\033[1;31mNo Unit Detected, Default: " << defaultValue << "S.\033[0m" << std::endl;
        sizeStr = std::to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    sizeStr = std::to_string(defaultValue) + "S";
    return (defaultValue);
}

void            Server::parseConfigFile( std::string line )
{
    size_t start = line.find_first_not_of(" \t");
    if (start != std::string::npos)
        line = line.substr(start);
    if (!line.find("listen"))
    {
        int tmpPort;
        int portCount = 0;
        std::stringstream ss(line.substr(6));
        while (ss >> tmpPort)
        {
            portCount++;
            if (portCount > 1)
            {
                std::cout << "Error: Multiple ports in listen directive. Only one port allowed." << std::endl;
                exit(1);
            }
            if (tmpPort >= 1024 && tmpPort <= 65535)
                this->_serverPort.push_back(tmpPort);
            else
            {
                std::cout << "Error: Invalid port " << tmpPort << std::endl;
                exit(1);
            }
        }
    }
    else if (!line.find("server_name"))
        this->_serverName = line.substr(12);
    else if (!line.find("root"))
        this->_serverRoot = line.substr(5);
    else if (!line.find("request_timeout"))
    {
        this->_reqTimeout.str = line.substr(16);
        this->_reqTimeout.num = parseTimeout(this->_reqTimeout.str, 30);
    }
    else if (!line.find("CGI_script_timeout"))
    {
        this->_cgiTimeout.str = line.substr(19);
        this->_cgiTimeout.num = parseTimeout(this->_cgiTimeout.str, 60);
    }
    else if (!line.find("client_max_body_size"))
    {
        this->_maxBodySize.str = line.substr(21);
        this->_maxBodySize.num = parseMemorySize(this->_maxBodySize.str);
    }
    else if (!line.find("address"))
        this->_serverIp = line.substr(8);
    else if (!line.find("error_page"))
    {
        std::stringstream ss(line.substr(11));
        std::string path;
        errorPage ep;
        int code;

        ss >> code >> path;
        ep.assign(code, path);
        this->_errorPages.push_back(ep);
        this->_errorIndex++;
    }
    else
    {
        std::cout << "Bad Configuration" << std::endl;
        exit (1);
    }
}

void Server::parseLocation(std::string map, std::string locationURL)
{
    Location loc(locationURL);
    
    while (!map.empty())
    {
        int semiPos = map.find(";");
        if (semiPos == std::string::npos)
            break;
        
        std::string line = map.substr(0, semiPos);
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos)
            line = line.substr(start);
            
        int spacePos = line.find(" ");
        if (spacePos == std::string::npos)
        {
            map.erase(0, semiPos + 1);
            continue;
        }
        
        std::string key = line.substr(0, spacePos);
        std::string value = line.substr(spacePos + 1);
        size_t valueStart = value.find_first_not_of(" \t");
        if (valueStart != std::string::npos)
            value = value.substr(valueStart);
        
        loc.directives[key] = value;
        
        if (key == "return")
        {
            loc.is_redirect = true;
            std::stringstream ss(value);
            ss >> loc.redirect_code;
            ss >> loc.redirect_url;
        }
        else if (key == "auto_index")
            loc.auto_index = (value == "on");
        else if (key == "upload_enable")
            loc.is_upload = (value == "on");
        else if (key == "cgi_enable")
            loc.is_cgi = (value == "on");
        
        map.erase(0, semiPos + 1);
        size_t mapStart = map.find_first_not_of(" \t\n");
        if (mapStart != std::string::npos)
            map = map.substr(mapStart);
        else
            break;
    }
    _locations.push_back(loc);
}

std::string Server::checkValue(std::string location, std::string key)
{
    for (size_t i = 0; i < _locations.size(); i++)
    {
        if (_locations[i].url == location)
            return _locations[i].getValue(key);
    }
    return ("");
}

void httpClientRequest::parseRequestV2(std::string& line)
{
    if (!line.find("GET ") || !line.find("POST ") || !line.find("DELETE "))
    {
        size_t pathStart = line.find(' ') + 1;
        this->_method.assign(line.substr(0, pathStart - 1));
        size_t secSpace = line.find(' ', pathStart);
        std::string path = line.substr(pathStart, secSpace - pathStart);
        size_t verStart = secSpace + 1;
        size_t line_end = line.find("\r\n", verStart);
        if (line_end == std::string::npos)
            line_end = line.length();
        std::string httpVer = line.substr(verStart, line_end - verStart);
        this->_path.assign(path);
        this->_httpVersion.assign(httpVer);
    }
    else if (!line.find("Host: "))
    {
        size_t line_end = line.find("\r\n");
        this->_host.assign(line.substr(6, line_end - 6));
    }
    else if (!line.find("Filename: "))
    {
        size_t line_end = line.find("\r\n");
        this->_filename.assign(line.substr(10, line_end - 10));
    }
    else if (!line.find("Connection: ") && !this->_conFlag)
    {
        size_t line_end = line.find("\r\n");
        this->_connection.assign(line.substr(12, line_end - 12));
    }
    else if (!line.find("User-Agent: "))
    {
        size_t line_end = line.find("\r\n");
        this->_userAgent.assign(line.substr(12, line_end - 12));
    }
    else if (!line.find("Accept: "))
    {
        size_t line_end = line.find("\r\n");
        this->_accept.assign(line.substr(8, line_end - 8));
    }
    else if (!line.find("Accept-Language: "))
    {
        size_t line_end = line.find("\r\n");
        this->_acceptLanguage.assign(line.substr(17, line_end - 17));
    }
    else if (!line.find("Accept-Encoding: "))
    {
        size_t line_end = line.find("\r\n");
        this->_acceptEncoding.assign(line.substr(17, line_end - 17));
    }
    else if (!line.find("Authorization: "))
    {
        size_t line_end = line.find("\r\n");
        this->_authorization.assign(line.substr(15, line_end - 15));
    }
    else if (!line.find("Referer: "))
    {
        size_t line_end = line.find("\r\n");
        this->_referer.assign(line.substr(9, line_end - 9));
    }
    else if (!line.find("Cookie: "))
    {
        size_t line_end = line.find("\r\n");
        this->_cookie.assign(line.substr(8, line_end - 8));
    }
    else if (!line.find("Content-Type: "))
    {
        size_t line_end = line.find("\r\n");
        this->_contentType.assign(line.substr(14, line_end - 14));
    }
    else if (!line.find("Content-Length: "))
    {
        size_t line_end = line.find("\r\n");
        this->_contentLength.assign(line.substr(16, line_end - 16));
    }
    else if (line.find("Content-Length: ")
        && line.find("Content-Length: ") != std::string::npos)
    {
        this->_connection.assign("close");
        this->_conFlag = 1;
    }
    else if (!line.find("Must-Download: "))
    {
        size_t line_end = line.find("\r\n");
        this->_mustDownload = std::atoi((line.substr(15, line_end - 15)).c_str());
    }
    else if (line == "\r\n")
        this->_finishFlag = 1;
    return;
}

httpClientRequest* httpClientRequest::parseRequest(std::string& request)
{
    std::string line;
    int endPos = 0;

    while (request.length() > 2)
    {
        endPos = request.find("\r\n");
        line = request.substr(0, endPos + 2);
        parseRequestV2(line);
        request.erase(0, endPos + 2);
        if (this->_finishFlag)
            break;
    }
    this->_body.append(request);
    return (this);
}

std::string to_string(int i)
{
    if (i == 0)
        return "0";
    
    std::string result;
    
    while (i > 0)
    {
        result = (char)('0' + (i % 10)) + result;
        i /= 10;
    }
    
    return result;
}

std::string get200Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>WebServ Home</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\nfont-family:'Arial',sans-serif;\n}\nbody{\nbackground:#121212;\ncolor:#ffffff;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\ntext-align:center;\n}\n.container{\nmax-width:600px;\npadding:2rem;\n}\nh1{\nfont-size:2.5rem;\ncolor:#bb86fc;\nmargin-bottom:1rem;\n}\np{\nfont-size:1.2rem;\ncolor:#cccccc;\n}\n.code{\nbackground:#1e1e1e;\npadding:1rem;\nborder-radius:5px;\nmargin-top:2rem;\nfont-family:'Courier New',monospace;\n}\n</style>\n</head>\n<body>\n<div class=\"container\">\n<h1>WebServ Running</h1>\n<p>Your C++ web server is operational</p>\n<div class=\"code\">Status: <span style=\"color:#4caf50\">Online</span></div>\n</div>\n</body>\n</html>";
}

std::string get201Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>WebServ Home</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\nfont-family:'Arial',sans-serif;\n}\nbody{\nbackground:#121212;\ncolor:#ffffff;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\ntext-align:center;\n}\n.container{\nmax-width:600px;\npadding:2rem;\n}\nh1{\nfont-size:2.5rem;\ncolor:#bb86fc;\nmargin-bottom:1rem;\n}\np{\nfont-size:1.2rem;\ncolor:#cccccc;\n}\n.code{\nbackground:#1e1e1e;\npadding:1rem;\nborder-radius:5px;\nmargin-top:2rem;\nfont-family:'Courier New',monospace;\n}\n</style>\n</head>\n<body>\n<div class=\"container\">\n<h1>WebServ Running</h1>\n<p>Your C++ web server is operational</p>\n<div class=\"code\">Status: <span style=\"color:#4caf50\">Online</span></div>\n</div>\n</body>\n</html>";
}

std::string get400Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>400 - Bad Request</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ff6b6b,#ffa726);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(255,107,107,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ffa726;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ff6b6b,#ffa726,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">400</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"The universe is not required to be in perfect harmony with human ambition.\"</p><p class=\"author\">- Carl Sagan</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get403Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>403 - Forbidden</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ef5350,#ab47bc);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(239,83,80,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ab47bc;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ef5350,#ab47bc,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">403</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"Some doors are not meant to be opened; some paths are not meant to be taken.\"</p><p class=\"author\">- Ancient Proverb</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get404Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>404 - Page Not Found</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\n}\nbody{\nbackground:#121212;\ncolor:#e0e0e0;\nfont-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\noverflow:hidden;\nposition:relative;\n}\n.container{\ntext-align:center;\nz-index:2;\nposition:relative;\npadding:2rem;\nbackground:rgba(18,18,18,0.8);\nborder-radius:15px;\nbackdrop-filter:blur(10px);\nborder:1px solid rgba(255,255,255,0.1);\n}\n.error-code{\nfont-size:8rem;\nfont-weight:900;\nbackground:linear-gradient(45deg,#ff6b6b,#4ecdc4);\n-webkit-background-clip:text;\n-webkit-text-fill-color:transparent;\ntext-shadow:0 0 30px rgba(255,107,107,0.3);\nmargin-bottom:1rem;\n}\n.error-message{\nfont-size:1.5rem;\nmargin-bottom:2rem;\ncolor:#b0b0b0;\n}\n.philosophical-quote{\nborder-left:3px solid #4ecdc4;\npadding-left:1.5rem;\nmargin:1.5rem 0;\n}\n.quote{\nfont-style:italic;\ncolor:#e0e0e0;\nfont-size:1.1rem;\nmargin:0;\nline-height:1.6;\n}\n.author{\ncolor:#b0b0b0;\nfont-size:0.9rem;\nmargin-top:0.5rem;\ntext-align:right;\n}\n.home-btn{\ndisplay:inline-block;\npadding:12px 30px;\nbackground:linear-gradient(45deg,#667eea,#764ba2);\ncolor:white;\ntext-decoration:none;\nborder-radius:25px;\nfont-weight:600;\ntransition:all 0.3s ease;\nborder:none;\ncursor:pointer;\n}\n.home-btn:hover{\ntransform:translateY(-2px);\nbox-shadow:0 10px 25px rgba(102,126,234,0.3);\n}\n.floating{\nposition:absolute;\nborder-radius:50%;\nbackground:linear-gradient(45deg,#ff6b6b,#4ecdc4,#667eea);\nopacity:0.1;\nanimation:float 6s ease-in-out infinite;\n}\n.floating:nth-child(1){\nwidth:80px;\nheight:80px;\ntop:20%;\nleft:10%;\nanimation-delay:0s;\n}\n.floating:nth-child(2){\nwidth:120px;\nheight:120px;\ntop:60%;\nright:15%;\nanimation-delay:2s;\n}\n.floating:nth-child(3){\nwidth:60px;\nheight:60px;\nbottom:20%;\nleft:20%;\nanimation-delay:4s;\n}\n@keyframes float{\n0%,100%{\ntransform:translateY(0px) rotate(0deg);\n}\n50%{\ntransform:translateY(-20px) rotate(180deg);\n}\n}\n@media (max-width:768px){\n.error-code{\nfont-size:6rem;\n}\n.error-message{\nfont-size:1.2rem;\n}\n.container{\nmargin:1rem;\npadding:1.5rem;\n}\n}\n</style>\n</head>\n<body>\n<div class=\"floating\"></div>\n<div class=\"floating\"></div>\n<div class=\"floating\"></div>\n<div class=\"container\">\n<div class=\"error-code\">404</div>\n<div class=\"error-message\">\n<div class=\"philosophical-quote\">\n<p class=\"quote\">\"When you stare into the abyss, the abyss stares back at you.\"</p>\n<p class=\"author\">- Friedrich Nietzsche</p>\n</div>\n</div>\n<button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button>\n</div>\n<script>\nfunction redirectToHome(){\nwindow.location.href=\"/\";\n}\ndocument.addEventListener('mousemove',(e)=>\n{const floating=document.querySelectorAll('.floating');\nfloating.forEach(element=>{\nconst speed=parseInt(element.style.width)/100;\nconst x=(window.innerWidth-e.pageX*speed)/100;\nconst y=(window.innerHeight-e.pageY*speed)/100;\nelement.style.transform=`translateX(${x}px) translateY(${y}px)`;\n});\n});\ndocument.addEventListener('keydown',(e)=>\n{if(e.key==='Enter')redirectToHome();\n});\n</script>\n</body>\n</html>";
}

std::string get405Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>405 - Method Not Allowed</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#26c6da,#42a5f5);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(38,198,218,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #42a5f5;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#26c6da,#42a5f5,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">405</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"Anyone who has never made a mistake has never tried anything new\"</p><p class=\"author\">- Einstein</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get408Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>408 - Request Timeout</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ffa726,#ff7043);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(255,167,38,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ff7043;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ffa726,#ff7043,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">408</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"Time is the most valuable thing a man can spend.\"</p><p class=\"author\">- Theophrastus</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Try Again</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get413Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>413 - Payload Too Large</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ef5350,#ec407a);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(239,83,80,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ec407a;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ef5350,#ec407a,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">413</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"The greatest wealth is to live content with little.\"</p><p class=\"author\">- Plato</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get500Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>500 - Internal Server Error</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#2c5da3,#1a781e);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(239,83,80,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #1a781e;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ef5350,#ec407a,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:110px;height:110px;top:10%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:160px;height:160px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:10%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">500</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"The hardest battles are the ones we fight within ourselves with ourselves.\"</p><p class=\"author\">- Dr. Shefali Tsabary</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string    createResponse( int code, clientData *data )
{
    std::string             bodyResponse = "";
    std::stringstream       ss;
    std::string             str;

    if (code == 200)
    {
        ss << get200Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get200Page();
    }
    else if (code == 201)
    {
        ss << get201Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 201 Created\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get201Page();
    }
    else if (code == 400)
    {
        ss << get400Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get400Page();
    }
    else if (code == 403)
    {
        ss << get403Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get403Page();
    }
    else if (code == 404)
    {
        ss << get404Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get404Page();
    }
    else if (code == 405)
    {
        ss << get405Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get405Page();
    }
    else if (code == 408)
    {
        ss << get408Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 408 Request Timeout\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get408Page();
    }
    else if (code == 413)
    {
        ss << get413Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 413 Payload Too Large\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get413Page();
    }
    else if (code == 500)
    {
        ss << get500Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get500Page();
    }
    data->state = SENDING_RESPONSE;
    return (bodyResponse);
}

std::string getStatusMessage(int code)
{
    switch (code)
    {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default: return "Unknown Error";
    }
}

std::string sendErrorCode(int code, Server server, clientData *data)
{
    std::string location = server.checkValue("/errors", "root");
    
    if (location.empty())
        return (createResponse(code, data));
    
    std::string cleanLocation = location;
    if (!cleanLocation.empty() && cleanLocation[0] == '/')
        cleanLocation = cleanLocation.substr(1);
    
    std::string errorFile = server.findFile(code);
    if (errorFile.empty())
        return (createResponse(code, data));
    
    std::string fileName = cleanLocation;
    if (!fileName.empty() && fileName[fileName.length() - 1] != '/')
        fileName += "/";
    fileName += errorFile;
    
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 0)
        return (createResponse(500, data));

    std::string fileContent;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
        fileContent.append(buffer, bytesRead);
    close(fd);

    if (bytesRead == -1)
        return (createResponse(500, data));

    std::string response = "HTTP/1.0 " + std::to_string(code) + " " + getStatusMessage(code) + "\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + std::to_string(fileContent.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += fileContent;
    return (response);
}

size_t hexToDecimal(const std::string& hexStr)
{
    size_t result;
    std::stringstream ss;
    ss << std::hex << hexStr;
    ss >> result;
    return result;
}

int handleChunkedRequest(clientData* cData, Server& server)
{
    uint64_t maxSize = server._maxBodySize.num;

    int cfd = open(cData->filename.c_str(), O_RDWR | O_APPEND | O_CREAT, 0644);
    if (cfd < 0)
    {
        perror("open()");
        return (close(cfd), 2);
    }

    while (cData->request_data.length())
    {
        size_t ch_size_end = cData->request_data.find("\r\n");

        if (ch_size_end == std::string::npos)
            return (close(cfd), 0);
        
        std::string ch_size_str = cData->request_data.substr(0, ch_size_end);
        size_t ch_size = hexToDecimal(ch_size_str);

        if (ch_size == 0)
        {
            if (ch_size_end + 4 <= cData->request_data.length())
            {
                cData->request_data.erase(0, ch_size + 4);
                return (close(cfd), 1);
            }
            return (close(cfd), 0);
        }

        size_t ch_data_start = ch_size_end + 2;
        size_t ch_data_end = ch_data_start + ch_size;

        if (cData->request_data.length() < ch_data_end + 2)
            return (close(cfd), 0);

        cData->request_data.erase(0, ch_data_start);

        std::string chunk_data = cData->request_data.substr(0, ch_size);
        ssize_t bytes_written = write(cfd, chunk_data.c_str(), chunk_data.size());
        if (bytes_written < 0)
        {
            perror("write()");
            return (2);
        }
        cData->totalSize += bytes_written;
        if (cData->totalSize > maxSize)
        {
            sendErrorCode(413, server, cData);
            return (2);
        }
        if (static_cast<size_t>(bytes_written) != chunk_data.size())
        {
            std::cerr << "Warning: Incomplete write to file" << std::endl;
            return (2);
        }
        cData->request_data.erase(0, ch_size + 2);
    }
    return (close(cfd), 0);
}

int handleContentLengthRequest(clientData* cData, Server& server)
{
    uint64_t maxSize = server._maxBodySize.num;
    if (cData->content_length > maxSize)
    {
        sendErrorCode(413, server, cData);
        return (2);
    }

    std::string& data = cData->request_data;

    if (cData->filename.empty())
    {
        cData->filename = server._serverRoot.substr(1) + "/swp/.clength_" + to_string(clIndex++) + ".swp";
        int fd = open(cData->filename.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            sendErrorCode(500, server, cData);
            return (2);
        }
        int bytesWritten = write(fd, cData->headerStr.c_str(), cData->headerStr.length());
        if (bytesWritten < 0)
        {
            sendErrorCode(500, server, cData);
            close(fd);
            return (2);
        }
        close(fd);
    }
    int fd = open(cData->filename.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
    {
        sendErrorCode(500, server, cData);
        return (2);
    }

    int bytesWritten = write(fd, data.c_str(), data.length());
    if (bytesWritten < 0)
    {
        sendErrorCode(500, server, cData);
        close(fd);
        return (2);
    }
    close(fd);
    data.erase();

    cData->totalbytes += bytesWritten;
    if (cData->totalbytes == cData->content_length)
        return (1);
    return (0);
}

Location* findLocation(Server& server, const std::string& path)
{
    for (size_t i = 0; i < server._locations.size(); i++)
        if (path == server._locations[i].url)
            return &server._locations[i];
    return (NULL);
}

bool isValidCgiRequest(httpClientRequest* req, Server server)
{
    
    std::string path = req->_path;

    int pos = path.find_last_of(".");
    if (pos == std::string::npos)
        return (false);

    std::string extension = path.substr(pos, path.length());
    
    pos = path.find_last_of("/");
    if (pos == std::string::npos)
        return (false);
    
    std::string tmp = path.substr(0, pos);

    Location* loc = findLocation(server, tmp);
    if (loc)
    {
        if (!loc->getValue(extension).empty() && loc->is_cgi)
            return (true);
    }
    return (false);
}

std::string extractFileContent(const std::string& body)
{
    size_t headers_end = body.find("\r\n\r\n");
    if (headers_end == std::string::npos)
        return body;
    
    return (body.substr(headers_end + 4));
}

std::string checkFilename(const std::string& filename)
{
    std::string safe_name;
    const std::string forbidden_chars = "\\/:*?\"<>|";
    
    for (char c : filename)
        if (forbidden_chars.find(c) == std::string::npos)
            safe_name += c;
    
    return safe_name;
}

bool saveFile(const std::string& filename, const std::string& content,
        Server server, Location *loc)
{
    std::string safe_name = checkFilename(filename);
    std::string half_name = loc->getValue("upload_store");
    std::string full_name = half_name.substr(1) + "/" + safe_name;
    std::cout << full_name << std::endl;

    std::ofstream file(full_name, std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

std::string getCurrentTimestamp()
{
    char buffer[20];
    std::time_t now = std::time(nullptr);

    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", std::localtime(&now));
    return (std::string(buffer));
}

std::string getFileExtensionFromContentType(const std::string& contentType)
{
    // Application types
    if (contentType.find("application/json") != std::string::npos)
        return ".json";
    if (contentType.find("application/pdf") != std::string::npos)
        return ".pdf";
    if (contentType.find("application/zip") != std::string::npos)
        return ".zip";
    if (contentType.find("application/gzip") != std::string::npos)
        return ".gz";
    if (contentType.find("application/tar") != std::string::npos)
        return ".tar";
    if (contentType.find("application/rar") != std::string::npos)
        return ".rar";

    // Text types
    if (contentType.find("text/plain") != std::string::npos)
        return ".txt";
    if (contentType.find("text/html") != std::string::npos)
        return ".html";
    if (contentType.find("text/css") != std::string::npos)
        return ".css";
    if (contentType.find("text/javascript") != std::string::npos)
        return ".js";

    // Image types
    if (contentType.find("image/jpeg") != std::string::npos)
        return ".jpg";
    if (contentType.find("image/png") != std::string::npos)
        return ".png";
    if (contentType.find("image/gif") != std::string::npos)
        return ".gif";
    if (contentType.find("image/webp") != std::string::npos)
        return ".webp";

    // Audio types
    if (contentType.find("audio/aac") != std::string::npos)
        return ".aac";
    if (contentType.find("audio/mpeg") != std::string::npos)
        return ".mp3";
    if (contentType.find("audio/wav") != std::string::npos)
        return ".wav";

    // Video types
    if (contentType.find("video/mp4") != std::string::npos)
        return ".mp4";
    if (contentType.find("video/mpeg") != std::string::npos)
        return ".mpeg";
    if (contentType.find("video/ogg") != std::string::npos)
        return ".ogv";
    if (contentType.find("video/webm") != std::string::npos)
        return ".webm";

    return ".bin";
}

std::string generateFilename(httpClientRequest* req)
{
    std::string extension = getFileExtensionFromContentType(req->_contentType);

    if (!req->_filename.empty())
        return (checkFilename(req->_filename) + extension);
    return ("u_" + getCurrentTimestamp() + extension);
}


std::string getContentType(const std::string& filename)
{
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos)
        return "application/octet-stream";
    
    std::string ext = filename.substr(dotPos);
    
    // Application types
    if (ext == ".json") return "application/json";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    if (ext == ".gz") return "application/gzip";
    if (ext == ".tar") return "application/tar";
    if (ext == ".rar") return "application/rar";
    if (ext == ".bin") return "application/octet-stream";

    // Text types
    if (ext == ".txt") return "text/plain";
    if (ext == ".html") return "text/html";
    if (ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "text/javascript";

    // Image types
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".webp") return "image/webp";
    if (ext == ".svg") return "image/svg+xml";

    // Audio types
    if (ext == ".aac") return "audio/aac";
    if (ext == ".mp3") return "audio/mpeg";
    if (ext == ".wav") return "audio/wav";

    // Video types
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".mpeg") return "video/mpeg";
    if (ext == ".ogv") return "video/ogg";
    if (ext == ".webm") return "video/webm";
    if (ext == ".avi") return "video/x-msvideo";
    if (ext == ".mov") return "video/quicktime";

    return "application/octet-stream";
}

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

void cleanupClient(int epoll_fd, clientData *cData, std::map<int, clientData*> &clientDataMap)
{
    if (!cData) return;

    printf("Cleaning up client FD %d\n", cData->fd);

    if (cData->cgiFd > 0) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiFd, NULL);
        clientDataMap.erase(cData->cgiFd);
        close(cData->cgiFd);
        cData->cgiFd = -1;
        
        if (cData->cgiPid > 0) 
        {
            kill(cData->cgiPid, SIGTERM);
            waitpid(cData->cgiPid, NULL, WNOHANG);
            cData->cgiPid = -1;
        }
    }
    
    if (cData->fd > 0) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->fd, NULL);
        clientDataMap.erase(cData->fd);
        close(cData->fd);
        cData->fd = -1;
    }
    
    delete cData;
}

std::string replacePlaceholders(std::string content, Server& server,
        clientData *data, Location loc)
{
    size_t pos;
    
    while ((pos = content.find("{{SERVER_NAME}}")) != std::string::npos)
        content.replace(pos, 15, server._serverName);
    
    while ((pos = content.find("{{PORT}}")) != std::string::npos)
        content.replace(pos, 8, std::to_string(data->serverPort));
    
    while ((pos = content.find("{{SERVER_IP}}")) != std::string::npos)
        content.replace(pos, 13, server._serverIp);
    
    std::string successRedirect = loc.getValue("success_redirect");
    while ((pos = content.find("{{SUCCESS}}")) != std::string::npos)
        content.replace(pos, 11, successRedirect);
    
    std::string failedRedirect = loc.getValue("failed_redirect");
    while ((pos = content.find("{{FAILED}}")) != std::string::npos)
        content.replace(pos, 10, failedRedirect);
    
    std::string maxSize = server._maxBodySize.str;
    while ((pos = content.find("{{MAXSIZE}}")) != std::string::npos)
        content.replace(pos, 11, maxSize);
    
    return content;
}

std::string serveFile(const std::string& filePath, httpClientRequest *req,
        clientData *data, Server& server, Location loc)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    
    if (!file.is_open())
        return (sendErrorCode(404, server, data));
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    content = replacePlaceholders(content, server, data, loc);
    std::string contentType = getContentType(filePath);
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;
    
    return (response);
}

std::string formatFileSize(off_t size)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double displaySize = static_cast<double>(size);
    
    while (displaySize >= 1024.0 && unitIndex < 3)
    {
        displaySize /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << displaySize << " " << units[unitIndex];
    return oss.str();
}

std::string formatDateTime(time_t time)
{
    struct tm* timeinfo = localtime(&time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
    return std::string(buffer);
}

std::string getFileIcon(const std::string& filename, bool isDirectory)
{
    if (isDirectory)
        return "📁";
    
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos)
        return "📄";
    
    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".html" || ext == ".htm")
        return "📄";
    else if (ext == ".css")
        return "🎨";
    else if (ext == ".js")
        return "⚡";
    else if (ext == ".json")
        return "📊";
    else if (ext == ".txt")
        return "📝";
    else if (ext == ".zip" || ext == ".tar" || ext == ".gz" || ext == ".rar")
        return "📦";
    else if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".svg" || ext == ".webp")
        return "🖼️";
    else if (ext == ".mp4" || ext == ".avi" || ext == ".mov" || ext == ".mkv" || ext == ".mpeg" || ext == ".ogv" || ext == ".webm")
        return "🎬";
    else if (ext == ".mp3" || ext == ".wav" || ext == ".aac")
        return "🎵";
    else if (ext == ".pdf")
        return "📕";
    else if (ext == ".py")
        return "🐍";
    else if (ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".hpp")
        return "⚙️";
    
    return "📄";
}

std::string readAutoIndexTemplate()
{
    return R"(<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Index of {{DIRECTORY_PATH}}</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#0a0a0a;color:#e0e0e0;min-height:100vh;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;overflow-x:hidden;}.bubbles{position:fixed;top:0;left:0;width:100%;height:100%;z-index:0;overflow:hidden;pointer-events:none;}.bubble{position:absolute;bottom:-100px;background:radial-gradient(circle,rgba(102,126,234,0.3),rgba(118,75,162,0.2));border-radius:50%;opacity:0.6;animation:rise 15s infinite ease-in;}.bubble:nth-child(1){width:80px;height:80px;left:10%;animation-duration:12s;}.bubble:nth-child(2){width:60px;height:60px;left:20%;animation-duration:14s;animation-delay:2s;}.bubble:nth-child(3){width:100px;height:100px;left:35%;animation-duration:16s;animation-delay:4s;}.bubble:nth-child(4){width:70px;height:70px;left:50%;animation-duration:13s;animation-delay:1s;}.bubble:nth-child(5){width:90px;height:90px;left:65%;animation-duration:15s;animation-delay:3s;}.bubble:nth-child(6){width:120px;height:120px;left:75%;animation-duration:17s;animation-delay:5s;}.bubble:nth-child(7){width:75px;height:75px;left:85%;animation-duration:14s;animation-delay:2.5s;}@keyframes rise{0%{bottom:-100px;transform:translateX(0);opacity:0.6;}50%{transform:translateX(100px);opacity:0.8;}100%{bottom:110%;transform:translateX(-50px);opacity:0;}}.container{position:relative;z-index:1;max-width:1000px;margin:0 auto;padding:3rem 2rem;min-height:100vh;}header{text-align:center;margin-bottom:3rem;padding:2rem;backdrop-filter:blur(10px);}h1{font-size:2.5rem;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;margin-bottom:0.5rem;}.path{font-size:1.1rem;color:#a0a0a0;margin-top:0.5rem;font-family:'Courier New',monospace;}.breadcrumb{display:inline-block;background:rgba(102,126,234,0.1);padding:0.5rem 1rem;border-radius:8px;margin-top:1rem;border:1px solid rgba(102,126,234,0.3);}.directory-list{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:2rem;backdrop-filter:blur(10px);}.list-header{display:grid;grid-template-columns:40px 2fr 1fr 1fr;gap:1rem;padding:1rem 1.5rem;background:rgba(102,126,234,0.1);border-radius:8px;margin-bottom:1rem;font-weight:600;color:#667eea;font-size:0.9rem;text-transform:uppercase;letter-spacing:0.5px;}.file-item{display:grid;grid-template-columns:40px 2fr 1fr 1fr;gap:1rem;padding:1rem 1.5rem;background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.08);border-radius:8px;margin-bottom:0.5rem;transition:all 0.3s ease;align-items:center;text-decoration:none;color:inherit;}.file-item:hover{background:rgba(255,255,255,0.08);border-color:rgba(102,126,234,0.5);box-shadow:0 4px 16px rgba(102,126,234,0.2);transform:translateX(5px);}.icon{font-size:1.5rem;text-align:center;}.file-name{color:#e0e0e0;font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}.directory .file-name{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;font-weight:600;}.file-size{color:#a0a0a0;font-size:0.9rem;text-align:right;}.file-date{color:#888;font-size:0.9rem;text-align:right;}.parent-link{background:rgba(102,126,234,0.15);border-color:rgba(102,126,234,0.3);}.parent-link:hover{background:rgba(102,126,234,0.25);border-color:rgba(102,126,234,0.6);}.stats{display:flex;justify-content:center;gap:2rem;margin-top:2rem;padding:1.5rem;background:rgba(255,255,255,0.03);border-radius:8px;}.stat-item{text-align:center;}.stat-value{font-size:1.5rem;font-weight:600;background:linear-gradient(135deg,#D11B00 0%,#00B5D1 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;}.stat-label{color:#888;font-size:0.85rem;margin-top:0.3rem;}footer{text-align:center;margin-top:4rem;padding:1.5rem;color:#666;border-top:1px solid rgba(255,255,255,0.1);}@media (max-width:768px){.list-header,.file-item{grid-template-columns:30px 1fr;}.file-size,.file-date,.list-header .file-size,.list-header .file-date{display:none;}}</style></head><body><div class="bubbles"><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div></div><div class="container"><header><h1>Directory Index</h1><p class="path"><span class="breadcrumb">{{DIRECTORY_PATH}}</span></p></header><div class="directory-list"><div class="list-header"><div></div><div>Name</div><div>Size</div><div>Modified</div></div>{{FILE_LIST}}</div><div class="stats"><div class="stat-item"><div class="stat-value">{{FILE_COUNT}}</div><div class="stat-label">Files</div></div><div class="stat-item"><div class="stat-value">{{DIR_COUNT}}</div><div class="stat-label">Directories</div></div><div class="stat-item"><div class="stat-value">{{TOTAL_SIZE}}</div><div class="stat-label">Total Size</div></div></div><footer><p>WebServ Auto-Index • HTTP/1.0</p><p style="margin-top:0.5rem;font-size:0.9rem;">Powered by caffeine and determination</p></footer></div></body></html>)";
}

std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return (str);
}

std::string autoIndex(Location* loc, Server server,
        clientData *data, std::string url, httpClientRequest *req)
{
    if (loc->auto_index == false)
        return (sendErrorCode(500, server, data));
    if (access(url.c_str(), F_OK) != 0)
        return (sendErrorCode(404, server, data));
    if (access(url.c_str(), R_OK | X_OK) != 0)
        return (sendErrorCode(403, server, data));
    
    DIR* dir = opendir(url.c_str());
    if (!dir)
        return (sendErrorCode(500, server, data));
    
    std::vector<FileInfo> files;
    struct dirent* entry;
    uint64_t totalSize = 0;
    int fileCount = 0;
    int dirCount = 0;
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        
        FileInfo info;
        info.name = entry->d_name;
        
        std::string fullPath = url + "/" + info.name;
        struct stat fileStat;
        
        if (stat(fullPath.c_str(), &fileStat) == 0)
        {
            info.isDirectory = S_ISDIR(fileStat.st_mode);
            info.size = fileStat.st_size;
            info.modTime = fileStat.st_mtime;
            
            if (info.isDirectory)
                dirCount++;
            else
            {
                fileCount++;
                totalSize += info.size;
            }
            files.push_back(info);
        }
    }
    closedir(dir);
    
    std::sort(files.begin(), files.end());
    std::ostringstream fileListHtml;

    if (url != "/" && url != ".")
    {
        std::string parentPath = req->_path;

        if (parentPath.length() > 1 && parentPath[parentPath.length() - 1] == '/')
            parentPath = parentPath.substr(0, parentPath.length() - 1);
        
        size_t lastSlash = parentPath.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            if (lastSlash == 0)
                parentPath = "/";
            else
                parentPath = parentPath.substr(0, lastSlash);
        }
        
        fileListHtml << "<a href=\"" << parentPath << "\" class=\"file-item parent-link directory\">"
                    << "<div class=\"icon\">📁</div>"
                    << "<div class=\"file-name\">..</div>"
                    << "<div class=\"file-size\">-</div>"
                    << "<div class=\"file-date\">-</div>"
                    << "</a>";
    }
    
    for (size_t i = 0; i < files.size(); i++)
    {
        const FileInfo& file = files[i];
        
        std::string href = req->_path + "/" + file.name;
        
        std::string classAttr = file.isDirectory ? "file-item directory" : "file-item";
        std::string icon = getFileIcon(file.name, file.isDirectory);
        std::string size = file.isDirectory ? "-" : formatFileSize(file.size);
        std::string date = formatDateTime(file.modTime);
        
        fileListHtml << "<a href=\"" << href << "\" class=\"" << classAttr << "\">"
                     << "<div class=\"icon\">" << icon << "</div>"
                     << "<div class=\"file-name\">" << file.name << "</div>"
                     << "<div class=\"file-size\">" << size << "</div>"
                     << "<div class=\"file-date\">" << date << "</div>"
                     << "</a>";
    }
    
    std::string html = readAutoIndexTemplate();
    
    html = replaceAll(html, "{{DIRECTORY_PATH}}", url);
    html = replaceAll(html, "{{FILE_LIST}}", fileListHtml.str());
    html = replaceAll(html, "{{FILE_COUNT}}", std::to_string(fileCount));
    html = replaceAll(html, "{{DIR_COUNT}}", std::to_string(dirCount));
    html = replaceAll(html, "{{TOTAL_SIZE}}", formatFileSize(totalSize));
    
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html; charset=utf-8\r\n";
    response << "Content-Length: " << html.length() << "\r\n";
    response << "Server: WebServ/1.0\r\n";
    response << "\r\n";
    response << html;
    
    return (response.str());
}

std::string readFileViewerTemplate()
{
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{FILENAME}}</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { background: #0a0a0a; color: #e0e0e0; min-height: 100vh; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; overflow-x: hidden; }
        .bubbles { position: fixed; top: 0; left: 0; width: 100%; height: 100%; z-index: 0; overflow: hidden; pointer-events: none; }
        .bubble { position: absolute; bottom: -100px; background: radial-gradient(circle, rgba(102, 126, 234, 0.3), rgba(118, 75, 162, 0.2)); border-radius: 50%; opacity: 0.6; animation: rise 15s infinite ease-in; }
        .bubble:nth-child(1) { width: 80px; height: 80px; left: 10%; animation-duration: 12s; }
        .bubble:nth-child(2) { width: 60px; height: 60px; left: 20%; animation-duration: 14s; animation-delay: 2s; }
        .bubble:nth-child(3) { width: 100px; height: 100px; left: 35%; animation-duration: 16s; animation-delay: 4s; }
        .bubble:nth-child(4) { width: 70px; height: 70px; left: 50%; animation-duration: 13s; animation-delay: 1s; }
        .bubble:nth-child(5) { width: 90px; height: 90px; left: 65%; animation-duration: 15s; animation-delay: 3s; }
        .bubble:nth-child(6) { width: 120px; height: 120px; left: 75%; animation-duration: 17s; animation-delay: 5s; }
        .bubble:nth-child(7) { width: 75px; height: 75px; left: 85%; animation-duration: 14s; animation-delay: 2.5s; }
        @keyframes rise { 0% { bottom: -100px; transform: translateX(0); opacity: 0.6; } 50% { transform: translateX(100px); opacity: 0.8; } 100% { bottom: 110%; transform: translateX(-50px); opacity: 0; } }
        .container { position: relative; z-index: 1; max-width: 1400px; margin: 0 auto; padding: 3rem 2rem; min-height: 100vh; }
        header { text-align: center; margin-bottom: 2rem; padding: 2rem; backdrop-filter: blur(10px); }
        .file-icon { font-size: 4rem; margin-bottom: 1rem; }
        h1 { font-size: 2rem; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text; margin-bottom: 0.5rem; word-break: break-all; }
        .file-info { display: flex; justify-content: center; gap: 2rem; margin-top: 1rem; font-size: 0.9rem; color: #888; }
        .file-info span { color: #667eea; font-weight: 600; }
        .content-box { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 12px; padding: 2rem; backdrop-filter: blur(10px); margin-bottom: 2rem; }
        .content-box h2 { color: #667eea; margin-bottom: 1.5rem; font-size: 1.2rem; }
        .file-content { background: rgba(0, 0, 0, 0.3); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 1.5rem; overflow-x: auto; overflow-y: scroll; min-height: 400px; max-height: 70vh; }
        .file-content pre { margin: 0; font-family: 'Courier New', monospace; font-size: 0.9rem; line-height: 1.6; color: #e0e0e0; white-space: pre-wrap; word-wrap: break-word; }
        .actions { display: flex; gap: 1rem; justify-content: center; margin-top: 2rem; flex-wrap: wrap; }
        .btn { padding: 1rem 2rem; border: none; border-radius: 8px; font-size: 1rem; font-weight: 600; cursor: pointer; transition: all 0.3s ease; text-decoration: none; display: inline-block; }
        .btn-primary { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; }
        .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(102, 126, 234, 0.4); }
        .btn-danger { background: linear-gradient(135deg, #f44336 0%, #d32f2f 100%); color: white; }
        .btn-danger:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(244, 67, 54, 0.4); }
        .btn-secondary { background: rgba(255, 255, 255, 0.05); color: #e0e0e0; border: 1px solid rgba(255, 255, 255, 0.2); }
        .btn-secondary:hover { background: rgba(255, 255, 255, 0.1); border-color: rgba(255, 255, 255, 0.3); }
        footer { text-align: center; margin-top: 4rem; padding: 1.5rem; color: #666; border-top: 1px solid rgba(255, 255, 255, 0.1); }
        @media (max-width: 768px) { .file-info { flex-direction: column; gap: 0.5rem; } .actions { flex-direction: column; } .btn { width: 100%; } }
    </style>
</head>
<body>
    <div class="bubbles">
        <div class="bubble"></div>
        <div class="bubble"></div>
        <div class="bubble"></div>
        <div class="bubble"></div>
        <div class="bubble"></div>
        <div class="bubble"></div>
        <div class="bubble"></div>
    </div>
    <div class="container">
        <header>
            <div class="file-icon">{{FILE_ICON}}</div>
            <h1>{{FILENAME}}</h1>
            <div class="file-info">
                <div>Size: <span>{{FILE_SIZE}}</span></div>
                <div>Type: <span>{{FILE_TYPE}}</span></div>
                <div>Modified: <span>{{MODIFIED_DATE}}</span></div>
            </div>
        </header>
        <div class="content-box">
            <h2>File Content</h2>
            <div class="file-content">
                <pre>{{FILE_CONTENT}}</pre>
            </div>
        </div>
        <div class="actions">
            <button id="downloadBtn" class="btn btn-primary">Download</button>
            <button id="deleteBtn" class="btn btn-danger">Delete</button>
            <a href="{{BACK_URL}}" class="btn btn-secondary">Back to Directory</a>
        </div>
        <footer>
            <p>WebServ File Viewer • HTTP/1.0</p>
            <p style="margin-top: 0.5rem; font-size: 0.9rem;">Powered by caffeine and determination</p>
        </footer>
    </div>
    <script>
        document.getElementById('downloadBtn').addEventListener('click', downloadFile);
        document.getElementById('deleteBtn').addEventListener('click', deleteFile);
        
        function downloadFile() {
            fetch(window.location.href, {
                method: 'GET',
                headers: { 'Must-Download': '1' }
            })
            .then(r => r.blob())
            .then(blob => {
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = '{{FILENAME}}';
                document.body.appendChild(a);
                a.click();
                window.URL.revokeObjectURL(url);
                document.body.removeChild(a);
            });
        }
        
        function deleteFile() {
            if (confirm('Are you sure you want to delete this file?')) {
                fetch(window.location.href, {
                    method: 'DELETE'
                })
                .then(r => {
                    if (r.ok) {
                        alert('File deleted successfully');
                        window.location.href = '{{BACK_URL}}';
                    } else if (r.status === 405) {
                        alert('Error: DELETE method not allowed for this location');
                    } else if (r.status === 403) {
                        alert('Error: Permission denied');
                    } else if (r.status === 404) {
                        alert('Error: File not found');
                    } else {
                        alert('Error: Failed to delete file (Status: ' + r.status + ')');
                    }
                })
                .catch(err => {
                    alert('Error: Network error occurred');
                    console.error(err);
                });
            }
        }
    </script>
</body>
</html>)";
}

std::string readMediaViewerTemplate()
{
    return R"(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>{{FILENAME}} - WebServ</title><style>* { margin: 0; padding: 0; box-sizing: border-box; } body { background: #0a0a0a; color: #e0e0e0; min-height: 100vh; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; overflow-x: hidden; } .bubbles { position: fixed; top: 0; left: 0; width: 100%; height: 100%; z-index: 0; overflow: hidden; pointer-events: none; } .bubble { position: absolute; bottom: -100px; background: radial-gradient(circle, rgba(102, 126, 234, 0.3), rgba(118, 75, 162, 0.2)); border-radius: 50%; opacity: 0.6; animation: rise 15s infinite ease-in; } .bubble:nth-child(1) { width: 80px; height: 80px; left: 10%; animation-duration: 12s; } .bubble:nth-child(2) { width: 60px; height: 60px; left: 20%; animation-duration: 14s; animation-delay: 2s; } .bubble:nth-child(3) { width: 100px; height: 100px; left: 35%; animation-duration: 16s; animation-delay: 4s; } .bubble:nth-child(4) { width: 70px; height: 70px; left: 50%; animation-duration: 13s; animation-delay: 1s; } .bubble:nth-child(5) { width: 90px; height: 90px; left: 65%; animation-duration: 15s; animation-delay: 3s; } .bubble:nth-child(6) { width: 120px; height: 120px; left: 75%; animation-duration: 17s; animation-delay: 5s; } .bubble:nth-child(7) { width: 75px; height: 75px; left: 85%; animation-duration: 14s; animation-delay: 2.5s; } @keyframes rise { 0% { bottom: -100px; transform: translateX(0); opacity: 0.6; } 50% { transform: translateX(100px); opacity: 0.8; } 100% { bottom: 110%; transform: translateX(-50px); opacity: 0; } } .container { position: relative; z-index: 1; max-width: 1200px; margin: 0 auto; padding: 3rem 2rem; min-height: 100vh; } header { text-align: center; margin-bottom: 2rem; padding: 2rem; backdrop-filter: blur(10px); } h1 { font-size: 2rem; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text; margin-bottom: 0.5rem; word-break: break-all; } .file-info { display: flex; justify-content: center; gap: 2rem; margin-top: 1rem; font-size: 0.9rem; color: #888; } .file-info span { color: #667eea; font-weight: 600; } .media-box { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 12px; padding: 2rem; backdrop-filter: blur(10px); margin-bottom: 2rem; display: flex; justify-content: center; align-items: center; min-height: 500px; } .media-box img { max-width: 100%; max-height: 70vh; border-radius: 8px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); } .media-box video { max-width: 100%; max-height: 70vh; border-radius: 8px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); } .media-box audio { width: 100%; max-width: 600px; } .actions { display: flex; gap: 1rem; justify-content: center; margin-top: 2rem; flex-wrap: wrap; } .btn { padding: 1rem 2rem; border: none; border-radius: 8px; font-size: 1rem; font-weight: 600; cursor: pointer; transition: all 0.3s ease; text-decoration: none; display: inline-block; } .btn-primary { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; } .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(102, 126, 234, 0.4); } .btn-danger { background: linear-gradient(135deg, #f44336 0%, #d32f2f 100%); color: white; } .btn-danger:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(244, 67, 54, 0.4); } .btn-secondary { background: rgba(255, 255, 255, 0.05); color: #e0e0e0; border: 1px solid rgba(255, 255, 255, 0.2); } .btn-secondary:hover { background: rgba(255, 255, 255, 0.1); border-color: rgba(255, 255, 255, 0.3); } footer { text-align: center; margin-top: 4rem; padding: 1.5rem; color: #666; border-top: 1px solid rgba(255, 255, 255, 0.1); } @media (max-width: 768px) { .file-info { flex-direction: column; gap: 0.5rem; } .actions { flex-direction: column; } .btn { width: 100%; } }</style></head>
<body><div class="bubbles"><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div><div class="bubble"></div></div><div class="container"><header><h1>{{FILENAME}}</h1><div class="file-info"><div>Size: <span>{{FILE_SIZE}}</span></div><div>Type: <span>{{FILE_TYPE}}</span></div></div></header><div class="media-box">{{MEDIA_CONTENT}}</div><div class="actions"><button id="downloadBtn" class="btn btn-primary">Download</button><button id="deleteBtn" class="btn btn-danger">Delete</button><a href="{{BACK_URL}}" class="btn btn-secondary">Back to Directory</a></div><footer><p>WebServ Media Viewer • HTTP/1.0</p><p style="margin-top: 0.5rem; font-size: 0.9rem;">Powered by caffeine and determination</p></footer></div><script>document.getElementById('downloadBtn').addEventListener('click',downloadFile);document.getElementById('deleteBtn').addEventListener('click',deleteFile);function downloadFile(){fetch(window.location.href,{method:'GET',headers:{'Must-Download':'1'}}).then(r=>r.blob()).then(blob=>{const url=window.URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download='{{FILENAME}}';document.body.appendChild(a);a.click();window.URL.revokeObjectURL(url);document.body.removeChild(a);});}function deleteFile(){if(confirm('Are you sure you want to delete this file?')){fetch(window.location.href,{method:'DELETE'}).then(r=>{if(r.ok){alert('File deleted successfully');window.location.href='{{BACK_URL}}';}else if(r.status===405){alert('Error: DELETE method not allowed for this location');}else if(r.status===403){alert('Error: Permission denied');}else if(r.status===404){alert('Error: File not found');}else{alert('Error: Failed to delete file (Status: '+r.status+')');}}).catch(err=>{alert('Error: Network error occurred');console.error(err);});}}</script></body>
</html>)";
}

bool checkLocationRestrict(std::string str, Server server)
{
    int pos = str.find_last_of("/");
    if (pos != std::string::npos)
    {
        std::string tmp = str.substr(0, pos);
        Location* loc = findLocation(server, tmp);
        if (loc)
            if (loc->getValue("allowed_methods").find("GET") != std::string::npos)
                return (true);
    }
    return (false);
}

class CgiEnv
{
    private:
        std::vector<std::string> env_strings;
        std::vector<char*> env_ptrs;
        
    public:
        CgiEnv() {};
        ~CgiEnv() {};
        
        void setEnv(const std::string& key, const  std::string& value)
        {
            if (key.empty() || value.empty())
                return;
            
            std::string env_var = key + "=" + value;
            env_strings.push_back(env_var);
        }
        char** getEnvp()
        {
            env_ptrs.clear();
            for (auto& s : env_strings)
                env_ptrs.push_back(const_cast<char*>(s.c_str()));
            env_ptrs.push_back(NULL);

            return (env_ptrs.data());
        }
        void setEnvInt(const char* key, int value)
        {
            setEnv(key, std::to_string(value));
        }
};

std::string startCgiRequest(httpClientRequest *req, Server server,
        clientData *data, Location* loc, std::map<int, clientData*>& clientDataMap,
        int epoll_fd)
{
    int     pid;
    int     pipeFd[2];
    CgiEnv  Env;

    if (pipe(pipeFd) == -1)
        return (sendErrorCode(500, server, data));
    pid = fork();
    if (pid < 0)
        return (close(pipeFd[0]), close(pipeFd[1]), sendErrorCode(500, server, data));
    if (pid == 0)
    {
        close(pipeFd[0]);
        if (dup2(pipeFd[1], STDOUT_FILENO) < 0)
            return (close(pipeFd[1]), "-1");
        close(pipeFd[1]);

        Env.setEnv("REQUEST_METHOD", req->_method);
        Env.setEnv("CONTENT_TYPE", req->_contentType.empty() ? "" : req->_contentType);
        Env.setEnv("CONTENT_LENGTH", req->_contentLength.empty() ? "0" : req->_contentLength);

        // Server information
        Env.setEnv("SERVER_NAME", server._serverName);
        Env.setEnvInt("SERVER_PORT", data->serverPort);
        Env.setEnv("SERVER_PROTOCOL", "HTTP/1.0");
        Env.setEnv("SERVER_SOFTWARE", "WebServ/1.0");
        Env.setEnv("GATEWAY_INTERFACE", "CGI/1.1");

        // Request information
        Env.setEnv("REQUEST_URI", req->_path);
        Env.setEnv("PATH_INFO", req->_path);
        Env.setEnv("REMOTE_ADDR", data->clientIp);
        Env.setEnvInt("REMOTE_PORT", data->clientPort);
        Env.setEnvInt("REDIRECT_STATUS", 200);

        // HTTP headers
        Env.setEnv("HTTP_HOST", req->_host);
        if (!req->_userAgent.empty())
            Env.setEnv("HTTP_USER_AGENT", req->_userAgent);
        if (!req->_accept.empty())
            Env.setEnv("HTTP_ACCEPT", req->_accept);
        if (!req->_referer.empty())
            Env.setEnv("HTTP_REFERER", req->_referer);
        if (!req->_cookie.empty())
            Env.setEnv("HTTP_COOKIE", req->_cookie);

        std::string tmp = req->_path;
        std::string ext = tmp.substr(tmp.find_last_of("."));
        tmp = tmp.substr(0, tmp.find_last_of("/"));
        std::string op = loc->getValue(ext);
        Location *locTmp = findLocation(server, tmp);
        std::string root = locTmp->getValue("root");

        if (root.empty())
            root = server._serverRoot;
        
        std::string fullPath = root.substr(1) + req->_path.substr(req->_path.find_last_of("/"));
        Env.setEnv("SCRIPT_NAME", fullPath);
        Env.setEnv("SCRIPT_FILENAME", fullPath);

        char *argv[] =
        {
            (char *)op.c_str(),
            (char *)(fullPath).c_str(),
            NULL
        };

        execve(argv[0], argv, Env.getEnvp());
        return "";
    }
    else
    {
        close(pipeFd[1]);
        data->cgiFd = pipeFd[0];
        data->cgiPid = pid;
        data->state = WAITING_CGI_OUTPUT;

        clientDataMap[data->cgiFd] = data;

        struct epoll_event cgi_event;
        cgi_event.events = EPOLLIN;
        cgi_event.data.fd = data->cgiFd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data->cgiFd, &cgi_event);

        return "";
    }
}

std::string getHandle(httpClientRequest *req, Server server,
        std::map<int, clientData*>& clientDataMap, clientData *data, int epoll_fd)
{
    if ((long)std::time(NULL) - (long)data->requestStartTime >= (long)server._reqTimeout.num)
       return (sendErrorCode(408, server, data));

    std::string response;
    int     flag = 0;
    bool    isFile = 1;
    bool    isForb = 1;
    
    if (req->_path == "/favicon.ico" && !flag)
    {
        response = "HTTP/1.0 200 OK\r\n";
        response += "Content-Type: image/x-icon\r\n";
        response += "Content-Length: " + std::to_string(ico_ico_len) + "\r\n";
        response += "Connection: close\r\n\r\n";
        
        response.append(reinterpret_cast<const char*>(ico_ico), ico_ico_len);
        flag = 1;
        return (response);
    }
    
    std::string fullPath = req->_path;
    int tmp = fullPath.find_last_of("/");
    if (tmp != std::string::npos)
    {
        std::string pathPart = fullPath.substr(0, tmp);
        Location* tempLoc = findLocation(server, pathPart);
        if (tempLoc)
        {
            std::string root = tempLoc->getValue("root");
            fullPath.replace(fullPath.find(pathPart), pathPart.length(), root);
        }
    }
    
    if (fullPath[0] == '/' && fullPath[1])
        fullPath = fullPath.substr(1);

    if (access(fullPath.c_str(), F_OK) < 0)
        isFile = 0;
    if (access(fullPath.c_str(), R_OK) < 0)
        isForb = 0;
    
    Location* loc = findLocation(server, req->_path);

    if (!loc)
    {
        struct stat path_stat;
        if (stat(fullPath.c_str(), &path_stat) == 0)
            if (S_ISDIR(path_stat.st_mode))
                return (sendErrorCode(403, server, data));
        if (!isFile)
            return (sendErrorCode(404, server, data));
        if (!isForb)
            return (sendErrorCode(403, server, data));
        if (!checkLocationRestrict(req->_path, server))
            return (sendErrorCode(405, server, data));
        if (isFile && isForb)
        {
            bool isHtmlFile = (fullPath.substr(fullPath.find_last_of(".") + 1) == "html");
            bool isImageFile = (getContentType(fullPath).find("image/") == 0);
            bool isVideoFile = (getContentType(fullPath).find("video/") == 0);
            bool isAudioFile = (getContentType(fullPath).find("audio/") == 0);
            bool isDownload = req->_mustDownload;
            std::ifstream file(fullPath.c_str(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
            file.close();

            if (isDownload)
            {
                size_t lastSlash = fullPath.find_last_of('/');
                std::string filename = (lastSlash != std::string::npos) 
                    ? fullPath.substr(lastSlash + 1) 
                    : fullPath;
                
                std::string response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n";
                response += "Content-Type: application/octet-stream\r\n";
                response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
                response += "Connection: close\r\n\r\n";
                response += content;
                
                return response;
            }
            else if (isHtmlFile)
            {
                std::string response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: text/html; charset=utf-8\r\n";
                response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
                response += "Connection: close\r\n\r\n";
                response += content;
                
                return response;
            }
            else if (isImageFile || isVideoFile || isAudioFile)
            {
                std::string contentType = getContentType(fullPath);
                
                bool isDirectRequest = true;
                if (!req->_referer.empty() && req->_referer.find(req->_path) != std::string::npos)
                    isDirectRequest = false;
                
                if (isDirectRequest)
                {
                    std::string html = readMediaViewerTemplate();
                    
                    size_t lastSlash = fullPath.find_last_of('/');
                    std::string filename = (lastSlash != std::string::npos)
                        ? fullPath.substr(lastSlash + 1)
                        : fullPath;
                    
                    struct stat fileStat;
                    stat(fullPath.c_str(), &fileStat);
                    
                    std::string mediaContent;
                    if (isImageFile)
                        mediaContent = "<img src=\"" + req->_path + "\" alt=\"" + filename + "\">";
                    else if (isVideoFile)
                        mediaContent = "<video controls><source src=\"" + req->_path + "\" type=\"" + contentType + "\"></video>";
                    else if (isAudioFile)
                        mediaContent = "<audio controls><source src=\"" + req->_path + "\" type=\"" + contentType + "\"></audio>";
                    
                    std::string backUrl = req->_path;
                    size_t lastSlashPos = backUrl.find_last_of('/');
                    if (lastSlashPos != std::string::npos)
                        backUrl = backUrl.substr(0, lastSlashPos);
                    if (backUrl.empty()) backUrl = "/";
                    
                    html = replaceAll(html, "{{FILENAME}}", filename);
                    html = replaceAll(html, "{{FILE_SIZE}}", formatFileSize(fileStat.st_size));
                    html = replaceAll(html, "{{FILE_TYPE}}", contentType);
                    html = replaceAll(html, "{{MEDIA_CONTENT}}", mediaContent);
                    html = replaceAll(html, "{{BACK_URL}}", backUrl);
                    
                    std::string response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html; charset=utf-8\r\n";
                    response += "Content-Length: " + std::to_string(html.length()) + "\r\n";
                    response += "Connection: close\r\n\r\n";
                    response += html;
                    
                    return response;
                }
                else
                {
                    std::string response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: " + contentType + "\r\n";
                    response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
                    response += "Connection: close\r\n\r\n";
                    response += content;
                    
                    return response;
                }
            }
            else if (isValidCgiRequest(req, server) && data->state != WAITING_CGI_OUTPUT)
            {
                std::string tmp = req->_path;

                tmp = tmp.substr(0, tmp.find_last_of("/"));
                loc = findLocation(server, tmp);
                return (startCgiRequest(req, server, data, loc, clientDataMap, epoll_fd));
            }
            else
            {
                std::string html = readFileViewerTemplate();
                
                size_t lastSlash = fullPath.find_last_of('/');
                std::string filename = (lastSlash != std::string::npos) 
                    ? fullPath.substr(lastSlash + 1) 
                    : fullPath;
                
                struct stat fileStat;
                stat(fullPath.c_str(), &fileStat);
                
                html = replaceAll(html, "{{FILENAME}}", filename);
                html = replaceAll(html, "{{FILE_ICON}}", getFileIcon(filename, false));
                html = replaceAll(html, "{{FILE_SIZE}}", formatFileSize(fileStat.st_size));
                html = replaceAll(html, "{{FILE_TYPE}}", getContentType(fullPath));
                html = replaceAll(html, "{{MODIFIED_DATE}}", formatDateTime(fileStat.st_mtime));
                html = replaceAll(html, "{{FILE_CONTENT}}", content);
                
                std::string backUrl = req->_path;
                size_t lastSlashPos = backUrl.find_last_of('/');
                if (lastSlashPos != std::string::npos)
                    backUrl = backUrl.substr(0, lastSlashPos);
                if (backUrl.empty())
                    backUrl = "/";
                
                html = replaceAll(html, "{{BACK_URL}}", backUrl);
                
                std::string response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: text/html; charset=utf-8\r\n";
                response += "Content-Length: " + std::to_string(html.length()) + "\r\n";
                response += "Connection: close\r\n\r\n";
                response += html;
                
                return response;
            }
        }
    }

    if (loc->getValue("allowed_methods").find("GET") == std::string::npos)
        return (sendErrorCode(405, server, data));

    if (loc->is_redirect)
    {
        response = "HTTP/1.0 " + std::to_string(loc->redirect_code) + " Found\r\n";
        response += "Location: " + loc->redirect_url + "\r\n";
        response += "Connection: close\r\n\r\n";
        return (response);
    }

    std::string root = loc->getValue("root");
    if (root.empty())
        root = server._serverRoot;

    std::string index = loc->getValue("index");
    if (index.empty())
        return (autoIndex(loc, server, data, root.substr(1) + "/" + index, req));
    
    std::string file_path = root.substr(1) + "/" + index;
    return (serveFile(file_path, req, data, server, *loc));
}

std::string postHandle(httpClientRequest *req, Server server, clientData *data)
{
    Location* loc = findLocation(server, req->_path);
    
    if (!loc)
        return (sendErrorCode(404, server, data));
    if (loc->getValue("allowed_methods").find("POST") == std::string::npos)
        return (sendErrorCode(405, server, data));
    std::cout << loc->is_upload << std::endl;
    if (loc->is_upload)
    {
        std::string filename = generateFilename(req);
        std::string file_content = extractFileContent(req->_body);
        
        if (saveFile(filename, file_content, server, loc))
            return (sendErrorCode(201, server, data));
        else
            return (sendErrorCode(500,server, data));
    }
    else
        return (sendErrorCode(403,server, data));
}

std::string deleteHandle(httpClientRequest *req, Server server, clientData *data)
{
    std::string fullPath = req->_path;
    int tmp = fullPath.find_last_of("/");
    std::string pathPart = fullPath.substr(0, tmp);
    Location* tempLoc = findLocation(server, pathPart);

    if (!tempLoc)
        return (sendErrorCode(404,server, data));

    if (tempLoc->getValue("allowed_methods").find("DELETE") == std::string::npos)
        return (sendErrorCode(405, server, data));

    std::string root = tempLoc->getValue("root");
    fullPath.replace(fullPath.find(pathPart), pathPart.length(), root);
    
    if (std::remove((fullPath.substr(1)).c_str()) != 0)
        return (sendErrorCode(500, server, data));
    return (sendErrorCode(200, server, data));
}

void    isChunkedOrContent(clientData *cData)
{
    cData->ccFlag = 0;
    size_t header_end = cData->request_data.find("\r\n\r\n");
    if (header_end != std::string::npos && !cData->headers_complete)
    {
        cData->headers_complete = true;
        std::string headers = cData->request_data.substr(0, header_end);
        
        size_t te_pos = headers.find("Transfer-Encoding: chunked");
        if (te_pos != std::string::npos)
        cData->is_chunked = 1;
        
        size_t cl_pos = headers.find("Content-Length: ");
        if (cl_pos != std::string::npos)
        {
            size_t cl_end = headers.find("\r\n", cl_pos);
            cl_pos += 16;
            std::string cl_str = headers.substr(cl_pos, cl_end - cl_pos);
            cData->content_length = atoi(cl_str.c_str());
        }
    }
}

void    createServerBlocks(std::string &config, std::vector<Server> *servers)
{
    config.erase(0, config.find("server {"));
    while (!config.empty())
    {
        std::string tmp;
        
        size_t next_server = config.find("server {", 8);
        if (next_server == std::string::npos)
        {
            tmp = config.substr(8);
            config.clear();
        }
        else
        {
            tmp = config.substr(8, next_server - 8);
            config.erase(0, next_server);
        }
        if (!tmp.empty() && tmp.back() == '}')
            tmp.pop_back();
            
        Server newServer;
        while (!tmp.empty())
        {
            newServer.getConfigLine(tmp);
            size_t start = tmp.find_first_not_of(" \t\n");
            if (start != std::string::npos)
                tmp = tmp.substr(start);
            else
                tmp.clear();
        }
        servers->push_back(newServer);
    }

}

ServerSocket* findServerSocket(std::vector<ServerSocket>& serverSockets, int fd)
{
    for (int i = 0; i < serverSockets.size(); i++)
        if (serverSockets[i].sockfd == fd)
            return (&serverSockets[i]);
    return (NULL);
}

unsigned long convertIPtoLong(const std::string& ip_str)
{
    if (ip_str.empty())
        return htonl(INADDR_ANY);
    
    if (ip_str == "localhost" || ip_str == "127.0.0.1")
        return htonl(INADDR_LOOPBACK);

    if (ip_str == "any" || ip_str == "0.0.0.0")
        return htonl(INADDR_ANY);
    
    unsigned char ip_parts[4] = {0, 0, 0, 0};
    int part_index = 0;
    int current_number = 0;
    bool has_digit = false;
    
    for (size_t i = 0; i <= ip_str.length(); i++)
    {
        char c = (i < ip_str.length()) ? ip_str[i] : '.'; // Treat end as final dot
        
        if (c >= '0' && c <= '9')
        {
            current_number = current_number * 10 + (c - '0');
            has_digit = true;
            if (current_number > 255)
            {
                std::cerr << "Invalid IP address: " << ip_str << " (number > 255)" << std::endl;
                return htonl(INADDR_ANY);
            }
        }
        else if (c == '.' || i == ip_str.length())
        {
            if (!has_digit)
            {
                std::cerr << "Invalid IP address: " << ip_str << " (empty part)" << std::endl;
                return htonl(INADDR_ANY);
            }
            
            if (part_index >= 4)
            {
                std::cerr << "Invalid IP address: " << ip_str << " (too many parts)" << std::endl;
                return htonl(INADDR_ANY);
            }
            
            ip_parts[part_index] = (unsigned char)current_number;
            part_index++;
            current_number = 0;
            has_digit = false;
        }
        else
        {
            std::cerr << "Invalid IP address: " << ip_str << " (invalid character)" << std::endl;
            return htonl(INADDR_ANY);
        }
    }
    
    if (part_index != 4)
    {
        std::cerr << "Invalid IP address: " << ip_str << " (wrong number of parts)" << std::endl;
        return htonl(INADDR_ANY);
    }
    
    unsigned long ip_long = (ip_parts[0] << 24) | (ip_parts[1] << 16) | (ip_parts[2] << 8) | ip_parts[3];
    return htonl(ip_long);
}

std::string convertLongToIP(unsigned long ip_long)
{
    unsigned char *ip_bytes = (unsigned char*)&ip_long;
    
    std::string ip_str = std::to_string(ip_bytes[0]) + "." +
                         std::to_string(ip_bytes[1]) + "." +
                         std::to_string(ip_bytes[2]) + "." +
                         std::to_string(ip_bytes[3]);
    
    return ip_str;
}

void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) // sigterm can be deleated
        std::cout << "\nCleaning and exiting..." << std::endl;
}

void setup_signals()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);    
    signal(SIGPIPE, SIG_IGN);
}

void    finalizeReq(Server server, clientData *cData, int cliefd,
            int epoll_fd, std::map<int, clientData*>& clientDataMap)
{
    if ((long)std::time(NULL) - (long)cData->requestStartTime >= (long)server._reqTimeout.num)
    {
        cData->response_data = sendErrorCode(408, server, cData);
        cData->state = SENDING_RESPONSE;
        
        struct epoll_event modify_event;
        modify_event.events = EPOLLOUT;
        modify_event.data.fd = cliefd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cliefd, &modify_event) == -1)
        {
            perror("epoll_ctl MOD to EPOLLOUT");
            cleanupClient(epoll_fd, cData, clientDataMap);
        }
        return;
    }
    else
    {
        std::string fileName = cData->filename;
        std::ifstream requestFile(fileName, std::ios::binary);
        if (requestFile)
        {
            std::stringstream buffer;
            buffer << requestFile.rdbuf();
            cData->request_data = buffer.str();
            requestFile.close();
            std::remove(fileName.c_str());
        }

        httpClientRequest* req = new httpClientRequest();
        req->parseRequest(cData->request_data);

        if (req->_method == "GET")
            cData->response_data = getHandle(req, server, clientDataMap, cData, epoll_fd);
        else if (req->_method == "POST")
            cData->response_data = postHandle(req, server, cData);
        else if (req->_method == "DELETE")
            cData->response_data = deleteHandle(req, server, cData);
        else
            cData->response_data = sendErrorCode(405, server, cData);

        cData->state = SENDING_RESPONSE;
        delete req;

        struct epoll_event modify_event;
        modify_event.events = EPOLLOUT;
        modify_event.data.fd = cliefd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cliefd, &modify_event) == -1)
        {
            perror("epoll_ctl MOD to EPOLLOUT");
            cleanupClient(epoll_fd, cData, clientDataMap);
        }
    }
    return;
}

int main(int ac, char **av)
{
    static bool _status = 1;
    if (ac != 2)
    {
        printf("Usage: ./webserv <configuration file> \n");
        return (1);
    }
    setup_signals();

    std::fstream    inFile;
    std::string     line;
    std::string     config;
    std::vector<Server> servers;

    inFile.open(av[1]);
    if (!inFile.is_open())
    {
        std::cout << "error file opening" << std::endl;
        return (1);
    }
    else
    {
        while (std::getline(inFile, line))
        {
            size_t first = line.find_first_not_of(" \t");
            size_t last = line.find_last_not_of(" \t");
            
            if (first != std::string::npos && last != std::string::npos)
                config += line.substr(first, last - first + 1);
            
        }
        inFile.close();
        createServerBlocks(config, &servers);
        for (size_t i = 0; i < servers.size(); i++)
            printServerConfig(servers[i], i + 1);
    }

    printInstructions();
    int         exitStatus = 0;
    int         cliefd = -1;
    sockaddr_in clie_add;

    std::vector<ServerSocket> serverSockets;

    int epoll_fd = epoll_create(true);
    if (epoll_fd == -1)
    {
        perror("epoll_create()");
        return(1);
    }

    for (size_t i = 0; i < servers.size(); i++)
    {
        for (size_t j = 0; j < servers[i]._serverPort.size(); j++)
        {
            int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
            if (sockfd < 0)
            {
                perror("Socket() Creation");
                continue;
            }

            int opt = true;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                    &opt, sizeof(opt)) < 0)
            {
                perror("setsockopt()");
                close(sockfd);
                continue;
            }

            sockaddr_in                 serv_add;
            memset(&serv_add, 0, sizeof(serv_add));
            serv_add.sin_addr.s_addr = convertIPtoLong(servers[i]._serverIp);
            serv_add.sin_port = htons(servers[i]._serverPort[j]);
            serv_add.sin_family = AF_INET;

            if (bind(sockfd, (const struct sockaddr *)&serv_add, sizeof(serv_add)) < 0)
            {
                perror("bind()");
                close(sockfd);
                continue;
            }

            if (listen(sockfd, SOMAXCONN) < 0)
            {
                perror("listen()");
                close(sockfd);
                continue;;
            }

            struct epoll_event l_event;
            l_event.events = EPOLLIN;
            l_event.data.fd = sockfd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &l_event) == -1)
            {
                close(sockfd);
                close(epoll_fd);
                perror("Server epoll_ctl()");
                continue;
            }
            serverSockets.push_back(ServerSocket(sockfd, servers[i]._serverPort[j], i));
        }
    }

    if (serverSockets.empty())
    {
        std::cerr << "No server sockets created successfully" << std::endl;
        close(epoll_fd);
        return (1);
    }

    std::map<int, clientData*> clientDataMap;
    struct epoll_event events[50];

    while (1)
    {
        int readyEvents = epoll_wait(epoll_fd, events, 100, -1);
        if (readyEvents == -1)
        {
            if (errno == EINTR)
                break;
            close(epoll_fd);
            perror("epoll_wait");
            exitStatus = 1;
            break;
        }

        for (int i = 0; i < readyEvents; i++)
        {
            int eventFd = events[i].data.fd;
            ServerSocket* serverSock = findServerSocket(serverSockets, eventFd);
            if (serverSock != NULL)                     // new_connection
            {
                socklen_t clie_len = sizeof(clie_add);
                int cliefd = accept(eventFd, (struct sockaddr*)&clie_add, &clie_len);
                
                if (cliefd < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        perror("Client accept()");
                    continue;
                }

                clientData *data = new clientData();
                data->fd = cliefd;
                data->serverIndex = serverSock->serIndex;
                data->serverPort = serverSock->port;
                
                struct sockaddr_in *client_addr = (struct sockaddr_in*)&clie_add;
                data->clientIp = convertLongToIP(client_addr->sin_addr.s_addr);
                data->clientPort = ntohs(client_addr->sin_port);
                data->serverIp = servers[data->serverIndex]._serverIp;

                struct epoll_event client_event;
                client_event.events = EPOLLIN;
                client_event.data.fd = cliefd;
                clientDataMap[cliefd] = data;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cliefd, &client_event) == -1)
                {
                    close(cliefd);
                    delete data;
                    clientDataMap.erase(cliefd);
                    perror("Client epoll_ctl");
                    continue;
                }
                std::cout << "New client on server " << servers[serverSock->serIndex]._serverIp << " port " << serverSock->port << std::endl;
            }
            else
            {
                int cliefd = events[i].data.fd;
                clientData *cData = clientDataMap[cliefd];
                
                if (!cData)
                {
                    printf("No client data for fd %d\n", cliefd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cliefd, NULL);
                    close(cliefd);
                    continue;
                }

                if (cData->cgiFd > 0 && cData->cgiFd == cliefd
                        && events[i].events & (EPOLLIN | EPOLLHUP))
                    {
                        char buffer[4096];
                        ssize_t bytes = read(cliefd, buffer, sizeof(buffer));
                        
                        if (bytes > 0)
                            cData->CgiBody += std::string(buffer, bytes);
                        if (bytes == 0 || (events[i].events & EPOLLHUP))
                        {                            
                            int status;
                            pid_t result = waitpid(cData->cgiPid, &status, WNOHANG);
                            
                            if (result == cData->cgiPid && WIFEXITED(status) && WEXITSTATUS(status) == 0) 
                            {
                                cData->response_data = "HTTP/1.1 200 OK\r\n";
                                cData->response_data += "Content-type: text/html; charset=utf-8\r\n";
                                cData->response_data += "Content-Length: " + std::to_string(cData->CgiBody.length()) + "\r\n";
                                cData->response_data += "Connection: close\r\n\r\n";
                                cData->response_data += cData->CgiBody;
                                cData->state = SENDING_RESPONSE;
                                
                                struct epoll_event client_event;
                                client_event.events = EPOLLOUT;
                                client_event.data.fd = cData->fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cData->fd, &client_event);
                            }
                            else 
                            {
                                cData->response_data = sendErrorCode(500, servers[cData->serverIndex], cData);
                                cData->state = SENDING_RESPONSE;
                                
                                struct epoll_event client_event;
                                client_event.events = EPOLLOUT;
                                client_event.data.fd = cData->fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cData->fd, &client_event);
                            }
                            
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiFd, NULL);
                            clientDataMap.erase(cData->cgiFd);
                            close(cData->cgiFd);
                            cData->cgiFd = -1;
                            cData->cgiPid = -1;
                        }
                        else if (bytes < 0) 
                        {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiFd, NULL);
                            clientDataMap.erase(cData->cgiFd);
                            close(cData->cgiFd);
                            cData->cgiFd = -1;
                        }
                        continue;
                    }

                if (cData->requestStartTime == 0)
                    cData->requestStartTime = static_cast<long>(std::time(nullptr));

                if (events[i].events & EPOLLIN)
                {
                    char rBuffer[8196];
                    int rBuffer_size = sizeof(rBuffer);
                    memset(rBuffer, 0, rBuffer_size);
                    int readed = read(cliefd, rBuffer, rBuffer_size - 1);
                    
                    if (readed == 0)
                    {
                        printf("client %d: has been disconnected\n", cliefd);
                        cleanupClient(epoll_fd, cData, clientDataMap);
                        continue;
                    }
                    else if (readed == -1)
                    {
                        perror("Client read()");
                        cleanupClient(epoll_fd, cData, clientDataMap);
                        continue;
                    }
                    else
                    {
                        cData->request_data.append(rBuffer, readed);
                        if (cData->ccFlag)
                            isChunkedOrContent(cData);
                        
                        if (cData->headers_complete || cData->ccFlag == 0)
                        {
                            if (cData->is_chunked)              // waitChunkedBody()
                            {
                                size_t header_end = cData->request_data.find("\r\n\r\n");

                                if (header_end == std::string::npos && !cData->flag)
                                    continue;
                                else if (header_end != std::string::npos && !cData->flag)
                                {
                                    cData->flag = 1;
                                    cData->headerStr = cData->request_data.substr(0, header_end + 4);
                                    cData->request_data.erase(0, header_end + 4);

                                    std::string fileStr = servers[cData->serverIndex]._serverRoot.substr(1) + "/swp/.chunked_" + to_string(chIndex++) + ".swp";
                                    int cfd = open(fileStr.c_str(), O_RDWR | O_APPEND | O_CREAT, 0644);
                                    if (cfd < 0)
                                    {
                                        perror("file open()");
                                        cleanupClient(epoll_fd, cData, clientDataMap);
                                        std::remove(fileStr.c_str());
                                        continue;
                                    }
                                    ssize_t bytes_written = write(cfd, cData->headerStr.c_str(), cData->headerStr.size());
                                    if (bytes_written < 0)
                                    {
                                        perror("file write()");
                                        cleanupClient(epoll_fd, cData, clientDataMap);
                                        continue;
                                    }
                                    close(cfd);
                                    cData->filename = fileStr;
                                }

                                int is_complete = handleChunkedRequest(cData, servers[cData->serverIndex]);
                                if (!is_complete)
                                    continue;
                                else if (is_complete == 2)
                                {
                                    cleanupClient(epoll_fd, cData, clientDataMap);
                                    continue;
                                }
                                else
                                    cData->body_complete = true;
                            }
                            else 
                            {
                                size_t header_end = cData->request_data.find("\r\n\r\n");

                                if (header_end == std::string::npos && !cData->flag)
                                    continue;
                                else if (header_end != std::string::npos && !cData->flag)
                                {
                                    cData->flag = 1;
                                    cData->headerStr = cData->request_data.substr(0, header_end + 4);
                                    cData->request_data.erase(0, header_end + 4);
                                }

                                int is_complete = handleContentLengthRequest(cData, servers[cData->serverIndex]);
                                if (!is_complete)
                                    continue;
                                else if (is_complete == 2)
                                {
                                    cleanupClient(epoll_fd, cData, clientDataMap);
                                    continue;
                                }
                                else
                                    cData->body_complete = true;
                            }
                        }
                        
                        if (cData->body_complete && cData->ccFlag == 0)
                            finalizeReq(servers[cData->serverIndex], cData, cliefd, epoll_fd, clientDataMap);
                    }
                }
                
                else if (events[i].events & EPOLLOUT)
                {
                    if (cData->state == SENDING_RESPONSE && !cData->response_data.empty())
                    {
                        size_t remaining = cData->response_data.length() - cData->bytes_sent;
                        int bytes_sent = write(cliefd, 
                                              cData->response_data.c_str() + cData->bytes_sent, 
                                              remaining);
                        
                        if (bytes_sent == -1)
                        {
                            perror("write()");
                            cleanupClient(epoll_fd, cData, clientDataMap);
                            continue;
                        }
                        
                        cData->bytes_sent += bytes_sent;
                        
                        if (cData->bytes_sent >= cData->response_data.length())
                            cleanupClient(epoll_fd, cData, clientDataMap);
                    }
                }
                else if (events[i].events & (EPOLLHUP | EPOLLERR)) 
                {                    
                    if (cData->cgiFd > 0 && cliefd == cData->cgiFd) 
                    {
                        if (cData->cgiFd != -1)
                        {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiFd, NULL);
                            clientDataMap.erase(cData->cgiFd);
                            close(cData->cgiFd);
                            cData->cgiFd = -1;
                        }
                        continue;
                    }
                    else if (cliefd == cData->fd) 
                    {
                        cleanupClient(epoll_fd, cData, clientDataMap);
                        continue;
                    }
                    continue;
                }
            }
        }
    }
    
    std::map<int, clientData*>::iterator it = clientDataMap.begin();
    while (it != clientDataMap.end())
    {
        std::map<int, clientData*>::iterator current = it;
        ++it;
        cleanupClient(epoll_fd, current->second, clientDataMap);
    }
    
    for (auto& ss : serverSockets)
        close(ss.sockfd);
    close(epoll_fd);
    
    return (exitStatus);
}
