

#include"../../include/HttpRequest.h"



std::ostream &operator<<(std::ostream &os,const HttpRequest &request){
    os<<"method:"<<request.mMethod<<std::endl;
    os<<"uri:"<<request.mMethod<<std::endl;
    os<<"version:"<<request.mMethod<<std::endl;

    for (auto a : request.mHeaders)
    {
        os<<a.first<<":"<<a.second<<std::endl;
    }
    return os;
}