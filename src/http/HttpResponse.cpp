

#include "../../include/HttpResponse.h"

#include <string>
#include<cstring>

std::unordered_map<std::string, MimeType> Mime_map = {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/msword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css"},
        {"", "text/plain"},
        {"default","text/plain"}
};

void HttpResponse::appenBuffer(char *buffer){

    //
    if(mVersion == HttpRequest::HTTP_11){
        sprintf(buffer,"HTTP/1.1 %d %s \r\n",mStatusCode,mStatusMsg.c_str());
    }
    else{
        sprintf(buffer,"HTTP/1.0 %d %s \r\n",mStatusCode,mStatusMsg.c_str());
    }

    for (auto a : mHeaders)
    {
        sprintf(strchr(buffer,'\0'),"%s%s: %s\r\n",buffer,a.first.c_str(),a.second.c_str());
    }

    sprintf(strchr(buffer,'\0'),"%sContent-type: %s\r\n",buffer,mMime.type.c_str());

    if(keep_alive_){
        sprintf(strchr(buffer,'\0'),"%sConnection: keep-alive\r\n",buffer);
    }else{
        sprintf(strchr(buffer,'\0'),"%sConnection: close\r\n",buffer);
    }
    
}