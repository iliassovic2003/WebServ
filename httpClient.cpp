#include "WebServer.h"

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