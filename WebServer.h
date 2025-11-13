#ifndef WEBSERVER_H
# define WEBSERVER_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <map>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <dirent.h>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>
#include <sstream>
#include "favicon.h"

static int chIndex = 0;
static int clIndex = 0;

class CgiEnv;
class clientData;
class httpClientRequest;
class Location;
class Server;

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
    uint64_t num;  // Changed from uint64_t for C++98
};

class errorPage
{
    public:
        int         typeCode;
        std::string link;

        void    assign(int code, std::string lk);
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


        Server() : _serverPort(0), _errorIndex(0),
                    _locIndex(0), _serverIp("127.0.0.0")
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
        int         cgiInputFd;
        bool        cgiInputDone;
        std::string cgiInputData;
        long        cgiStartTime;
        bool        cgiTypeRequest;
        uint64_t    totalSize;
        bool        headers_complete;
        bool        body_complete;
        int         totalbytes;
        long        requestStartTime;
        std::string CgiBody;

        clientData() : fd(-1), cgiPid(-1), cgiFd(-1), fileFd(-1), serverIndex(0),
                      serverPort(0), serverIp(""), clientIp(""), clientPort(0),
                      state(READING_HEADERS), request_data(""), response_data(""), headerStr(""), 
                      unchunkBody(""), filename(""), content_length(0), bytes_sent(0), flag(0), 
                      ccFlag(1), posOffset(0), is_chunked(0), cgiInputFd(-1), cgiInputDone(false), 
                      cgiInputData(""), cgiStartTime(0), cgiTypeRequest(0), totalSize(0), 
                      headers_complete(false), body_complete(false), totalbytes(0), 
                      requestStartTime(0), CgiBody("") {}
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
        std::string _cookie;
        std::string _contentType;
        std::string _contentLength;
        std::string _filename;
        std::string _body;
        bool        _mustDownload;
        bool        _conFlag;
        bool        _finishFlag;

        httpClientRequest() : _method(""), _path(""), _httpVersion(""), _host(""), 
                            _connection(""), _userAgent(""), _accept(""), _acceptLanguage(""),
                            _acceptEncoding(""), _authorization(""), _referer(""), _cookie(""),
                            _contentType(""), _contentLength(""), _filename(""), _body(""),
                            _mustDownload(0), _conFlag(0), _finishFlag(0) {};
        httpClientRequest*  parseRequest(std::string& request);
        void                parseRequestV2(std::string& line);
        ~httpClientRequest() {};
};


std::string to_string(int i);
class CgiEnv
{
    private:
        std::vector<std::string> env_strings;
        std::vector<char*> env_ptrs;
        
    public:
        CgiEnv() {};
        ~CgiEnv() {};
        
        void setEnv(const std::string& key, const  std::string& value) {
            if (key.empty() || value.empty())
                return;
            
            std::string env_var = key + "=" + value;
            env_strings.push_back(env_var);
        }
        
        char** getEnvp() {
            env_ptrs.clear();
            for (std::vector<std::string>::iterator it = env_strings.begin(); it != env_strings.end(); ++it)
                env_ptrs.push_back(const_cast<char*>(it->c_str()));
            env_ptrs.push_back(NULL);

            return (&env_ptrs[0]);
        }
        
        void setEnvInt(const std::string& key, int value) {
            setEnv(key, to_string(value));
        }
};

int handleChunkedRequest(clientData* cData, Server& server);
int handleContentLengthRequest(clientData* cData, Server& server);
Location* findLocation(Server& server, const std::string& path);
bool isValidCgiRequest(httpClientRequest* req, Server server);
std::string getFileExtensionFromContentType(const std::string& contentType);
std::string getContentType(const std::string& filename);
void printServerConfig(Server& server, int index);
void printInstructions();
void cleanupClient(int epoll_fd, clientData *cData, std::map<int, clientData*> &clientDataMap);
std::string replacePlaceholders(std::string content, Server& server, clientData *data, Location loc);
std::string serveFile(const std::string& filePath, clientData *data, Server& server, Location loc);
std::string formatFileSize(off_t size);
std::string formatDateTime(time_t time);
std::string getFileIcon(const std::string& filename, bool isDirectory);
std::string readAutoIndexTemplate();
std::string replaceAll(std::string str, const std::string& from, const std::string& to);
std::string autoIndex(Location* loc, Server server, clientData *data, std::string url, httpClientRequest *req);
std::string readFileViewerTemplate();
std::string readMediaViewerTemplate();
std::string startCgiRequest(httpClientRequest *req, Server server, clientData *data, Location* loc, std::map<int, clientData*>& clientDataMap, int epoll_fd);
void isChunkedOrContent(clientData *cData);
std::string sendErrorCode(int code, Server server, clientData *data);
void createServerBlocks(std::string &config, std::vector<Server> *servers);
ServerSocket* findServerSocket(std::vector<ServerSocket>& serverSockets, int fd);
unsigned long convertIPtoLong(const std::string& ip_str);
std::string convertLongToIP(unsigned long ip_long);
void    finalizeReq(Server server, clientData *cData, int cliefd, int epoll_fd, std::map<int, clientData*>& clientDataMap);

#endif