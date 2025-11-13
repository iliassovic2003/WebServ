#include "WebServer.h"

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
    else if (ext == ".php")
        return "🐘";
    else if (ext == ".pl")
        return "🐪";
    else if (ext == ".rb")
        return "💎";
    else if (ext == ".js")
        return "📜";
    else if (ext == ".sh")
        return "🐚";
    else if (ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".hpp")
        return "⚙️";
    
    return "📄";
}
