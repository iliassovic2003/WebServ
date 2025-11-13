#include "WebServer.h"


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
        char c = (i < ip_str.length()) ? ip_str[i] : '.';
        
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
    
    std::string ip_str = to_string(ip_bytes[0]) + "." +
                         to_string(ip_bytes[1]) + "." +
                         to_string(ip_bytes[2]) + "." +
                         to_string(ip_bytes[3]);
    
    return ip_str;
}
