#include "WebServer.h"

void    errorPage::assign(int code, std::string lk)
{
    this->typeCode = code;
    this->link = lk;
}

std::string get200Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>WebServ Home</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\nfont-family:'Arial',sans-serif;\n}\nbody{\nbackground:#121212;\ncolor:#ffffff;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\ntext-align:center;\n}\n.container{\nmax-width:600px;\npadding:2rem;\n}\nh1{\nfont-size:2.5rem;\ncolor:#bb86fc;\nmargin-bottom:1rem;\n}\np{\nfont-size:1.2rem;\ncolor:#cccccc;\n}\n.code{\nbackground:#1e1e1e;\npadding:1rem;\nborder-radius:5px;\nmargin-top:2rem;\nfont-family:'Courier New',monospace;\n}\n</style>\n</head>\n<body>\n<div class=\"container\">\n<h1>WebServ Running</h1>\n<p>Your C++ web server is operational</p>\n<div class=\"code\">Status: <span style=\"color:#4caf50\">Online</span></div>\n</div>\n</body>\n</html>";
}

std::string get201Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>WebServ Home</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\nfont-family:'Arial',sans-serif;\n}\nbody{\nbackground:#121212;\ncolor:#ffffff;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\ntext-align:center;\n}\n.container{\nmax-width:600px;\npadding:2rem;\n}\nh1{\nfont-size:2.5rem;\ncolor:#bb86fc;\nmargin-bottom:1rem;\n}\np{\nfont-size:1.2rem;\ncolor:#cccccc;\n}\n.code{\nbackground:#1e1e1e;\npadding:1rem;\nborder-radius:5px;\nmargin-top:2rem;\nfont-family:'Courier New',monospace;\n}\n</style>\n</head>\n<body>\n<div class=\"container\">\n<h1>WebServ Running</h1>\n<p>Your C++ web server is operational</p>\n<div class=\"code\">Status: <span style=\"color:#4caf50\">Online</span></div>\n</div>\n</body>\n</html>";
}

std::string get400Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>400 - Bad Request</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ff6b6b,#ffa726);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(255,107,107,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ffa726;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ff6b6b,#ffa726,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">400</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"The universe is not required to be in perfect harmony with human ambition.\"</p><p class=\"author\">- Carl Sagan</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get403Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>403 - Forbidden</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ef5350,#ab47bc);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(239,83,80,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ab47bc;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ef5350,#ab47bc,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">403</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"Some doors are not meant to be opened; some paths are not meant to be taken.\"</p><p class=\"author\">- Ancient Proverb</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get404Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>404 - Page Not Found</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\n}\nbody{\nbackground:#121212;\ncolor:#e0e0e0;\nfont-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\noverflow:hidden;\nposition:relative;\n}\n.container{\ntext-align:center;\nz-index:2;\nposition:relative;\npadding:2rem;\nbackground:rgba(18,18,18,0.8);\nborder-radius:15px;\nbackdrop-filter:blur(10px);\nborder:1px solid rgba(255,255,255,0.1);\n}\n.error-code{\nfont-size:8rem;\nfont-weight:900;\nbackground:linear-gradient(45deg,#ff6b6b,#4ecdc4);\n-webkit-background-clip:text;\n-webkit-text-fill-color:transparent;\ntext-shadow:0 0 30px rgba(255,107,107,0.3);\nmargin-bottom:1rem;\n}\n.error-message{\nfont-size:1.5rem;\nmargin-bottom:2rem;\ncolor:#b0b0b0;\n}\n.philosophical-quote{\nborder-left:3px solid #4ecdc4;\npadding-left:1.5rem;\nmargin:1.5rem 0;\n}\n.quote{\nfont-style:italic;\ncolor:#e0e0e0;\nfont-size:1.1rem;\nmargin:0;\nline-height:1.6;\n}\n.author{\ncolor:#b0b0b0;\nfont-size:0.9rem;\nmargin-top:0.5rem;\ntext-align:right;\n}\n.home-btn{\ndisplay:inline-block;\npadding:12px 30px;\nbackground:linear-gradient(45deg,#667eea,#764ba2);\ncolor:white;\ntext-decoration:none;\nborder-radius:25px;\nfont-weight:600;\ntransition:all 0.3s ease;\nborder:none;\ncursor:pointer;\n}\n.home-btn:hover{\ntransform:translateY(-2px);\nbox-shadow:0 10px 25px rgba(102,126,234,0.3);\n}\n.floating{\nposition:absolute;\nborder-radius:50%;\nbackground:linear-gradient(45deg,#ff6b6b,#4ecdc4,#667eea);\nopacity:0.1;\nanimation:float 6s ease-in-out infinite;\n}\n.floating:nth-child(1){\nwidth:80px;\nheight:80px;\ntop:20%;\nleft:10%;\nanimation-delay:0s;\n}\n.floating:nth-child(2){\nwidth:120px;\nheight:120px;\ntop:60%;\nright:15%;\nanimation-delay:2s;\n}\n.floating:nth-child(3){\nwidth:60px;\nheight:60px;\nbottom:20%;\nleft:20%;\nanimation-delay:4s;\n}\n@keyframes float{\n0%,100%{\ntransform:translateY(0px) rotate(0deg);\n}\n50%{\ntransform:translateY(-20px) rotate(180deg);\n}\n}\n@media (max-width:768px){\n.error-code{\nfont-size:6rem;\n}\n.error-message{\nfont-size:1.2rem;\n}\n.container{\nmargin:1rem;\npadding:1.5rem;\n}\n}\n</style>\n</head>\n<body>\n<div class=\"floating\"></div>\n<div class=\"floating\"></div>\n<div class=\"floating\"></div>\n<div class=\"container\">\n<div class=\"error-code\">404</div>\n<div class=\"error-message\">\n<div class=\"philosophical-quote\">\n<p class=\"quote\">\"if you gaze long into an abyss, the abyss also gazes into you.\"</p>\n<p class=\"author\">- Friedrich Nietzsche</p>\n</div>\n</div>\n<button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button>\n</div>\n<script>\nfunction redirectToHome(){\nwindow.location.href=\"/\";\n}\ndocument.addEventListener('mousemove',(e)=>\n{const floating=document.querySelectorAll('.floating');\nfloating.forEach(element=>{\nconst speed=parseInt(element.style.width)/100;\nconst x=(window.innerWidth-e.pageX*speed)/100;\nconst y=(window.innerHeight-e.pageY*speed)/100;\nelement.style.transform=`translateX(${x}px) translateY(${y}px)`;\n});\n});\ndocument.addEventListener('keydown',(e)=>\n{if(e.key==='Enter')redirectToHome();\n});\n</script>\n</body>\n</html>";
}

std::string get405Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>405 - Method Not Allowed</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#26c6da,#42a5f5);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(38,198,218,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #42a5f5;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#26c6da,#42a5f5,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">405</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"Anyone who has never made a mistake has never tried anything new\"</p><p class=\"author\">- Einstein</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get408Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>408 - Request Timeout</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ffa726,#ff7043);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(255,167,38,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ff7043;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ffa726,#ff7043,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">408</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"Time is the most valuable thing a man can spend.\"</p><p class=\"author\">- Theophrastus</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Try Again</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get413Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>413 - Payload Too Large</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#ef5350,#ec407a);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(239,83,80,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #ec407a;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ef5350,#ec407a,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:80px;height:80px;top:20%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:120px;height:120px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:20%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">413</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"The greatest wealth is to live content with little.\"</p><p class=\"author\">- Plato</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get500Page()
{
    return "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>500 - Internal Server Error</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{background:#121212;color:#e0e0e0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;overflow:hidden;position:relative;}.container{text-align:center;z-index:2;position:relative;padding:2rem;background:rgba(18,18,18,0.8);border-radius:15px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1);}.error-code{font-size:8rem;font-weight:900;background:linear-gradient(45deg,#2c5da3,#1a781e);-webkit-background-clip:text;-webkit-text-fill-color:transparent;text-shadow:0 0 30px rgba(239,83,80,0.3);margin-bottom:1rem;}.error-message{font-size:1.5rem;margin-bottom:2rem;color:#b0b0b0;}.philosophical-quote{border-left:3px solid #1a781e;padding-left:1.5rem;margin:1.5rem 0;}.quote{font-style:italic;color:#e0e0e0;font-size:1.1rem;margin:0;line-height:1.6;}.author{color:#b0b0b0;font-size:0.9rem;margin-top:0.5rem;text-align:right;}.home-btn{display:inline-block;padding:12px 30px;background:linear-gradient(45deg,#667eea,#764ba2);color:white;text-decoration:none;border-radius:25px;font-weight:600;transition:all 0.3s ease;border:none;cursor:pointer;}.home-btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(102,126,234,0.3);}.floating{position:absolute;border-radius:50%;background:linear-gradient(45deg,#ef5350,#ec407a,#667eea);opacity:0.1;animation:float 6s ease-in-out infinite;}.floating:nth-child(1){width:110px;height:110px;top:10%;left:10%;animation-delay:0s;}.floating:nth-child(2){width:160px;height:160px;top:60%;right:15%;animation-delay:2s;}.floating:nth-child(3){width:60px;height:60px;bottom:10%;left:20%;animation-delay:4s;}@keyframes float{0%,100%{transform:translateY(0px) rotate(0deg);}50%{transform:translateY(-20px) rotate(180deg);}}@media (max-width:768px){.error-code{font-size:6rem;}.error-message{font-size:1.2rem;}.container{margin:1rem;padding:1.5rem;}}</style></head><body><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"floating\"></div><div class=\"container\"><div class=\"error-code\">500</div><div class=\"error-message\"><div class=\"philosophical-quote\"><p class=\"quote\">\"The hardest battles are the ones we fight within ourselves with ourselves.\"</p><p class=\"author\">- Dr. Shefali Tsabary</p></div></div><button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button></div><script>function redirectToHome(){window.location.href=\"/\";}document.addEventListener('mousemove',(e)=>{const floating=document.querySelectorAll('.floating');floating.forEach(element=>{const speed=parseInt(element.style.width)/100;const x=(window.innerWidth-e.pageX*speed)/100;const y=(window.innerHeight-e.pageY*speed)/100;element.style.transform=`translateX(${x}px) translateY(${y}px)`;});});document.addEventListener('keydown',(e)=>{if(e.key==='Enter')redirectToHome();});</script></body></html>";
}

std::string get504Page()
{
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<title>504 - Gateway Timeout</title>\n<style>\n*{\nmargin:0;\npadding:0;\nbox-sizing:border-box;\n}\nbody{\nbackground:#121212;\ncolor:#e0e0e0;\nfont-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;\nmin-height:100vh;\ndisplay:flex;\njustify-content:center;\nalign-items:center;\noverflow:hidden;\nposition:relative;\n}\n.container{\ntext-align:center;\nz-index:2;\nposition:relative;\npadding:2rem;\nbackground:rgba(18,18,18,0.8);\nborder-radius:15px;\nbackdrop-filter:blur(10px);\nborder:1px solid rgba(255,255,255,0.1);\n}\n.error-code{\nfont-size:8rem;\nfont-weight:900;\nbackground:linear-gradient(45deg,#ff6b6b,#ee5a6f);\n-webkit-background-clip:text;\n-webkit-text-fill-color:transparent;\ntext-shadow:0 0 30px rgba(255,107,107,0.3);\nmargin-bottom:1rem;\n}\n.error-message{\nfont-size:1.5rem;\nmargin-bottom:2rem;\ncolor:#b0b0b0;\n}\n.philosophical-quote{\nborder-left:3px solid #ff6b6b;\npadding-left:1.5rem;\nmargin:1.5rem 0;\n}\n.quote{\nfont-style:italic;\ncolor:#e0e0e0;\nfont-size:1.1rem;\nmargin:0;\nline-height:1.6;\n}\n.author{\ncolor:#b0b0b0;\nfont-size:0.9rem;\nmargin-top:0.5rem;\ntext-align:right;\n}\n.home-btn{\ndisplay:inline-block;\npadding:12px 30px;\nbackground:linear-gradient(45deg,#667eea,#764ba2);\ncolor:white;\ntext-decoration:none;\nborder-radius:25px;\nfont-weight:600;\ntransition:all 0.3s ease;\nborder:none;\ncursor:pointer;\n}\n.home-btn:hover{\ntransform:translateY(-2px);\nbox-shadow:0 10px 25px rgba(102,126,234,0.3);\n}\n.floating{\nposition:absolute;\nborder-radius:50%;\nbackground:linear-gradient(45deg,#ff6b6b,#ee5a6f,#ffa502);\nopacity:0.1;\nanimation:float 6s ease-in-out infinite;\n}\n.floating:nth-child(1){\nwidth:110px;\nheight:110px;\ntop:10%;\nleft:10%;\nanimation-delay:0s;\n}\n.floating:nth-child(2){\nwidth:160px;\nheight:160px;\ntop:60%;\nright:15%;\nanimation-delay:2s;\n}\n.floating:nth-child(3){\nwidth:60px;\nheight:60px;\nbottom:10%;\nleft:20%;\nanimation-delay:4s;\n}\n@keyframes float{\n0%,100%{\ntransform:translateY(0px) rotate(0deg);\n}\n50%{\ntransform:translateY(-20px) rotate(180deg);\n}\n}\n@media (max-width:768px){\n.error-code{\nfont-size:6rem;\n}\n.error-message{\nfont-size:1.2rem;\n}\n.container{\nmargin:1rem;\npadding:1.5rem;\n}\n}\n</style>\n</head>\n<body>\n<div class=\"floating\"></div>\n<div class=\"floating\"></div>\n<div class=\"floating\"></div>\n<div class=\"container\">\n<div class=\"error-code\">504</div>\n<div class=\"philosophical-quote\">\n<p class=\"quote\">\"Sometimes the messenger fails, not the message.\"</p>\n<p class=\"author\">- Unknown</p>\n</div>\n<button class=\"home-btn\" onclick=\"redirectToHome()\">Return to Safety</button>\n</div>\n<script>\nfunction redirectToHome(){\nwindow.location.href=\"/\";\n}\ndocument.addEventListener('mousemove',(e)=>\n{const floating=document.querySelectorAll('.floating');\nfloating.forEach(element=>{\nconst speed=parseInt(element.style.width)/100;\nconst x=(window.innerWidth-e.pageX*speed)/100;\nconst y=(window.innerHeight-e.pageY*speed)/100;\nelement.style.transform=`translateX(${x}px) translateY(${y}px)`;\n});\n});\ndocument.addEventListener('keydown',(e)=>\n{if(e.key==='Enter')redirectToHome();\n});\n</script>\n</body>\n</html>";
}

std::string    createResponse( int code, clientData *data )
{
    std::string             bodyResponse = "";
    std::stringstream       ss;
    std::string             str;

    if (code == 200)
    {
        ss << get200Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get200Page();
    }
    else if (code == 201)
    {
        ss << get201Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 201 Created\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get201Page();
    }
    else if (code == 400)
    {
        ss << get400Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get400Page();
    }
    else if (code == 403)
    {
        ss << get403Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get403Page();
    }
    else if (code == 404)
    {
        ss << get404Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get404Page();
    }
    else if (code == 405)
    {
        ss << get405Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get405Page();
    }
    else if (code == 408)
    {
        ss << get408Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 408 Request Timeout\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get408Page();
    }
    else if (code == 413)
    {
        ss << get413Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 413 Payload Too Large\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get413Page();
    }
    else if (code == 500)
    {
        ss << get500Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get500Page();
    }
    else if (code == 504)
    {
        ss << get504Page().length();
        ss >> str;
        bodyResponse += "HTTP/1.0 504 Gateway Timeout\r\nContent-Type: text/html\r\nContent-length: " + str + "\r\nConnection: close\r\n\r\n";
        bodyResponse += get504Page();
    }
    data->state = SENDING_RESPONSE;
    return (bodyResponse);
}

static std::string getStatusMessage(int code)
{
    switch (code)
    {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default: return "Random Error Occured";
    }
}

std::string sendErrorCode(int code, Server server, clientData *data)
{
    std::string location = server.checkValue("/errors", "root");
    
    if (location.empty())
        return (createResponse(code, data));
    
    std::string cleanLocation = location;
    if (!cleanLocation.empty() && cleanLocation[0] == '/')
        cleanLocation = cleanLocation.substr(1);
    
    std::string errorFile = server.findFile(code);
    if (errorFile.empty())
        return (createResponse(code, data));
    
    std::string fileName = cleanLocation;
    if (!fileName.empty() && fileName[fileName.length() - 1] != '/')
        fileName += "/";
    fileName += errorFile;
    
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 0)
        return (createResponse(500, data));

    std::string fileContent;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
        fileContent.append(buffer, bytesRead);
    close(fd);

    if (bytesRead == -1)
        return (createResponse(500, data));

    std::string response = "HTTP/1.0 " + to_string(code) + " " + getStatusMessage(code) + "\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + to_string(fileContent.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += fileContent;
    return (response);
}
