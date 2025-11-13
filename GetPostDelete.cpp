#include "WebServer.h"

static bool checkLocationRestrict(std::string str, Server server)
{
    size_t pos = str.find_last_of("/");
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

std::string getHandle(httpClientRequest *req, Server server,
        std::map<int, clientData*>& clientDataMap, clientData *data, int epoll_fd)
{
    if ((long)std::time(NULL) - (long)data->requestStartTime >= (long)server._reqTimeout.num)
        return (std::remove(data->filename.c_str()), sendErrorCode(408, server, data));

    std::string response;
    int         flag = 0;
    bool        isFile = 1;
    bool        isForb = 1;

    
    if (req->_path == "/favicon.ico" && !flag)
    {
        response = "HTTP/1.0 200 OK\r\n";
        response += "Content-Type: image/x-icon\r\n";
        response += "Content-Length: " + to_string(ico_ico_len) + "\r\n";
        response += "Connection: close\r\n\r\n";
        
        response.append(reinterpret_cast<const char*>(ico_ico), ico_ico_len);
        flag = 1;
        return (response);
    }
    
    std::string fullPath = req->_path;
    size_t tmp = fullPath.find_last_of("/");
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
                response += "Content-Length: " + to_string(content.length()) + "\r\n";
                response += "Connection: close\r\n\r\n";
                response += content;
                
                return response;
            }
            else if (isHtmlFile)
            {
                std::string response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: text/html; charset=utf-8\r\n";
                response += "Content-Length: " + to_string(content.length()) + "\r\n";
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
                    response += "Content-Length: " + to_string(html.length()) + "\r\n";
                    response += "Connection: close\r\n\r\n";
                    response += html;
                    
                    return response;
                }
                else
                {
                    std::string response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: " + contentType + "\r\n";
                    response += "Content-Length: " + to_string(content.length()) + "\r\n";
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
                response += "Content-Length: " + to_string(html.length()) + "\r\n";
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
        response = "HTTP/1.0 " + to_string(loc->redirect_code) + " Found\r\n";
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
    return (serveFile(file_path, data, server, *loc));
}

static std::string extractFileContent(const std::string& body)
{
    size_t headers_end = body.find("\r\n\r\n");
    if (headers_end == std::string::npos)
        return body;
    
    return (body.substr(headers_end + 4));
}

static std::string checkFilename(const std::string& filename)
{
    std::string safe_name;
    const std::string forbidden_chars = "\\/:*?\"<>|";
    
    for (size_t i = 0; i < filename.length(); ++i)
    {
        char c = filename[i];
        if (forbidden_chars.find(c) == std::string::npos)
            safe_name += c;
    }
    
    return safe_name;
}

static bool saveFile(const std::string& filename,
            const std::string& content, Location *loc)
{
    std::string safe_name = checkFilename(filename);
    std::string half_name = loc->getValue("upload_store");
    std::string full_name = half_name.substr(1) + "/" + safe_name;

    std::ofstream file(full_name.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

static std::string getCurrentTimestamp()
{
    char buffer[20];
    std::time_t now = std::time(NULL);

    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", std::localtime(&now));
    return (std::string(buffer));
}

static std::string generateFilename(httpClientRequest* req)
{
    std::string extension = getFileExtensionFromContentType(req->_contentType);

    if (!req->_filename.empty())
        return (checkFilename(req->_filename) + extension);
    return ("u_" + getCurrentTimestamp() + extension);
}

std::string postHandle(httpClientRequest *req, Server server, std::map<int, clientData*>& clientDataMap,
        clientData *data, int epoll_fd)
{
    if (isValidCgiRequest(req, server))
    {
        std::string tmp = req->_path;
        tmp = tmp.substr(0, tmp.find_last_of("/"));
        Location* cgiLoc = findLocation(server, tmp);

        if (cgiLoc && cgiLoc->is_cgi)
        {
            data->cgiTypeRequest = 1;
            return (startCgiRequest(req, server, data, cgiLoc, clientDataMap, epoll_fd));
        }
    }

    Location* loc = findLocation(server, req->_path);
    
    if (!loc)
        return (sendErrorCode(404, server, data));
    if (loc->getValue("allowed_methods").find("POST") == std::string::npos)
        return (sendErrorCode(405, server, data));
    if (loc->is_upload)
    {
        std::string filename = generateFilename(req);
        std::string file_content = extractFileContent(req->_body);
        
        if (saveFile(filename, file_content, loc))
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
    size_t tmp = fullPath.find_last_of("/");
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

void    finalizeReq(Server server, clientData *cData, int cliefd,
            int epoll_fd, std::map<int, clientData*>& clientDataMap)
{
    if ((long)std::time(NULL) - (long)cData->requestStartTime >= (long)server._reqTimeout.num)
    {
        std::remove(cData->filename.c_str());
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
        std::ifstream requestFile(fileName.c_str(), std::ios::binary);
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
            cData->response_data = postHandle(req, server, clientDataMap, cData, epoll_fd);
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