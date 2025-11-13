#include "WebServer.h"

static size_t hexToDecimal(const std::string& hexStr)
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
    if ((size_t)cData->totalbytes == cData->content_length)
        return (1);
    return (0);
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
