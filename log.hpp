#ifndef _LOG_HPP_
#define _LOG_HPP_


#include <ncurses.h>
#include <vector>
#include <string>
#include <algorithm>
#include <deque>
#include <regex>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <magic.h>
#include "config.hpp"

class Log{
    std::vector<std::string> log;
    
    public:
    void add(const std::string& str){
        if (ENABLE_LOGGING){
            log.push_back(str);
        }
    }
    void print(){
        if (ENABLE_LOGGING){
            for (const auto& l : log){
                printf("%s\n", l.c_str());
            }
        }
    }
};
static Log FElog;
static std::string ERROR_STR;

static inline void
exitError(
    const std::string& str,
    const std::string& error = strerror(errno))
{
    ERROR_STR = str + ": " + error;
    FElog.add(ERROR_STR);

    if (!FORCE_EXIT_ON_ERROR) return;
    endwin();
    FElog.print();
    exit(1);
}

#endif
