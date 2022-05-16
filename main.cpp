// this program uses dtach

#include "win.hpp"
#include <signal.h>
#include <functional>
#include <locale.h>

void sigVaultPrintLog(int sig){
    endwin();
    FElog.add("Segmentation fault");
    FElog.print();
    exit(1);
}

int main(){
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nonl();
    curs_set(0);
    ESCDELAY = 0;
    
    magicInit();
    Win win = Win().gethw();
    Explorer explorer;
    Controller controller;

    if (PRINT_LOG_ON_SEG_VAULT){
        signal(SIGSEGV, sigVaultPrintLog);
    }
    
    do{
        win.setUI(controller, explorer).draw();
    }while(controller.readInput().control(explorer));
    
    endwin();
    magicEnd();
    FElog.print();
    return 0;
}
