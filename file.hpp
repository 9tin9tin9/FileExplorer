#ifndef _FILE_HPP_
#define _FILE_HPP_

#include "config.hpp"
#include "log.hpp"

using namespace std::string_literals;

static magic_t magicCookie;

static inline void magicInit(){
    FElog.add("Initialize libmagic");
    magicCookie = magic_open(
        MAGIC_NO_CHECK_APPTYPE | MAGIC_NO_CHECK_COMPRESS |
        MAGIC_NO_CHECK_ELF | MAGIC_NO_CHECK_ENCODING |
        MAGIC_NO_CHECK_TOKENS );
    if (magicCookie == NULL) {
        exitError(
            "Explorer",
            "unable to initialize magic library");
        return;
    }
    
    FElog.add("Loading default magic database");
    if (magic_load(magicCookie, NULL) != 0) {
        exitError(
            "cannot load magic database",
            magic_error(magicCookie));
        magic_close(magicCookie);
        return;
    }
}

static inline void magicEnd(){
    magic_close(magicCookie);
}

static inline std::string escapePath(const std::string& path){
    std::string escaped;
    for (const auto& c : path){
        if (c == ' ' || c == '\'' || c == '"'){
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    return escaped;
}

static inline void runShell(std::string command){
    FElog.add("run shell: " + command);
    def_prog_mode();
    auto val = system(command.c_str());
    refresh();
    
    if (val == -1){
        exitError(command);
        return;
    }
    if (val == 127){
        exitError("command", "execution failed");
        return;
    }
}

struct File{
    std::string fullpath = "";
    std::string name = "";
    bool selected = false;
    enum Type {
        DIR, EXE, REG, SYM, UKN
    }type = REG;
    std::string sym = "";
    off_t size = 0;
    std::string magic;

    File(const std::string& name,
        const std::string& parentDir,
        bool useMagic = true):
            name(name)
    {
        fullpath = parentDir;
        if (parentDir.back() != '/') {
            fullpath += '/';
        }
        fullpath += name;

        FElog.add("checking file status with lstat: " + fullpath);
        struct stat filestat;
        if (lstat(fullpath.c_str(), &filestat) == -1){
            exitError(fullpath);
        }

        size = filestat.st_size;

        if (useMagic){
            magic = magic_file(magicCookie, fullpath.c_str());
            FElog.add("Magic of " + fullpath + ": " + magic);

            if (S_ISDIR(filestat.st_mode)){
                type = DIR;
            }else if (S_ISLNK(filestat.st_mode)) {
                type = SYM;
                sym = resolveSymLink(fullpath);
            }else if (isText()){
                type = REG;
            }else if (isExecutable()){
                type = EXE;
            }else {
                type = UKN;
            }
        }else{
            if (S_ISDIR(filestat.st_mode)){
                type = DIR;
            }else if (S_ISLNK(filestat.st_mode)){
                type = SYM;
                sym = resolveSymLink(fullpath);
            }else{
                type = UKN;
            }
        }
        
    };
    
    File(const std::string& name,
        const std::string& parentdir,
        const std::string& basepath,
        bool useMagic = true) :
    File(
        [&](){
            auto realpath = parentdir.ends_with('/') ?
                parentdir + name :
                parentdir + '/' + name;
            return std::string(
                realpath.begin() + basepath.length() + 1,
                realpath.end()
            );
        }(),
        basepath,
        useMagic) { }
    
    bool hasEnding (const std::string& fullString, const std::string& ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (
                fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }
    
    std::string open(){
        switch (type){
            case DIR:
            return fullpath;

            // spawn new terminal to run
            case EXE: {
                std::string command =
                    std::string(TERM) + ' ' +
                    escapePath(fullpath);
                runShell(command);
                return "";
            };
            
            case REG: {
                std::string command;
                // open with text editor
                command =
                    std::string(TERM) + ' ' + 
                    escapePath(EDITOR) + ' ' +
                    escapePath(fullpath);
                runShell(command);
                return "";
            };
            
            // resolve sym link recursively
            case SYM: {
                return File(sym, "").open();
            };
            
            // open with system default application
            case UKN: {
                // double quote file path
                std::string command =
                    std::string(OPEN) + ' ' +
                    escapePath(fullpath);
                runShell(command);
            }
            return "";
        }
    }
    
    const std::string& getDescription(){
        return magic;
    }
    
    private:
    bool isText(){
        return isType("text") || isType("JSON") || isType("CSV");
    }
    
    bool isExecutable(){
        return isType("executable");
    }

    bool isType(const std::string& type){
        return magic.find(type) != std::string::npos;
    }

    std::string resolveSymLink(const std::string& name){
        ssize_t length = 0;
        char buf[PATH_MAX];
        length = readlink(name.c_str(), buf, PATH_MAX);
        if (length == -1){
            exitError(name);
            return "";
        }
        buf[length] = 0;
        return buf;
    }
};
#endif
