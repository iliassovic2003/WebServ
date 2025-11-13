#include "WebServer.h"

std::string startCgiRequest(httpClientRequest *req, Server server,
        clientData *data, Location* loc, std::map<int, clientData*>& clientDataMap,
        int epoll_fd)
{
    int     pid;
    int     outputPipe[2];
    int     inputPipe[2];
    CgiEnv  Env;

    if (pipe(outputPipe) == -1)
        return (sendErrorCode(500, server, data));
    if (pipe(inputPipe) == -1)
    {
        close(outputPipe[0]);
        close(outputPipe[1]);
        return (sendErrorCode(500, server, data));
    }
    pid = fork();
    if (pid < 0)
        return (close(outputPipe[0]), close(outputPipe[1]), close(inputPipe[0]), close(inputPipe[1]), sendErrorCode(500, server, data));
    if (pid == 0)
    {
        close(inputPipe[1]);
        if (dup2(inputPipe[0], STDIN_FILENO) < 0)
            return (close(inputPipe[0]), close(outputPipe[0]), close(outputPipe[1]), "-1");
        close(inputPipe[0]);

        close(outputPipe[0]);
        if (dup2(outputPipe[1], STDOUT_FILENO) < 0)
            return (close(outputPipe[1]), "-1");
        close(outputPipe[1]);

        Env.setEnv("REQUEST_METHOD", req->_method);
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
        close(outputPipe[1]);
        close(inputPipe[0]);

        data->cgiFd = outputPipe[0];
        data->cgiInputFd = inputPipe[1];
        data->cgiPid = pid;
        data->cgiStartTime = static_cast<long>(std::time(NULL));
        data->state = WAITING_CGI_OUTPUT;

        if (req->_method == "POST" && !req->_body.empty())
        {
            const char* body_data = req->_body.c_str();
            size_t total_written = 0;
            size_t body_size = req->_body.length();
            
            while (total_written < body_size)
            {
                ssize_t written = write(data->cgiInputFd, 
                                       body_data + total_written, 
                                       body_size - total_written);
                
                if (written < 0)
                    break;
                total_written += written;
            }
        }

        close(data->cgiInputFd);
        data->cgiInputFd = -1;
        data->cgiInputDone = true;

        clientDataMap[data->cgiFd] = data;
        struct epoll_event cgi_event;
        cgi_event.events = EPOLLIN;
        cgi_event.data.fd = data->cgiFd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data->cgiFd, &cgi_event);

        return "";
    }
}

bool isValidCgiRequest(httpClientRequest* req, Server server)
{
    std::string path = req->_path;

    size_t pos = path.find_last_of(".");
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
