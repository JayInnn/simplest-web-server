#ifndef _LOCAL_CACHE_H
#define _LOCAL_CACHE_H

#include <string>
#include <unordered_map>

class database_local_cache {

public:
    static std::unordered_map<std::string, std::string> verify_msg_map;

};

std::unordered_map<std::string, std::string> database_local_cache::verify_msg_map = {
    {"root", "root"},
};

#endif