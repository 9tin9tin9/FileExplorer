#ifndef _EXPLORER_HPP_
#define _EXPLORER_HPP_

#include "file.hpp"

class Explorer{
    magic_t magicCookie;
    std::vector<File> files;
    std::vector<size_t> filterResult;
    std::vector<std::string> history;
    long cur = 0;
    enum Sort{
        NAME_A = 0,
        NAME_D,
        SIZE_A,
        SIZE_D,
        NONE,
    }sortMethod = NONE;

    static bool sortNameA(const File& l, const File& r){
        return l.name < r.name;
    }
    static bool sortNameD(const File& l, const File& r){
        return l.name > r.name;
    }
    static bool sortSizeA(const File& l, const File& r){
        return l.size < r.size;
    }
    static bool sortSizeD(const File& l, const File& r){
        return l.size > r.size;
    }
    
    auto sortFunction(auto&& l, auto&& r){
        switch (sortMethod){
            case NAME_A: return sortNameA(l, r);
            case NAME_D: return sortNameD(l, r);
            case SIZE_A: return sortSizeA(l, r);
            case SIZE_D: return sortSizeD(l, r);
            default:     return sortNameA(l, r);  // default
        }
    }
    
    // sort filterResult
    auto sort(){
        std::sort(filterResult.begin(), filterResult.end(),
            [&](size_t l, size_t r){
                return sortFunction(files.at(l), files.at(r));
            });
    }
    
    std::string getRealPath(std::string path){
        // expand ~ to home directory
        if (path == "~" || path.starts_with("~/")){
            path.erase(0, 1);
            std::string home = getHomeDir();
            path = home + path;
        }
        // append current working directory
        if (!path.starts_with("/")){
            path = getcwd() + path;
        }

        char buf[PATH_MAX];
        FILE* pathfile = fopen(path.c_str(), "r");
        if (!pathfile){
            exitError(path);
            return "";
        }
        if (fcntl(fileno(pathfile), F_GETPATH, buf) == -1) {
            exitError(path);
            fclose(pathfile);
            return "";
        }
        fclose(pathfile);
        return buf;
    }
    
    void loadEntries(DIR* dir, const std::string& path)
    {
        struct dirent* entry;
        size_t i = 0;
        while((entry = readdir(dir))){
            // turn of libmagic when doing recursive search
            File file (entry->d_name, path, USE_MAGIC);
            files.push_back(file);
            filterResult.push_back(i++);
        }
    }
    
    void filesClear(){
        files.clear();
    }
    
    bool matchName(
        const std::string& filename,
        const std::string& match)
    {
        bool useRegex = match.starts_with("r:");
        FElog.add("Use regex:" + std::to_string(useRegex));

        if (useRegex){
            std::string expr = std::string(
                match.begin() + 2, match.end());
            FElog.add("Expression:" + expr);

            try {
                std::regex regex(expr,
                    std::regex_constants::ECMAScript |
                    std::regex_constants::icase);
                return std::regex_match(filename, regex);
            }catch(...){
                return false;
            }
        }else{
            FElog.add("Expression:" + match);
            return filename.find(match) != std::string::npos;
        }
    }
    
    public:
    Explorer(){
        char buf[PATH_MAX];
        if (!getwd(buf)){
            exitError("getcwd()");
            throw std::string(ERROR_STR);
        }else{
            history.push_back(buf + "/"s);
            cd(".");
        }
    }
    
    const std::string& getcwd(){
        return history.back();
    }
    
    void cd(const std::string& path, bool recur = false){
        auto realPath = getRealPath(path);
        FElog.add("change directory: " + realPath);

        DIR* dir = opendir(realPath.c_str());
        if (!dir) {
            exitError("cd" + realPath);
            return;
        }
        files.clear();
        filterResult.clear();
        
        loadEntries(dir, realPath);
        closedir(dir);
        
        sort();
        cur = 0;
        if (history.size() == 0 || history.back() != realPath){
            history.push_back(realPath);
        }
    }
    
    void back(){
        if (history.size() == 1){
            return;
        }
        history.pop_back();
        cd(history.back());
    }
    
    void nextSort(){
        sortMethod = Sort((sortMethod + 1) % ( NONE+1 ));
        sort();
    }
    
    std::string sortBy(){
        switch (sortMethod){
            case NAME_A: return "Name ascending";
            case NAME_D: return "Name decending";
            case SIZE_A: return "Size ascending";
            case SIZE_D: return "Size descending";
            case NONE:   return "";
        }
    }
    
    size_t length(){
        return filterResult.size();
    }
    
    std::vector<File> getFiles(){
        std::vector<File> fs;
        for (const auto& r : filterResult){
            fs.push_back(files.at(r));
        }
        return fs;
    }
    
    bool& select(size_t index){
        return files[filterResult[index]].selected;
    }
    
    std::vector<File> getSelected(){
        std::vector<File> selected;
        for (const auto& f : files){
            if (f.selected){
                selected.push_back(f);
            }
        }
        return selected;
    }
    
    std::vector<std::string> getSelectedPaths(){
        std::vector<File> selected = getSelected();
        std::vector<std::string> paths;
        for (auto& f : selected){
            paths.push_back(f.fullpath);
        }
        return paths;
    }
    
    void clearSelection(){
        for (auto& f : files){
            f.selected = false;
        }
    }
    
    void clearFilter(){
        filterResult.clear();
        for (size_t i = 0; i < files.size(); i++){
            filterResult.push_back(i);
        }
    }
    
    void setCur(long pos){
        if (pos < 0) {
            cur = 0;
        }else if (pos >= filterResult.size()){
            if (filterResult.size()){
                cur = filterResult.size()-1;
            }else{
                cur = 0;
            }
        }else{
            cur = pos;
        }
    }
    
    long getCur(){
        return cur;
    }
    
    File getCurFile(){
        return files.at(filterResult.at(cur));
    }
    
    void filterName(const std::string& name){
        cur = 0;
        FElog.add("filtering: " + name);
        filterResult.clear();
        size_t fno = 0;
        for (int i = 0; i < files.size(); i++){
            if (matchName(files[i].name, name)){
                filterResult.push_back(i);
                fno++;
            }
        }
        FElog.add("filtered " + std::to_string(fno));
    }
    
    void searchRecur(const std::string& name){
        files.clear();
        filterResult.clear();

        auto basepath = getcwd();
        std::deque<File> dirs;
        dirs.push_back(File("", basepath, false));

        while(dirs.size()){
            File f = dirs[0];
            dirs.pop_front();

            if (f.type != File::DIR){
                continue;
            }
            DIR* d = opendir(f.fullpath.c_str());
            struct dirent* dirent;
            while((dirent = readdir(d))){
                File entry(dirent->d_name, f.fullpath, basepath, false);
                if (entry.fullpath.ends_with("/.") ||
                    entry.fullpath.ends_with("/.."))
                {
                    continue;
                }

                // if match
                // only match file name
                auto filename = std::string(
                    entry.name.begin() + entry.name.find_last_of('/'),
                    entry.name.end());
                if (matchName(filename, name)){
                    files.push_back(entry);
                    filterResult.push_back(files.size()-1);
                }

                // if directory
                if (entry.type == File::DIR){
                    dirs.push_back(entry);
                }
            }
        }
    }
    
    std::string getHomeDir(){
        return getenv("HOME");
    }
    
};

#endif
