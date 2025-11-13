#include "WebServer.h"

std::string serveFile(const std::string& filePath,
        clientData *data, Server& server, Location loc)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    
    if (!file.is_open())
        return (sendErrorCode(404, server, data));
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    content = replacePlaceholders(content, server, data, loc);
    std::string contentType = getContentType(filePath);
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + to_string(content.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;
    
    return (response);
}

std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return (str);
}

std::string replacePlaceholders(std::string content, Server& server,
        clientData *data, Location loc)
{
    size_t pos;
    
    while ((pos = content.find("{{SERVER_NAME}}")) != std::string::npos)
        content.replace(pos, 15, server._serverName);
    
    while ((pos = content.find("{{PORT}}")) != std::string::npos)
        content.replace(pos, 8, to_string(data->serverPort));
    
    while ((pos = content.find("{{SERVER_IP}}")) != std::string::npos)
        content.replace(pos, 13, server._serverIp);
    
    std::string successRedirect = loc.getValue("success_redirect");
    while ((pos = content.find("{{SUCCESS}}")) != std::string::npos)
        content.replace(pos, 11, successRedirect);
    
    std::string failedRedirect = loc.getValue("failed_redirect");
    while ((pos = content.find("{{FAILED}}")) != std::string::npos)
        content.replace(pos, 10, failedRedirect);
    
    std::string maxSize = server._maxBodySize.str;
    while ((pos = content.find("{{MAXSIZE}}")) != std::string::npos)
        content.replace(pos, 11, maxSize);
    
    return content;
}

std::string readFileViewerTemplate()
{
    return "<!DOCTYPE html>"
           "<html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>{{FILENAME}} - WebServ</title><style>* { margin: 0; padding: 0; box-sizing: border-box; } body { background: #0a0a0a; color: #e0e0e0; min-height: 100vh; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; overflow-x: hidden; } .bubbles { position: fixed; top: 0; left: 0; width: 100%; height: 100%; z-index: 0; overflow: hidden; pointer-events: none; } .bubble { position: absolute; bottom: -100px; background: radial-gradient(circle, rgba(102, 126, 234, 0.3), rgba(118, 75, 162, 0.2)); border-radius: 50%; opacity: 0.6; animation: rise 15s infinite ease-in; } .bubble:nth-child(1) { width: 80px; height: 80px; left: 10%; animation-duration: 12s; } .bubble:nth-child(2) { width: 60px; height: 60px; left: 20%; animation-duration: 14s; animation-delay: 2s; } .bubble:nth-child(3) { width: 100px; height: 100px; left: 35%; animation-duration: 16s; animation-delay: 4s; } .bubble:nth-child(4) { width: 70px; height: 70px; left: 50%; animation-duration: 13s; animation-delay: 1s; } .bubble:nth-child(5) { width: 90px; height: 90px; left: 65%; animation-duration: 15s; animation-delay: 3s; } .bubble:nth-child(6) { width: 120px; height: 120px; left: 75%; animation-duration: 17s; animation-delay: 5s; } .bubble:nth-child(7) { width: 75px; height: 75px; left: 85%; animation-duration: 14s; animation-delay: 2.5s; } @keyframes rise { 0% { bottom: -100px; transform: translateX(0); opacity: 0.6; } 50% { transform: translateX(100px); opacity: 0.8; } 100% { bottom: 110%; transform: translateX(-50px); opacity: 0; } } .container { position: relative; z-index: 1; max-width: 1200px; margin: 0 auto; padding: 3rem 2rem; min-height: 100vh; } header { text-align: center; margin-bottom: 2rem; padding: 2rem; backdrop-filter: blur(10px); } h1 { font-size: 2rem; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text; margin-bottom: 0.5rem; word-break: break-all; } .file-info { display: flex; justify-content: center; gap: 2rem; margin-top: 1rem; font-size: 0.9rem; color: #888; } .file-info span { color: #667eea; font-weight: 600; } .media-box { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 12px; padding: 2rem; backdrop-filter: blur(10px); margin-bottom: 2rem; display: flex; justify-content: center; align-items: center; min-height: 500px; } .media-box img { max-width: 100%; max-height: 70vh; border-radius: 8px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); } .media-box video { max-width: 100%; max-height: 70vh; border-radius: 8px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); } .media-box audio { width: 100%; max-width: 600px; } .actions { display: flex; gap: 1rem; justify-content: center; margin-top: 2rem; flex-wrap: wrap; } .btn { padding: 1rem 2rem; border: none; border-radius: 8px; font-size: 1rem; font-weight: 600; cursor: pointer; transition: all 0.3s ease; text-decoration: none; display: inline-block; } .btn-primary { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; } .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(102, 126, 234, 0.4); } .btn-danger { background: linear-gradient(135deg, #f44336 0%, #d32f2f 100%); color: white; } .btn-danger:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(244, 67, 54, 0.4); } .btn-secondary { background: rgba(255, 255, 255, 0.05); color: #e0e0e0; border: 1px solid rgba(255, 255, 255, 0.2); } .btn-secondary:hover { background: rgba(255, 255, 255, 0.1); border-color: rgba(255, 255, 255, 0.3); } footer { text-align: center; margin-top: 4rem; padding: 1.5rem; color: #666; border-top: 1px solid rgba(255, 255, 255, 0.1); } @media (max-width: 768px) { .file-info { flex-direction: column; gap: 0.5rem; } .actions { flex-direction: column; } .btn { width: 100%; } }</style></head>"
           "<body><div class=\"bubbles\"><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div></div><div class=\"container\"><header><h1>{{FILENAME}}</h1><div class=\"file-info\"><div>Size: <span>{{FILE_SIZE}}</span></div><div>Type: <span>{{FILE_TYPE}}</span></div></div></header><div class=\"media-box\">{{MEDIA_CONTENT}}</div><div class=\"actions\"><button id=\"downloadBtn\" class=\"btn btn-primary\">Download</button><button id=\"deleteBtn\" class=\"btn btn-danger\">Delete</button><a href=\"{{BACK_URL}}\" class=\"btn btn-secondary\">Back to Directory</a></div><footer><p>WebServ Media Viewer • HTTP/1.0</p><p style=\"margin-top: 0.5rem; font-size: 0.9rem;\">Powered by caffeine and determination</p></footer></div><script>document.getElementById('downloadBtn').addEventListener('click',downloadFile);document.getElementById('deleteBtn').addEventListener('click',deleteFile);function downloadFile(){fetch(window.location.href,{method:'GET',headers:{'Must-Download':'1'}}).then(r=>r.blob()).then(blob=>{const url=window.URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download='{{FILENAME}}';document.body.appendChild(a);a.click();window.URL.revokeObjectURL(url);document.body.removeChild(a);});}function deleteFile(){if(confirm('Are you sure you want to delete this file?')){fetch(window.location.href,{method:'DELETE'}).then(r=>{if(r.ok){alert('File deleted successfully');window.location.href='{{BACK_URL}}';}else if(r.status===405){alert('Error: DELETE method not allowed for this location');}else if(r.status===403){alert('Error: Permission denied');}else if(r.status===404){alert('Error: File not found');}else{alert('Error: Failed to delete file (Status: '+r.status+')');}}).catch(err=>{alert('Error: Network error occurred');console.error(err);});}}</script></body>"
           "</html>";
}

std::string readMediaViewerTemplate()
{
    return "<!DOCTYPE html>"
           "<html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>{{FILENAME}} - WebServ</title><style>* { margin: 0; padding: 0; box-sizing: border-box; } body { background: #0a0a0a; color: #e0e0e0; min-height: 100vh; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; overflow-x: hidden; } .bubbles { position: fixed; top: 0; left: 0; width: 100%; height: 100%; z-index: 0; overflow: hidden; pointer-events: none; } .bubble { position: absolute; bottom: -100px; background: radial-gradient(circle, rgba(102, 126, 234, 0.3), rgba(118, 75, 162, 0.2)); border-radius: 50%; opacity: 0.6; animation: rise 15s infinite ease-in; } .bubble:nth-child(1) { width: 80px; height: 80px; left: 10%; animation-duration: 12s; } .bubble:nth-child(2) { width: 60px; height: 60px; left: 20%; animation-duration: 14s; animation-delay: 2s; } .bubble:nth-child(3) { width: 100px; height: 100px; left: 35%; animation-duration: 16s; animation-delay: 4s; } .bubble:nth-child(4) { width: 70px; height: 70px; left: 50%; animation-duration: 13s; animation-delay: 1s; } .bubble:nth-child(5) { width: 90px; height: 90px; left: 65%; animation-duration: 15s; animation-delay: 3s; } .bubble:nth-child(6) { width: 120px; height: 120px; left: 75%; animation-duration: 17s; animation-delay: 5s; } .bubble:nth-child(7) { width: 75px; height: 75px; left: 85%; animation-duration: 14s; animation-delay: 2.5s; } @keyframes rise { 0% { bottom: -100px; transform: translateX(0); opacity: 0.6; } 50% { transform: translateX(100px); opacity: 0.8; } 100% { bottom: 110%; transform: translateX(-50px); opacity: 0; } } .container { position: relative; z-index: 1; max-width: 1200px; margin: 0 auto; padding: 3rem 2rem; min-height: 100vh; } header { text-align: center; margin-bottom: 2rem; padding: 2rem; backdrop-filter: blur(10px); } h1 { font-size: 2rem; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text; margin-bottom: 0.5rem; word-break: break-all; } .file-info { display: flex; justify-content: center; gap: 2rem; margin-top: 1rem; font-size: 0.9rem; color: #888; } .file-info span { color: #667eea; font-weight: 600; } .media-box { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 12px; padding: 2rem; backdrop-filter: blur(10px); margin-bottom: 2rem; display: flex; justify-content: center; align-items: center; min-height: 500px; } .media-box img { max-width: 100%; max-height: 70vh; border-radius: 8px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); } .media-box video { max-width: 100%; max-height: 70vh; border-radius: 8px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); } .media-box audio { width: 100%; max-width: 600px; } .actions { display: flex; gap: 1rem; justify-content: center; margin-top: 2rem; flex-wrap: wrap; } .btn { padding: 1rem 2rem; border: none; border-radius: 8px; font-size: 1rem; font-weight: 600; cursor: pointer; transition: all 0.3s ease; text-decoration: none; display: inline-block; } .btn-primary { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; } .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(102, 126, 234, 0.4); } .btn-danger { background: linear-gradient(135deg, #f44336 0%, #d32f2f 100%); color: white; } .btn-danger:hover { transform: translateY(-2px); box-shadow: 0 8px 24px rgba(244, 67, 54, 0.4); } .btn-secondary { background: rgba(255, 255, 255, 0.05); color: #e0e0e0; border: 1px solid rgba(255, 255, 255, 0.2); } .btn-secondary:hover { background: rgba(255, 255, 255, 0.1); border-color: rgba(255, 255, 255, 0.3); } footer { text-align: center; margin-top: 4rem; padding: 1.5rem; color: #666; border-top: 1px solid rgba(255, 255, 255, 0.1); } @media (max-width: 768px) { .file-info { flex-direction: column; gap: 0.5rem; } .actions { flex-direction: column; } .btn { width: 100%; } }</style></head>"
           "<body><div class=\"bubbles\"><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div></div><div class=\"container\"><header><h1>{{FILENAME}}</h1><div class=\"file-info\"><div>Size: <span>{{FILE_SIZE}}</span></div><div>Type: <span>{{FILE_TYPE}}</span></div></div></header><div class=\"media-box\">{{MEDIA_CONTENT}}</div><div class=\"actions\"><button id=\"downloadBtn\" class=\"btn btn-primary\">Download</button><button id=\"deleteBtn\" class=\"btn btn-danger\">Delete</button><a href=\"{{BACK_URL}}\" class=\"btn btn-secondary\">Back to Directory</a></div><footer><p>WebServ Media Viewer • HTTP/1.0</p><p style=\"margin-top: 0.5rem; font-size: 0.9rem;\">Powered by caffeine and determination</p></footer></div><script>document.getElementById('downloadBtn').addEventListener('click',downloadFile);document.getElementById('deleteBtn').addEventListener('click',deleteFile);function downloadFile(){fetch(window.location.href,{method:'GET',headers:{'Must-Download':'1'}}).then(r=>r.blob()).then(blob=>{const url=window.URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download='{{FILENAME}}';document.body.appendChild(a);a.click();window.URL.revokeObjectURL(url);document.body.removeChild(a);});}function deleteFile(){if(confirm('Are you sure you want to delete this file?')){fetch(window.location.href,{method:'DELETE'}).then(r=>{if(r.ok){alert('File deleted successfully');window.location.href='{{BACK_URL}}';}else if(r.status===405){alert('Error: DELETE method not allowed for this location');}else if(r.status===403){alert('Error: Permission denied');}else if(r.status===404){alert('Error: File not found');}else{alert('Error: Failed to delete file (Status: '+r.status+')');}}).catch(err=>{alert('Error: Network error occurred');console.error(err);});}}</script></body>"
           "</html>";
}

std::string readAutoIndexTemplate()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Index of {{DIRECTORY_PATH}}</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#0a0a0a;color:#e0e0e0;min-height:100vh;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;overflow-x:hidden;}.bubbles{position:fixed;top:0;left:0;width:100%;height:100%;z-index:0;overflow:hidden;pointer-events:none;}.bubble{position:absolute;bottom:-100px;background:radial-gradient(circle,rgba(102,126,234,0.3),rgba(118,75,162,0.2));border-radius:50%;opacity:0.6;animation:rise 15s infinite ease-in;}.bubble:nth-child(1){width:80px;height:80px;left:10%;animation-duration:12s;}.bubble:nth-child(2){width:60px;height:60px;left:20%;animation-duration:14s;animation-delay:2s;}.bubble:nth-child(3){width:100px;height:100px;left:35%;animation-duration:16s;animation-delay:4s;}.bubble:nth-child(4){width:70px;height:70px;left:50%;animation-duration:13s;animation-delay:1s;}.bubble:nth-child(5){width:90px;height:90px;left:65%;animation-duration:15s;animation-delay:3s;}.bubble:nth-child(6){width:120px;height:120px;left:75%;animation-duration:17s;animation-delay:5s;}.bubble:nth-child(7){width:75px;height:75px;left:85%;animation-duration:14s;animation-delay:2.5s;}@keyframes rise{0%{bottom:-100px;transform:translateX(0);opacity:0.6;}50%{transform:translateX(100px);opacity:0.8;}100%{bottom:110%;transform:translateX(-50px);opacity:0;}}.container{position:relative;z-index:1;max-width:1000px;margin:0 auto;padding:3rem 2rem;min-height:100vh;}header{text-align:center;margin-bottom:3rem;padding:2rem;backdrop-filter:blur(10px);}h1{font-size:2.5rem;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;margin-bottom:0.5rem;}.path{font-size:1.1rem;color:#a0a0a0;margin-top:0.5rem;font-family:'Courier New',monospace;}.breadcrumb{display:inline-block;background:rgba(102,126,234,0.1);padding:0.5rem 1rem;border-radius:8px;margin-top:1rem;border:1px solid rgba(102,126,234,0.3);}.directory-list{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:2rem;backdrop-filter:blur(10px);}.list-header{display:grid;grid-template-columns:40px 2fr 1fr 1fr;gap:1rem;padding:1rem 1.5rem;background:rgba(102,126,234,0.1);border-radius:8px;margin-bottom:1rem;font-weight:600;color:#667eea;font-size:0.9rem;text-transform:uppercase;letter-spacing:0.5px;}.file-item{display:grid;grid-template-columns:40px 2fr 1fr 1fr;gap:1rem;padding:1rem 1.5rem;background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.08);border-radius:8px;margin-bottom:0.5rem;transition:all 0.3s ease;align-items:center;text-decoration:none;color:inherit;}.file-item:hover{background:rgba(255,255,255,0.08);border-color:rgba(102,126,234,0.5);box-shadow:0 4px 16px rgba(102,126,234,0.2);transform:translateX(5px);}.icon{font-size:1.5rem;text-align:center;}.file-name{color:#e0e0e0;font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}.directory .file-name{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;font-weight:600;}.file-size{color:#a0a0a0;font-size:0.9rem;text-align:right;}.file-date{color:#888;font-size:0.9rem;text-align:right;}.parent-link{background:rgba(102,126,234,0.15);border-color:rgba(102,126,234,0.3);}.parent-link:hover{background:rgba(102,126,234,0.25);border-color:rgba(102,126,234,0.6);}.stats{display:flex;justify-content:center;gap:2rem;margin-top:2rem;padding:1.5rem;background:rgba(255,255,255,0.03);border-radius:8px;}.stat-item{text-align:center;}.stat-value{font-size:1.5rem;font-weight:600;background:linear-gradient(135deg,#D11B00 0%,#00B5D1 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;}.stat-label{color:#888;font-size:0.85rem;margin-top:0.3rem;}footer{text-align:center;margin-top:4rem;padding:1.5rem;color:#666;border-top:1px solid rgba(255,255,255,0.1);}@media (max-width:768px){.list-header,.file-item{grid-template-columns:30px 1fr;}.file-size,.file-date,.list-header .file-size,.list-header .file-date{display:none;}}</style></head><body><div class=\"bubbles\"><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div><div class=\"bubble\"></div></div><div class=\"container\"><header><h1>Directory Index</h1><p class=\"path\"><span class=\"breadcrumb\">{{DIRECTORY_PATH}}</span></p></header><div class=\"directory-list\"><div class=\"list-header\"><div></div><div>Name</div><div>Size</div><div>Modified</div></div>{{FILE_LIST}}</div><div class=\"stats\"><div class=\"stat-item\"><div class=\"stat-value\">{{FILE_COUNT}}</div><div class=\"stat-label\">Files</div></div><div class=\"stat-item\"><div class=\"stat-value\">{{DIR_COUNT}}</div><div class=\"stat-label\">Directories</div></div><div class=\"stat-item\"><div class=\"stat-value\">{{TOTAL_SIZE}}</div><div class=\"stat-label\">Total Size</div></div></div><footer><p>WebServ Auto-Index • HTTP/1.0</p><p style=\"margin-top:0.5rem;font-size:0.9rem;\">Powered by caffeine and determination</p></footer></div></body></html>";
}

std::string autoIndex(Location* loc, Server server,
        clientData *data, std::string url, httpClientRequest *req)
{
    if (loc->auto_index == false)
        return (sendErrorCode(500, server, data));
    if (access(url.c_str(), F_OK) != 0)
        return (sendErrorCode(404, server, data));
    if (access(url.c_str(), R_OK | X_OK) != 0)
        return (sendErrorCode(403, server, data));
    
    DIR* dir = opendir(url.c_str());
    if (!dir)
        return (sendErrorCode(500, server, data));
    
    std::vector<FileInfo> files;
    struct dirent* entry;
    uint64_t totalSize = 0;
    int fileCount = 0;
    int dirCount = 0;
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        
        FileInfo info;
        info.name = entry->d_name;
        
        std::string fullPath = url + "/" + info.name;
        struct stat fileStat;
        
        if (stat(fullPath.c_str(), &fileStat) == 0)
        {
            info.isDirectory = S_ISDIR(fileStat.st_mode);
            info.size = fileStat.st_size;
            info.modTime = fileStat.st_mtime;
            
            if (info.isDirectory)
                dirCount++;
            else
            {
                fileCount++;
                totalSize += info.size;
            }
            files.push_back(info);
        }
    }
    closedir(dir);
    
    std::sort(files.begin(), files.end());
    std::ostringstream fileListHtml;

    if (url != "/" && url != ".")
    {
        std::string parentPath = req->_path;

        if (parentPath.length() > 1 && parentPath[parentPath.length() - 1] == '/')
            parentPath = parentPath.substr(0, parentPath.length() - 1);
        
        size_t lastSlash = parentPath.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            if (lastSlash == 0)
                parentPath = "/";
            else
                parentPath = parentPath.substr(0, lastSlash);
        }
        
        fileListHtml << "<a href=\"" << parentPath << "\" class=\"file-item parent-link directory\">"
                    << "<div class=\"icon\">📁</div>"
                    << "<div class=\"file-name\">..</div>"
                    << "<div class=\"file-size\">-</div>"
                    << "<div class=\"file-date\">-</div>"
                    << "</a>";
    }
    
    for (size_t i = 0; i < files.size(); i++)
    {
        const FileInfo& file = files[i];
        
        std::string href = req->_path + "/" + file.name;
        
        std::string classAttr = file.isDirectory ? "file-item directory" : "file-item";
        std::string icon = getFileIcon(file.name, file.isDirectory);
        std::string size = file.isDirectory ? "-" : formatFileSize(file.size);
        std::string date = formatDateTime(file.modTime);
        
        fileListHtml << "<a href=\"" << href << "\" class=\"" << classAttr << "\">"
                     << "<div class=\"icon\">" << icon << "</div>"
                     << "<div class=\"file-name\">" << file.name << "</div>"
                     << "<div class=\"file-size\">" << size << "</div>"
                     << "<div class=\"file-date\">" << date << "</div>"
                     << "</a>";
    }
    
    std::string html = readAutoIndexTemplate();
    
    html = replaceAll(html, "{{DIRECTORY_PATH}}", url);
    html = replaceAll(html, "{{FILE_LIST}}", fileListHtml.str());
    html = replaceAll(html, "{{FILE_COUNT}}", to_string(fileCount));
    html = replaceAll(html, "{{DIR_COUNT}}", to_string(dirCount));
    html = replaceAll(html, "{{TOTAL_SIZE}}", formatFileSize(totalSize));
    
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html; charset=utf-8\r\n";
    response << "Content-Length: " << html.length() << "\r\n";
    response << "Server: WebServ/1.0\r\n";
    response << "\r\n";
    response << html;
    
    return (response.str());
}
