#include "WebServer.h"

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

Location* findLocation(Server& server, const std::string& path)
{
    for (size_t i = 0; i < server._locations.size(); i++)
        if (path == server._locations[i].url)
            return &server._locations[i];
    return (NULL);
}

void cleanupClient(int epoll_fd, clientData *cData, std::map<int, clientData*> &clientDataMap)
{
    if (!cData)
        return;

    if (cData->cgiInputFd > 0) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiInputFd, NULL);
        clientDataMap.erase(cData->cgiInputFd);
        close(cData->cgiInputFd);
        cData->cgiInputFd = -1;
    }

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
        if (!tmp.empty() && tmp[tmp.length() - 1] == '}')
            tmp.erase(tmp.length() - 1);
            
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
    for (size_t i = 0; i < serverSockets.size(); i++)
        if (serverSockets[i].sockfd == fd)
            return (&serverSockets[i]);
    return (NULL);
}
