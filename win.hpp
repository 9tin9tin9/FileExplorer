#ifndef _WIN_HPP_
#define _WIN_HPP_

#include "controller.hpp"

class Win{
    public:
    struct Attr{
        enum Align{
            LEFT, RIGHT
        }align;
        bool standout = false;
        bool underline = false;
    };
    
    struct Col{
        std::string str;
        Attr attr;
        float wp;
    };
    
    using Line = std::vector<Col>;
    using enum Attr::Align;
    
    private:
    size_t h, w;
    std::vector<Col> header;
    std::vector<Col> footer;
    std::vector<Line> entries;
    std::vector<size_t> selected;
    size_t scroll = 0;
    
    void print(size_t y, const Line& line){
        size_t x = 0;
        float accColWidth = 0;
        for (const auto& col : line){
            switch (col.attr.align) {
                case LEFT:
                    x = (accColWidth) * w;
                    break;
                case RIGHT:
                    x = (accColWidth + col.wp) * w
                        - col.str.length();
                    break;
            }
            
            if (col.attr.standout) standout();
            if (col.attr.underline) attron(A_UNDERLINE);
            mvprintw(y, x, "%s", col.str.c_str());
            standend();
            accColWidth += col.wp;
        }
    }
    
    void printSeparator(size_t y, char c){
        for (int i = 0; i < w; i++){
            mvprintw(y, i, "%c", c);
        }
    }
    
    // check file type and add suffix:
    // dir: '/'
    // executable: '*'
    // symlink: "-> (resolve)"
    std::string suffixByFileType(const File& file){
        std::string suffix;
        switch (file.type){
            using enum File::Type;
            case DIR: suffix = "/"; break;
            case EXE: suffix = "*"; break;
            case REG: suffix = ""; break;
            case SYM: suffix = " -> " + file.sym; break;
            case UKN: suffix = ""; break;
        }
        return suffix;
    }
    
    // change from bytes to KB / MB / GB / TB
    std::string fileSizeStr(const File& file){
        float size = file.size;
        const std::vector<std::string> units = {
            "B", "KB", "MB", "GB", "TB"
        };
        
        size_t i = log2(size)/10;
        char buf[100];
        sprintf(buf, "%.1f %s", size/(pow(1024, i)), units[i].c_str());
        return buf;
    }
    
    std::vector<Col> divideCol(const Col& col){
        std::vector<Col> cols;
        size_t width = col.wp * w;

        for (size_t x = 0; x < col.str.length(); x += width){
            auto begin = col.str.begin() + x;
            auto end = x + width < col.str.length() ?
                col.str.begin() + x + width :
                col.str.end();
            cols.push_back({
                std::string(begin, end),
                col.attr,
                col.wp
            });
        }

        return cols;
    }
    
    std::vector<Line> divideLine(const Line& line){
        std::vector<Line> lines;
        std::vector<std::vector<Col>> cols;
        
        for (const auto& col : line){
            cols.push_back(divideCol(col));
        }
        // empty line?
        if (!cols.size() || !cols[0].size()) {
            return lines;
        }

        size_t maxNoOfLines = 0;
        std::for_each(cols.begin(), cols.end(),
            [&](const auto& col){
                if (col.size() > maxNoOfLines){
                    maxNoOfLines = col.size();
                }
            });
        for (size_t i = 0; i < maxNoOfLines; i++){
            Line l;
            for (const auto& c : cols){
                if (i >= c.size()){
                    l.push_back({"", c[0].attr, c[0].wp});
                }else{
                    l.push_back(c[i]);
                }
            }
            lines.push_back(l);
        }

        return lines;
    }
    
    /*
     * header:
     * File Explorer
     *   explorer.getcwd()
     *   (number) entries
     *   Sort by: explorer.sortBy()
     * 
     * footer:
     * control.getFooter()
     * ERROR_STR || control.getBuf()
     */
    public:
    Win& setUI(Controller& control, Explorer& explorer){
        if (control.isresize()){
            gethw();
        }

        auto files = explorer.getFiles();
        // header
        pushHeader(divideCol({"File Explorer", { LEFT }, 1}));
        pushHeader(divideCol({"  " + explorer.getcwd(), { LEFT }, 1}));
        pushHeader(divideCol({
            "  " +  std::to_string(files.size()) + " Files",
            { LEFT },
            1
        }));
        auto sortByStr = explorer.sortBy();
        if (sortByStr != ""){
            auto fullStr = "  Sort by: " + explorer.sortBy();
            pushHeader(divideCol({fullStr, { LEFT }, 1}));
        }

        // footer
        pushFooter(divideCol({control.getMsg(), { LEFT }, 1}));
        if (ERROR_STR != ""){
            // trim beginning spaces
            auto begin =
                ERROR_STR.find_first_not_of(" \t") == std::string::npos ?
                    ERROR_STR.end() :
                    ERROR_STR.begin();
            ERROR_STR = std::string(begin , ERROR_STR.end());

            pushFooter(divideCol({ERROR_STR, { LEFT }, 1}));
            ERROR_STR = "";
        }else{
            pushFooter(divideCol({control.getBuf(), { RIGHT }, 1}));
        }
        
        // scrolling
        size_t centreHeight = h - header.size() - footer.size() - 2;
        if (explorer.getCur() >= scroll + centreHeight){
            scroll = explorer.getCur()-centreHeight+1;
        }else if (explorer.getCur() < scroll){
            scroll = explorer.getCur();
        }
        FElog.add("centreHeight: " + std::to_string(centreHeight));
        FElog.add("scroll: " + std::to_string(scroll));
        FElog.add("cursor: " + std::to_string(explorer.getCur()));

        float lineNoWidth = (log10(files.size()) + 3) / w;
        std::vector<float> colWidths;
        const auto NUM_OF_COL = sizeof(COL_WIDTHS)/sizeof(*COL_WIDTHS);
        for (size_t i = 0; i < NUM_OF_COL;  i++){
            colWidths.push_back(COL_WIDTHS[i]*(1-lineNoWidth));
        }
        // get all files and format the names
        // | num | file name (with suffix) | size |
        for (size_t i = scroll;
            i < files.size() && i - scroll < centreHeight;
            i++)
        {
            const auto& file = files[i];
            bool standout = explorer.getCur() == i;
            bool underline = file.selected;
            auto name = file.name + suffixByFileType(file);
            Line line = {
                {
                    std::to_string(i) + ')',
                    { LEFT, standout, underline  },
                    lineNoWidth
                },
                {
                    name,
                    { LEFT, standout, underline },
                    colWidths[0]
                },
                {
                    fileSizeStr(file),
                    { RIGHT, standout, underline },
                    colWidths[1]
                },
            };
            pushFiles(divideLine(line));
            centreHeight = h - header.size() - footer.size() - 2;
        }
        
        return *this;
    }

    Win& draw(){
        clear();
        size_t centreHeight = h - header.size() - footer.size() - 2;
        size_t y = 0;

        // header
        for (const auto& s : header){
            print(y, {s});
            y++;
        }

        printSeparator(y, '-');
        y++;

        // files
        // set scroll
        for (size_t i = 0; i < centreHeight; i++){
            if (i >= entries.size()) break;
            print(y, entries[i]);
            y++;
        }

        // footer
        // footer may cover the already printed lines
        y = h - footer.size()-1;
        printSeparator(y, '-');
        y++;

        for (const auto& s : footer){
            print(y, {s});
            y++;
        }

        refresh();
        header.clear();
        footer.clear();
        entries.clear();
        return *this;
    }
    
    Win& pushHeader(const std::vector<Col>& header){
        this->header.insert(this->header.end(), header.begin(), header.end());
        return *this;
    }
    Win& pushHeader(const Col& header){
        this->header.push_back(header);
        return *this;
    }
    
    Win& pushFooter(const std::vector<Col>& footer){
        this->footer.insert(this->footer.end(), footer.begin(), footer.end());
        return *this;
    }
    Win& pushFooter(const Col& footer){
        this->footer.push_back(footer);
        return *this;
    }
    
    Win& pushFiles(const std::vector<Line>& files){
        this->entries.insert(this->entries.end(), files.begin(), files.end());
        return *this;
    }
    Win& pushFiles(const Line& files){
        this->entries.push_back(files);
        return *this;
    }
    
    Win& gethw(){
        getmaxyx(stdscr, h, w);
        FElog.add("h: " + std::to_string(h));
        FElog.add("w: " + std::to_string(w));
        return *this;
    }
};

#endif
