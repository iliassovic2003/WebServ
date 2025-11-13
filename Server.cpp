#include "WebServer.h"

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
    size_t semiPos = config.find(";");
    size_t startLocationPos = config.find("location");
    size_t endLocationPos = config.find("}");
    
    if (semiPos != std::string::npos && startLocationPos != 0)
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
        sizeStr = to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    std::string numStr;
    char unit = '\0';
    bool isNegative = false;
    
    if (sizeStr[0] == '-')
    {
        isNegative = true;
        std::cerr << "\033[1;31mNegative Value, Default: " << defaultValue << "S.\033[0m" << std::endl;
        sizeStr = to_string(defaultValue) + "S";
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
            sizeStr = to_string(defaultValue) + "S";
            return (defaultValue);
        }
    }
    
    if (numStr.empty())
    {
        sizeStr = to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    uint64_t value = std::strtoull(numStr.c_str(), NULL, 10);
    
    if (isNegative)
    {
        std::cerr << "\033[1;31mNegative Value, Default: " << defaultValue << "S.\033[0m" << std::endl;
        sizeStr = to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    if (unit == 'M')
        return (value * 60);
    else if (unit == 'S')
        return (value);
    else if (unit != '\0')
    {
        std::cerr << "\033[1;31mNo Unit Detected, Default: " << defaultValue << "S.\033[0m" << std::endl;
        sizeStr = to_string(defaultValue) + "S";
        return (defaultValue);
    }
    
    sizeStr = to_string(defaultValue) + "S";
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
        size_t semiPos = map.find(";");
        if (semiPos == std::string::npos)
            break;
        
        std::string line = map.substr(0, semiPos);
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos)
            line = line.substr(start);
            
        size_t spacePos = line.find(" ");
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
