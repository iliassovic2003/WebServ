#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

class JSON {
public:
    static std::string escape(const std::string& str)
    {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i)
        {
            char c = str[i];
            switch (c)
            {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    result += c;
            }
        }
        return result;
    }

    static std::string unescape(const std::string& str)
    {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i)
        {
            if (str[i] == '\\' && i + 1 < str.length())
            {
                char next = str[i + 1];
                switch (next)
                {
                    case '"':  result += '"'; i++; break;
                    case '\\': result += '\\'; i++; break;
                    case 'b':  result += '\b'; i++; break;
                    case 'f':  result += '\f'; i++; break;
                    case 'n':  result += '\n'; i++; break;
                    case 'r':  result += '\r'; i++; break;
                    case 't':  result += '\t'; i++; break;
                    default: result += str[i];
                }
            } else
                result += str[i];
        }
        return result;
    }

    static std::string getCurrentTimestamp()
    {
        time_t now = time(0);
        struct tm* timeinfo = gmtime(&now);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
        return std::string(buffer);
    }

    static std::map<std::string, std::string> parseSimpleJSON(const std::string& json)
    {
        std::map<std::string, std::string> result;
        size_t pos = 0;
        
        while (pos < json.length())
        {
            size_t keyStart = json.find('"', pos);
            if (keyStart == std::string::npos)
                break;
            keyStart++;
            
            size_t keyEnd = json.find('"', keyStart);
            if (keyEnd == std::string::npos)
                break;
            
            std::string key = json.substr(keyStart, keyEnd - keyStart);
            
            size_t colonPos = json.find(':', keyEnd);
            if (colonPos == std::string::npos)
                break;
            
            size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
            if (valueStart == std::string::npos)
                break;
            
            std::string value;
            if (json[valueStart] == '"')
            {
                valueStart++;
                size_t valueEnd = valueStart;
                while (valueEnd < json.length())
                {
                    if (json[valueEnd] == '"' && json[valueEnd - 1] != '\\') break;
                    valueEnd++;
                }
                value = json.substr(valueStart, valueEnd - valueStart);
                value = unescape(value);
                pos = valueEnd + 1;
            }
            else {
                size_t valueEnd = json.find_first_of(",}", valueStart);
                value = json.substr(valueStart, valueEnd - valueStart);
                pos = valueEnd;
            }
            result[key] = value;
        }
        
        return result;
    }
};

class FileSystem {
public:
    static bool fileExists(const std::string& path)
    {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    static bool createDirectory(const std::string& path)
    {
        if (fileExists(path))
            return true;
        return mkdir(path.c_str(), 0755) == 0;
    }

    static std::string readFile(const std::string& path)
    {
        std::ifstream file(path.c_str());
        if (!file.is_open())
            return "";
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static bool writeFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(path.c_str());
        if (!file.is_open())
            return false;
        file << content;
        file.close();
        return true;
    }

    static std::string getExecutablePath()
    {
        char path[1024];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) {
            path[len] = '\0';
            std::string fullPath(path);
            size_t lastSlash = fullPath.find_last_of('/');
            if (lastSlash != std::string::npos)
                return fullPath.substr(0, lastSlash);
        }
        return ".";
    }
};

struct Message {
    std::string user;
    std::string ai;
    std::string timestamp;
};

struct SessionData {
    std::string sessionId;
    std::string createdAt;
    std::string updatedAt;
    std::string expiresAt;
    std::vector<Message> messages;
    int messageCount;
};

class SessionManager {
private:
    std::string sessionsDir;

    std::string getSessionFilePath(const std::string& sessionId) {
        return sessionsDir + "/." + sessionId + ".json";
    }

    std::string serializeSession(const SessionData& session)
    {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"session_id\": \"" << JSON::escape(session.sessionId) << "\",\n";
        ss << "  \"created_at\": \"" << JSON::escape(session.createdAt) << "\",\n";
        ss << "  \"updated_at\": \"" << JSON::escape(session.updatedAt) << "\",\n";
        ss << "  \"expires_at\": " << (session.expiresAt.empty() ? "null" : "\"" + JSON::escape(session.expiresAt) + "\"") << ",\n";
        ss << "  \"message_count\": " << session.messageCount << ",\n";
        ss << "  \"messages\": [\n";
        
        for (size_t i = 0; i < session.messages.size(); ++i) {
            ss << "    {\n";
            ss << "      \"user\": \"" << JSON::escape(session.messages[i].user) << "\",\n";
            ss << "      \"ai\": \"" << JSON::escape(session.messages[i].ai) << "\",\n";
            ss << "      \"timestamp\": \"" << JSON::escape(session.messages[i].timestamp) << "\"\n";
            ss << "    }" << (i < session.messages.size() - 1 ? "," : "") << "\n";
        }
        
        ss << "  ]\n";
        ss << "}\n";
        return ss.str();
    }

    SessionData parseSession(const std::string& json)
    {
        SessionData session;
        session.messageCount = 0;
        
        std::map<std::string, std::string> parsed = JSON::parseSimpleJSON(json);
        session.sessionId = parsed["session_id"];
        session.createdAt = parsed["created_at"];
        session.updatedAt = parsed["updated_at"];
        session.expiresAt = parsed["expires_at"];
        
        if (!parsed["message_count"].empty())
            session.messageCount = atoi(parsed["message_count"].c_str());
        
        size_t msgStart = json.find("\"messages\"");
        if (msgStart != std::string::npos) 
        {
            size_t arrayStart = json.find('[', msgStart);
            size_t arrayEnd = json.find(']', arrayStart);
            
            if (arrayStart != std::string::npos && arrayEnd != std::string::npos)
            {
                std::string messagesJson = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                
                size_t pos = 0;
                while (pos < messagesJson.length())
                {
                    size_t objStart = messagesJson.find('{', pos);
                    if (objStart == std::string::npos)
                        break;
                    
                    size_t objEnd = messagesJson.find('}', objStart);
                    if (objEnd == std::string::npos)
                        break;
                    
                    std::string msgJson = messagesJson.substr(objStart, objEnd - objStart + 1);
                    std::map<std::string, std::string> msgParsed = JSON::parseSimpleJSON(msgJson);
                    
                    Message msg;
                    msg.user = msgParsed["user"];
                    msg.ai = msgParsed["ai"];
                    msg.timestamp = msgParsed["timestamp"];
                    session.messages.push_back(msg);
                    
                    pos = objEnd + 1;
                }
            }
        }
        
        return session;
    }

public:
    SessionManager(const std::string& dir) : sessionsDir(dir)
    {
        if (!FileSystem::createDirectory(sessionsDir))
            std::cerr << "Failed to create sessions directory: " << sessionsDir << std::endl;
    }

    SessionData loadSession(const std::string& sessionId)
    {
        std::string filePath = getSessionFilePath(sessionId);
        
        if (FileSystem::fileExists(filePath))
        {
            std::string content = FileSystem::readFile(filePath);
            if (!content.empty())
                return parseSession(content);
        }
        
        SessionData empty;
        empty.messageCount = 0;
        return empty;
    }

    bool saveSession(const SessionData& session)
    {
        std::string filePath = getSessionFilePath(session.sessionId);
        std::string json = serializeSession(session);
        return FileSystem::writeFile(filePath, json);
    }

    SessionData createNewSession(const std::string& sessionId, const std::string& expiryDate = "")
    {
        SessionData session;
        session.sessionId = sessionId;
        session.createdAt = JSON::getCurrentTimestamp();
        session.updatedAt = JSON::getCurrentTimestamp();
        session.expiresAt = expiryDate;
        session.messageCount = 0;
        
        saveSession(session);
        return session;
    }
};

class CGI {
public:
    static void sendResponse(const std::string& json)
    {
        std::cout << json << std::endl;
    }

    static void sendError(const std::string& message)
    {
        std::cout << "{\"error\": \"" << JSON::escape(message) << "\"}" << std::endl;
    }

    static std::string getPostData()
    {
        const char* contentLengthStr = getenv("CONTENT_LENGTH");
        if (!contentLengthStr)
            return "";
        
        int contentLength = atoi(contentLengthStr);
        if (contentLength <= 0)
            return "";
        
        char* buffer = new char[contentLength + 1];
        std::cin.read(buffer, contentLength);
        buffer[contentLength] = '\0';
        
        std::string result(buffer);
        delete[] buffer;

        return result;
    }
};

static std::string getInputData(int argc, char** argv)
{
    std::string postData;
    
    if (argc > 1)
        postData = argv[1];
    else
        postData = CGI::getPostData();
    
    return postData;
}

int main(int argc, char** argv)
{    
    try {
        std::string scriptDir = FileSystem::getExecutablePath();
        std::string sessionsDir = scriptDir + "/chat_sessions";
        SessionManager manager(sessionsDir);
        
        std::string postData = getInputData(argc, argv);
        
        if (postData.empty())
        {
            CGI::sendError("No POST data received");
            return 1;
        }
        
        std::map<std::string, std::string> request = JSON::parseSimpleJSON(postData);
        std::string sessionId = request["session_id"];
        std::string action = request["action"];
        if (action.empty()) action = "chat";
        
        if (sessionId.empty() || sessionId.length() < 5)
        {
            CGI::sendError("Invalid session ID");
            return 1;
        }
        
        SessionData session = manager.loadSession(sessionId);
        if (session.sessionId.empty())
            session = manager.createNewSession(sessionId, request["expiry_date"]);
        
        if (action == "save_message")
        {
            std::string userMsg = request["user_message"];
            std::string aiMsg = request["ai_response"];
            
            if (!userMsg.empty() && !aiMsg.empty())
            {
                Message msg;
                msg.user = userMsg;
                msg.ai = aiMsg;
                msg.timestamp = JSON::getCurrentTimestamp();
                
                session.messages.push_back(msg);
                session.messageCount++;
                session.updatedAt = JSON::getCurrentTimestamp();
                
                if (!request["expiry_date"].empty())
                    session.expiresAt = request["expiry_date"];
                
                if (!manager.saveSession(session))
                {
                    CGI::sendError("Failed to save message");
                    return 1;
                }
            }
            
            std::stringstream response;
            response << "{\n";
            response << "  \"status\": \"saved\",\n";
            response << "  \"session_id\": \"" << JSON::escape(session.sessionId) << "\",\n";
            response << "  \"message_count\": " << session.messageCount << "\n";
            response << "}";
            CGI::sendResponse(response.str());
            
        }
        else if (action == "load_history")
        {
            size_t startIdx = session.messages.size() > 10 ? session.messages.size() - 10 : 0;
            
            std::stringstream response;
            response << "{\n";
            response << "  \"session_id\": \"" << JSON::escape(session.sessionId) << "\",\n";
            response << "  \"message_count\": " << session.messageCount << ",\n";
            response << "  \"chat_history\": [\n";
            
            for (size_t i = startIdx; i < session.messages.size(); ++i)
            {
                response << "    {\n";
                response << "      \"user\": \"" << JSON::escape(session.messages[i].user) << "\",\n";
                response << "      \"ai\": \"" << JSON::escape(session.messages[i].ai) << "\",\n";
                response << "  \"timestamp\": \"" << JSON::escape(session.messages[i].timestamp) << "\"\n";
                response << "    }" << (i < session.messages.size() - 1 ? "," : "") << "\n";
            }
            
            response << "  ]\n";
            response << "}";
            CGI::sendResponse(response.str());
            
        }
        else {
            std::string userMsg = request["message"];
            std::string aiResponse;
            
            if (!userMsg.empty())
            {
                aiResponse = "Session response to: '" + userMsg + "'";
                
                Message msg;
                msg.user = userMsg;
                msg.ai = aiResponse;
                msg.timestamp = JSON::getCurrentTimestamp();
                
                session.messages.push_back(msg);
                session.messageCount++;
                session.updatedAt = JSON::getCurrentTimestamp();
                
                if (!manager.saveSession(session)) 
                {
                    CGI::sendError("Failed to save session");
                    return 1;
                }
            }
            else
                aiResponse = "Welcome!";
            
            size_t startIdx = session.messages.size() > 10 ? session.messages.size() - 10 : 0;
            
            std::stringstream response;
            response << "{\n";
            response << "  \"response\": \"" << JSON::escape(aiResponse) << "\",\n";
            response << "  \"session_id\": \"" << JSON::escape(session.sessionId) << "\",\n";
            response << "  \"message_count\": " << session.messageCount << ",\n";
            response << "  \"chat_history\": [\n";
            
            for (size_t i = startIdx; i < session.messages.size(); ++i) {
                response << "    {\n";
                response << "      \"user\": \"" << JSON::escape(session.messages[i].user) << "\",\n";
                response << "      \"ai\": \"" << JSON::escape(session.messages[i].ai) << "\",\n";
                response << "      \"timestamp\": \"" << JSON::escape(session.messages[i].timestamp) << "\"\n";
                response << "    }" << (i < session.messages.size() - 1 ? "," : "") << "\n";
            }
            
            response << "  ]\n";
            response << "}";
            CGI::sendResponse(response.str());
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        CGI::sendError(std::string("Internal server error: ") + e.what());
        return 1;
    } catch (...) {
        CGI::sendError("Unknown internal server error");
        return 1;
    }
}