#ifndef _COMMAND_HPP_
#define _COMMAND_HPP_

#include "explorer.hpp"

static inline std::pair<std::string, std::vector<std::string>>
parse(const std::string& str){
    FElog.add("parse command:" + str);
    auto iter = std::find(str.begin(), str.end(), ' ');
    std::string cmd (str.begin(), iter);
    if (!cmd.length()){ return {"", {}}; }
    FElog.add("cmd = \"" + cmd + "\"");

    if (iter++ == str.end()){
        FElog.add("parsed: cmd = \"" + cmd + "\"");
        return {cmd, {}};
    }
    std::string tmp;
    int escaped = false;
    std::vector<std::string> args;
    while(iter != str.end()){
        if (escaped){
            switch (*iter){
                case '\\':
                escaped = true;
                break;
                
                case ' ':
                FElog.add("arg = \"" + tmp + "\"");
                args.push_back(tmp);
                tmp.clear();
                break;
            }
        }else{
            tmp.push_back(*iter);
            escaped = false;
        }
        iter++;
    }
    if (tmp.length()){
        FElog.add("arg = \"" + tmp + "\"");
        args.push_back(tmp);
    }

    return {cmd, args};
}

#define checkArgc(args, n) \
    do { \
        if ((args).size() != n ) { \
            exitError("cd requires " + std::to_string(n) + " args"); \
            return; \
        } \
    } while(0)

static inline void
mv(std::vector<std::string> from, std::string to){
    std::string cmd = "mv ";
    for (auto& s : from){
        cmd += s;
        cmd += ' ';
    }
    cmd += to;
    FElog.add("Call shell");
    runShell(cmd);
}

static inline void
command(const std::string& input, Explorer& explorer){
    auto [cmd, args] = parse(input);
    if (cmd == ""){ return; }
    
    if (cmd == "cd"){
        checkArgc(args, 1);
        explorer.cd(args[0]);
        return;
    }
    // move to ~/.Trash
    if (cmd == "rm"){
        auto paths = explorer.getSelectedPaths();
        if (!paths.size()){
            exitError("rm", "no selection");
            return;
        }
        mv(paths, TRASH);
        explorer.cd(explorer.getcwd());
        return;
    }
    if (cmd == "mv"){
        checkArgc(args, 1);
        auto paths = explorer.getSelectedPaths();
        if (!paths.size()){
            exitError("rm", "no selection");
            return;
        }
        mv(paths, args[0]);
        explorer.cd(explorer.getcwd());
        return;
    }
    if (cmd == "cwd"){
        char path[PATH_MAX];
        if (!getcwd(path, PATH_MAX)){
            exitError("getcwd()");
        }else{
            FElog.add("Get shell cwd: "s + path);
            explorer.cd(path);
        }
        return;
    }
    // open in Finder / default file explorer
    if (cmd == "opendir"){
        system(("open " + explorer.getcwd()).c_str());
        return;
    }
}

#endif
