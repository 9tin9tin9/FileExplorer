#ifndef _CONTROLLER_HPP_
#define _CONTROLLER_HPP_

#include "command.hpp"

static inline bool isNum(char c){
    return c >= '0' && c <= '9';
}

#define ctrl(a) ((a) & 0x1F)
#define ESC 27
#define DEL 127
static const char* ENTER_STR = "<enter>";
static const char* TAB_STR = "<tab>";
static const char* SPACE_STR = "<space>";
static const char* DEL_STR = "<del>";
static const char* ESC_STR = "<esc>";
static const char* CTRL_STR = "C-";
    
#define ARM_START() if (0) {}
#define ARM(cond, ...) else if (cond) { __VA_ARGS__; buf.clear(); }
#define ARM_DEFAULT(...) else { __VA_ARGS__; buf.clear(); }

class Controller{
    std::vector<int> buf;
    std::string inputBuf;
    std::string msg;
    enum Mode{
        NORMAL, SELECT, SEARCH, RECUR_SEARCH, COMMAND
    }mode = NORMAL;
    
    bool resize = false;
  
    size_t getRepeat(){
        size_t repeat = 0;
        for (const auto& c : buf){
            if (isNum(c)){
                repeat = repeat*10 + c-'0';
            }else{
                break;
            }
        }
        return repeat;
    }
    
    std::string getVerb(){
        auto begin = std::find_if(
            buf.begin(),
            buf.end(),
            [](int c){
                return !isNum(c);
            });
        return std::string(begin, buf.end());
    }
    
    void move_down(Explorer& explorer, size_t repeat){
        repeat = repeat ? repeat : 1;
        explorer.setCur(explorer.getCur() + repeat);
    }
    
    void move_up(Explorer& explorer, size_t repeat){
        repeat = repeat ? repeat : 1;
        explorer.setCur(explorer.getCur() - repeat);
    }
    
    void move_top(Explorer& explorer){
        explorer.setCur(0);
    }
    
    void move_goto(Explorer& explorer, size_t repeat){
        if (repeat) {
            explorer.setCur(repeat);
        }else{
            explorer.setCur(explorer.length()-1);
        }
    }
    
    void openFiles(Explorer& explorer){
        auto selected = explorer.getSelected();
        if (selected.size()){
            for (auto& f : selected){
                f.open();
            }
        }else{
            if (!explorer.length()){
                return;
            }
            auto dir = explorer.getCurFile().open();
            if (dir != ""){
                explorer.cd(dir);
            }
        }
    }
    
    static bool has(const std::string& verb, int ch, size_t length = -1){
        std::string::const_iterator end;
        if (length == -1){
            end = verb.cend();
        }else{
            end = verb.cbegin() + length;
        }
        return std::find(verb.cbegin(), end, ch) != verb.end();
    }
    
    template<typename OnEsc, typename OnEnter,
             typename OnDel, typename OnUpdate>
    void textInput(const std::string& verb,
                   OnEsc onEsc, OnEnter onEnter,
                   OnDel onDel, OnUpdate onUpdate)
    {
        ARM_START()
        // exit search mode
        // clear filter
        ARM(has(verb, ESC, 1), {
            mode = NORMAL;
            onEsc();
            inputBuf.clear();
        })
        // enter
        // exit search mode and keep filter result
        ARM(has(verb, '\r', 1), {
            mode = NORMAL;
            onEnter();
            inputBuf.clear();
        })
        // backspace
        ARM(has(verb, DEL, 1), {
            if (inputBuf.length()){
                inputBuf.pop_back();
                onDel();
            }
        })
        ARM_DEFAULT({
            inputBuf += verb;
            onUpdate();
        });
    }

    std::string getBufRaw(){
        return std::string(buf.begin(), buf.end());
    }

    public:
    std::string getBuf(){
        std::string str = " ";
        for (const auto& c : buf){
            switch(c){
                case ESC: str += ESC_STR; continue;
                case DEL: str += DEL_STR; continue;
                case '\r': str += ENTER_STR; continue;
                case ' ': str += SPACE_STR; continue;
                case '\t': str += TAB_STR; continue;
            }
            for (int a = 'A'; a <= 'Z'; a++){
                if (c == ctrl(a)){
                    str += std::string(CTRL_STR) + (char)a;
                    goto next;
                }
            }
            str.push_back(c);
            next:;
        }
        return str;
    }
    
    std::string getMsg(){
        if (msg != ""){
            return msg;
        }
        switch (mode){
            case SEARCH:
            return "Search:" + inputBuf;
            
            case COMMAND:
            return ":" + inputBuf;
            
            case SELECT:
            return "Select";
        
            case RECUR_SEARCH:
            return "Recursive Search:" + inputBuf;
        
            case NORMAL:
            return "Normal";
        }
    }

    Controller& readInput(){
        resize = false;
        int ch;

        do {
            ch = getch();
            FElog.add("Input:" + std::to_string(ch));
            if (ch == KEY_RESIZE){
                resize = true;
                ch = buf.size() ? -1 : ESC;
            }
        }while(ch == -1);

        buf.push_back(ch);
        return *this;
    }

    bool isresize(){
        return resize;
    }

    int control(Explorer& explorer){
        // cancel command
        auto verb = getVerb();
        size_t repeat = getRepeat();
        msg = "";

        switch (mode){
            case NORMAL:
            ARM_START()
            // esc - clear command
            ARM(has(verb, ESC), {
                buf.clear();
            })
            // quit
            ARM(verb == "q", {
                return 0;
            })
            // down
            ARM(verb == "j", { move_down(explorer, repeat); })
            // up
            ARM(verb == "k", { move_up(explorer, repeat); })
            // top
            ARM(verb == "gg", { move_top(explorer); })
            // bottom / goto
            ARM(verb == "G", { move_goto(explorer, repeat); })
            // open
            ARM(verb == "\r", {
                openFiles(explorer);
            })
            // file description
            ARM(verb == "d", {
                msg = explorer.getCurFile().getDescription();
            })
            // back history
            ARM(verb == "b", {
                explorer.back();
            })
            // sort
            ARM(verb == "x", {
                explorer.nextSort();
            })
            // select
            ARM(verb == "s", {
                bool& s = explorer.select(explorer.getCur());
                s = !s;
            })
            // clear selection
            ARM(verb == "S", {
                explorer.clearSelection();
            })
            // refresh
            ARM(verb == "R", {
                FElog.add("Refresh");
                explorer.cd(explorer.getcwd());
            })
            // goto home directory
            ARM(verb == "~", {
                FElog.add("Read environment variable $HOME");
                std::string home = explorer.getHomeDir();
                FElog.add("Home: " + home);
                explorer.cd(home);
            })
            // select mode
            ARM(verb == "v", {
                FElog.add("change to select mode");
                mode = SELECT;
            })
            // search mode
            ARM(verb == "/", {
                FElog.add("change to search mode");
                mode = SEARCH;
            })
            // recursive search mode
            ARM(verb == "?", {
                FElog.add("change to recursive searching mode");
                mode = RECUR_SEARCH;
            })
            // command mode
            ARM(verb == ":", {
                FElog.add("change to command mode");
                mode = COMMAND;
            })
            break;
            
            case SELECT:
            ARM_START()
            // quit
            ARM(verb == "q", {
                return 0;
            })
            // exit select mode
            ARM(has(verb, ESC), {
                FElog.add("change to normal mode");
                mode = NORMAL;
            })
            // movement
            // select all files in between start and end
            ARM(verb == "j" || verb == "k" ||
                verb == "G" || verb == "gg",
            {
                long before = explorer.getCur();
                switch (verb.front()){
                    case 'j': move_down(explorer, repeat); break;
                    case 'k': move_up(explorer, repeat); break; 
                    case 'g': move_top(explorer); break;
                    case 'G': move_goto(explorer, repeat); break;
                }
                long dest = explorer.getCur();
                bool beforeSmaller = before < dest;
                long start = beforeSmaller ? before : dest;
                long end = beforeSmaller ? dest : before;
                for (long i = start; i < end; i++){
                    FElog.add("select index: " + std::to_string(i));
                    explorer.select(i) = !explorer.select(i);
                }
            })
            // open files
            ARM(verb == "\r", {
                openFiles(explorer);
            });
            break;
            
            case SEARCH:{
                auto onEsc = [&](){ explorer.clearFilter(); };
                auto onEnter = [](){};
                auto onDel = [&](){ explorer.filterName(inputBuf); };
                auto onUpdate = [&](){ explorer.filterName(inputBuf); };
                textInput(getBufRaw(), onEsc, onEnter, onDel, onUpdate);
                break;
            }

            case RECUR_SEARCH:{
                auto onEsc = [&](){
                    explorer.clearFilter();
                    explorer.cd(explorer.getcwd());
                };
                auto onEnter = [&](){
                    explorer.searchRecur(inputBuf);
                };
                auto onDel = [](){};
                auto onUpdate = [](){};
                textInput(getBufRaw(), onEsc, onEnter, onDel, onUpdate);
                break;
            }
        
            case COMMAND:{
                auto onEsc = [](){};
                auto onEnter = [&](){ command(inputBuf, explorer); };
                auto onDel = [](){};
                auto onUpdate = [](){};
                textInput(getBufRaw(), onEsc, onEnter, onDel, onUpdate);
                break;
            }
        }
        return 1;
    }
    
    #undef ARM_START
    #undef ARM
};

#endif
