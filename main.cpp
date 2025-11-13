#include "WebServer.h"

static bool isValid(char *str)
{
    if (!str || strlen(str) == 0)
        return false;

    size_t str_len = strlen(str);
    if (str_len < 5)
        return false;

    return (strcmp(str + str_len - 5, ".conf") == 0);
}

static void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
        std::cout << "\nCleaning and exiting..." << std::endl;
}

static void setup_signals()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
}

int main(int ac, char **av)
{
    if (ac != 2 || (ac > 1 && !isValid(av[1])))
    {
        printf("Usage: ./webserv <configuration file>.conf \n");
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
        std::map<int, clientData*>::iterator timeout_it = clientDataMap.begin();
        while (timeout_it != clientDataMap.end())
        {
            clientData* cData = timeout_it->second;

            if (cData->state == SENDING_RESPONSE && cData->cgiFd > 0 && cData->cgiPid > 0)
            {
                long current_time = static_cast<long>(std::time(NULL));
                long elapsed = current_time - cData->cgiStartTime;
                
                if (elapsed >= static_cast<long>(servers[cData->serverIndex]._cgiTimeout.num))
                {
                    if (cData->cgiPid > 0)
                    {
                        kill(cData->cgiPid, SIGKILL);
                        waitpid(cData->cgiPid, NULL, 0);
                        cData->cgiPid = -1;
                    }
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
                    }
                    
                    cData->response_data = sendErrorCode(504, servers[cData->serverIndex], cData);
                    cData->state = SENDING_RESPONSE;
                    
                    struct epoll_event client_event;
                    client_event.events = EPOLLOUT;
                    client_event.data.fd = cData->fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cData->fd, &client_event);
                }
            }
            ++timeout_it;
        }
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

                if (cData->cgiInputFd > 0 && cData->cgiInputFd == cliefd 
                    && events[i].events & EPOLLOUT)
                {
                    if (!cData->cgiInputDone && !cData->cgiInputData.empty())
                    {
                        ssize_t written = write(cData->cgiInputFd, 
                                            cData->cgiInputData.c_str(), 
                                            cData->cgiInputData.length());
                        
                        if (written > 0)
                        {
                            cData->cgiInputData.erase(0, written);
                            
                            if (cData->cgiInputData.empty())
                            {
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiInputFd, NULL);
                                clientDataMap.erase(cData->cgiInputFd);
                                close(cData->cgiInputFd);
                                cData->cgiInputFd = -1;
                                cData->cgiInputDone = true;
                            }
                        }
                        else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                        {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiInputFd, NULL);
                            clientDataMap.erase(cData->cgiInputFd);
                            close(cData->cgiInputFd);
                            cData->cgiInputFd = -1;
                            cData->cgiInputDone = true;
                        }
                    }
                    continue;
                }

                if (cData->cgiFd > 0 && cData->cgiFd == cliefd 
                    && events[i].events & (EPOLLIN | EPOLLHUP))
                {
                    long current_time = static_cast<long>(std::time(NULL));
                    long elapsed = current_time - cData->cgiStartTime;
                    
                    if (elapsed >= static_cast<long>(servers[cData->serverIndex]._cgiTimeout.num))
                    {
                        std::cout << "CGI timeout: " << elapsed << "s (limit: " 
                                << servers[cData->serverIndex]._cgiTimeout.num << "s)" << std::endl;
                        
                        cData->response_data = sendErrorCode(504, servers[cData->serverIndex], cData);
                        cData->state = SENDING_RESPONSE;

                        if (cData->cgiPid > 0)
                        {
                            kill(cData->cgiPid, SIGKILL);
                            waitpid(cData->cgiPid, NULL, 0);
                            cData->cgiPid = -1;
                        }
                        
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiFd, NULL);
                        clientDataMap.erase(cData->cgiFd);
                        close(cData->cgiFd);
                        cData->cgiFd = -1;
                        
                        struct epoll_event client_event;
                        client_event.events = EPOLLOUT;
                        client_event.data.fd = cData->fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cData->fd, &client_event);
                        
                        continue;
                    }

                    char buffer[4096];
                    ssize_t bytes = read(cliefd, buffer, sizeof(buffer));
                                        
                    if (bytes > 0)
                        cData->CgiBody += std::string(buffer, bytes);
                    
                    if (bytes == 0)  
                    {
                        if (cData->cgiInputFd > 0)
                        {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiInputFd, NULL);
                            clientDataMap.erase(cData->cgiInputFd);
                            close(cData->cgiInputFd);
                            cData->cgiInputFd = -1;
                        }

                        int status;
                        pid_t result = waitpid(cData->cgiPid, &status, WNOHANG);
                        
                        
                        if (result == cData->cgiPid) 
                        {
                            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
                            {
                                if (!cData->cgiTypeRequest)
                                {
                                    cData->response_data = "HTTP/1.1 200 OK\r\n";
                                    cData->response_data += "Content-Length: " + to_string(cData->CgiBody.length()) + "\r\n";
                                    cData->response_data += "Connection: close\r\n\r\n";
                                }
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
                        }
                        else
                        {
                            if (result == 0)
                            {
                                usleep(100000);
                                result = waitpid(cData->cgiPid, &status, WNOHANG);
                            }
                            
                            if (result == cData->cgiPid && WIFEXITED(status) && WEXITSTATUS(status) == 0)
                            {
                                cData->response_data = "HTTP/1.1 200 OK\r\n";
                                cData->response_data += "Content-type: text/html; charset=utf-8\r\n";
                                cData->response_data += "Content-Length: " + to_string(cData->CgiBody.length()) + "\r\n";
                                cData->response_data += "Connection: close\r\n\r\n";
                                cData->response_data += cData->CgiBody;
                                cData->state = SENDING_RESPONSE;
                            }
                            else
                            {
                                kill(cData->cgiPid, SIGKILL);
                                waitpid(cData->cgiPid, &status, 0);
                                cData->response_data = sendErrorCode(500, servers[cData->serverIndex], cData);
                                cData->state = SENDING_RESPONSE;
                            }
                        }
                        
                        struct epoll_event client_event;
                        client_event.events = EPOLLOUT;
                        client_event.data.fd = cData->fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cData->fd, &client_event);
                        
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cData->cgiFd, NULL);
                        clientDataMap.erase(cData->cgiFd);
                        close(cData->cgiFd);
                        cData->cgiFd = -1;
                        cData->cgiPid = -1;
                    }
                    continue;
                }

                if (cData->requestStartTime == 0)
                    cData->requestStartTime = static_cast<long>(std::time(NULL));

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
    
    for (std::vector<ServerSocket>::iterator it = serverSockets.begin(); it != serverSockets.end(); ++it)
        close(it->sockfd);
    close(epoll_fd);
    
    return (exitStatus);
}
