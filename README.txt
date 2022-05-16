File Explorer
A small Ncurses file explorer that has vim like key mapping

Compile:
    Compile main.cpp with C++ compiler that supports C++20. Link to libmagic and libncurses.


Normal Mode:
    q     - quit File Explorer
    ESC   - clear command
    [n]j  - move down n lines
    [n]k  - move up n lines
    gg    - move to top
    [n]G  - move to bottom if n is empty or 0; else move to n-th file
    Enter - open file [see Open File section]
    d     - show file description [see File Description section]
    b     - cd back to last directory in history
    x     - change sorting method and sort the files accordingly [see Sorting section]
    s     - toggle selection file under cursor. Selected files are underlined
    S     - clear all selections
    R     - refresh directory. Reopen current directory to read entries.
    ~     - goto home directory set in $HOME environment variable
    v     - change to select mode [see Select Mode section]
    /     - change to search mode [see Search Mode section]
    ?     - change to recursive search mode [see Recursive Search Mode section]
    :     - change to command mode [see Command Mode section]
    

Select Mode:
    q     - quit File Explorer
    ESC   - change to Normal Mode

    [n]j
    [n]k
    gg
    [n]G  - movement same as Normal Mode. Toggle selections of all files in between last cursor position and current cursor position after movement. [see Normal Mode section]
    
    Enter - open all selected files
    

Search Mode:
    Input query to filter files in current directory and update result on every keypress.
    Normal query is case sensitive.
    Query starting with "r:" are treated as regular expression [see Regex section]
    ESC   - clear search result and change to Normal Mode
    Enter - change to Normal Mode without clearing results
    Del   - delete last character in search query
    

Recursive Search Mode:
    Input query to search for files recursively in current directory, and update result after confirming query
    Normal query is case sensitive.
    Query starting with "r:" are treated as regular expression
    ESC   - clear search result and change to Normal Mode
    Enter - confirm search query, start searching files and change to Normal Mode
    Del   - delete last character in search query
    

Command Mode:
    cd `path`
        change directory to `path`

    mv `target`
        move all selected files (or file under cursor if there are no selections) to `target`. The behaviour is same as the `mv` command in Shell
    
    rm
        move all selected files (or file under cursor if there are no selections) to `TRASH` [see Config section]

    cwd
        change directory to cwd of current shell section
        
    opendir
        open current directory using default application [see Open File section]
        

Open File:
    File Explorer first uses lstat to determine if the file is a directory, a symlink or a regular file.
    If the file is a regular file, then File Explorer uses libmagic(3) to further identify if the file is a text file, an executable or other file types
    
    If the file is a directory, File Explorer cd into that directory;
    if the file is a symlink, File Explorer follows the symlink and tries to open the file;
    if the file is a text file, File Explorer spawns `TERM` and uses `EDITOR` to open the file (both variables are set in config.hpp [see Config section]);
    if the file is an executable, File Explorer spawns `TERM` and tries to run the executable;
    otherwise, File Explorer uses `OPEN` to open the file.
    

File Description:
    File Explorer uses libmagic(3) to determine the file type. The use of libmagic(3) can be disabled by setting `USE_MAGIC` to false. However, File Explorer can then only determine whether the file is a directory, a symlink or a regular file. Moreover, regular files are marked as unknown file type to allow `OPEN` to determine which application is used to open the file.
    

Sorting:
    There are currently 4 sorting methods:
    - sort by name ascending
    - sort by name descending
    - sort by size ascending
    - sort by size descending
    
    sorting by name uses `<` and `>` operators on std::string
    sorting by size uses `<` and `>` operators on off_t
    

Regex:
    Search query starting with "r:" causes File Explorer to treat the remaining query as regular expression and be feeded to regex engine provided by C++ STL
    File Explorer uses ECMA Script regex and the expression is case insensitive.
    The regex engine tries to match the expression on the entire file name (not the full path)
    
    
Config:
    Any configurations should be set in config.hpp file. File Explorer should be recompiled to allow the modified configurations to take effect.
    Configurable constants:
    
    char* TERM
        This is the command to spawn a new terminal session.
        Default value: "dtach -A /tmp/fe-dtach-session -E"
    
    char* EDITOR
        This is the application to open text files.
        Default value: "nvim"
        
    char* OPEN
        This is the command to open files that cannot be handled by File Explorer
        Linux users may use "xdg-open" instead
        Default value: "open"
        
    bool USE_MAGIC
        Time spent on consulting libmagic(3) for file type may be significant when there are a lot of files in a directory. Setting `USE_MAGIC` to false may speed up time to open a directory if one is not interested in file type of regular files.
        Setting `USE_MAGIC` causes 'd' command in Normal Mode to return nothing about File Description [see Normal Mode]. Opening files will use `OPEN` directly [see Open Files section]
        Default value: true
        
    float COL_WIDTHS[]
        It stores the ratio of widths each column takes. Sum of ratios should be exactly equal to 1.
        Currently there are only 2 columns, file name and file size. Therefore there should only be 2 values in this array.
        Default value: { 0.8, 0.2 }
        
    bool ENABLE_LOGGING
        Debug option.
        Default value: false
        
    bool PRINT_LOG_ON_SEG_VAULT
        Debug option.
        Default value: false
        
    bool FORCE_EXIT
        Debug option. Whether File Explorer should abort when there is error.
        Default value: false