/*
 * 학번 : 2015202065
 * 이름 : 윤홍찬
 */

/* GUI Headers */
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

/* Socket Headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/socket.h>

/* GUI Flags */
#define HIGHTLIGHT 1
#define O 1
#define X 2

/* Socket Flags */
#define SIGN_UP 1111
#define SIGN_IN 2222
#define WITHDRAWAL 3333
#define PLAY_GAME 4444
#define PLAYER1 5555
#define PLAYER2 6666
#define GAME_START 7777
#define LOGOUT 8888
#define GAME_NEXTTURN 9000
#define GAME_REGAME 9010
#define GAME_EXIT 9020
#define WIN_PLAYER1 9030
#define WIN_PLAYER2 9040
#define DRAW 9050
#define GAME_MOVE 9060

/* User Flags */
#define MAX_ID_PASS_SIZE 32

/* GUI Functions */
WINDOW* makeFirstWindow(void);
WINDOW* makeSecondWindow(void);
void printMainMenu(WINDOW *win, int hightlight, char* menu[], int menuNum, int inter);
void windowMainBefore(void);        // Main Window (Before Login)
void windowMainAfter(void);         // Main Window (After Login)
void windowSignIn(void);            // Sign In Window 
void windowSignUp(void);            // Sign Up Winodw
void windowUserStatus(void);        // User Status Window
void windowWithdrawal(void);        // Withdrawal Window
void windowGame(void);              // Game Window
void initArray(int (*arr)[]);
void printGame(WINDOW *win, int (*hightlight)[], int (*gameBoard)[]);

/* Add Project 2 */
void printGameMenu(WINDOW *win, int hightlight, char* menu[], int menuNum);
void printGameMenuAllUnderline(WINDOW *window2);
int gameMenuInput(WINDOW* window1, WINDOW* window2, int winHightlight, char* menu[]);
void printGamePlayerStatus(WINDOW* window2, int, int, int);
int checkBoard(int (*gameBoard)[], int, int);


/* Socket Functions */
void connectServer(char const ip[], char const port[]); // Connect to server
void signalHandler(int signum); // must add!

/* GUI Global var */
int currentX;
int currentY;

/* Socket Global var */
int clientfd = 0;
struct sockaddr_in serverAddr = { 0x00, };

typedef struct User{
    char id[MAX_ID_PASS_SIZE + 1];
    char pass[MAX_ID_PASS_SIZE + 1];
    int win;
    int lose;
}User;

typedef struct SendRequest{
    int flag;
    char id[MAX_ID_PASS_SIZE + 1];
    char pass[MAX_ID_PASS_SIZE + 1];
}SendRequest;

typedef struct SendRequestGame{
    int flag;
    User anotherUser;
}SendRequestGame;

typedef struct GameStatus{
    int gameBoard[6][6];
    int player1Num;
    int player2Num;
    int currentPlayer;
    int flag;
    int status;
    int currentX;
    int currentY;
}GameStatus;

/* User Global var*/
User currentLoginUser = { 0x00, };      // Now login User
User anotherUser = { 0x00, };           // Now login another User
int playerNum;                          // Now login Number
int myNum = 2;
int anotherNum = 2;
User previousUser = { 0x00, };

int main(int argc, char const *argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, signalHandler);

    if(argc != 3){
        printf("Usage: %s [server IP] [port]\n", argv[0]);
        return -1;
    }

    connectServer(argv[1], argv[2]);

    initscr();

    cbreak();               // Line buffering disabled
    noecho();               // Hide input char
    keypad(stdscr, TRUE);   // To get direction key

    windowMainBefore();
    endwin();
    return 0;
}

void connectServer(char const ip[], char const port[]){
    if((clientfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket error!");
        exit(-1);
    }

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(atoi(port));

    if(connect(clientfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1){
        perror("connect error!");
        exit(-1);
    }
}

void signalHandler(int signum){
    if(signum == SIGINT){
        close(clientfd);
        endwin();
        exit(-1);
    }
    else if(signum == SIGPIPE){
        close(clientfd);
        puts("Disconnected!");
        endwin();
        exit(-1);
    }
}

WINDOW* makeFirstWindow(void){
    WINDOW* newWindow;
    if (has_colors() == FALSE) {
        puts("Terminal does not support colors!");
        endwin();
        exit(-1);
    } 
    else {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_WHITE); // font, screen
    }
    refresh();
    newWindow = newwin(18, 80, 0, 0); // height, width, start y, start x
    wbkgd(newWindow, COLOR_PAIR(1));

    return newWindow;
}

WINDOW* makeSecondWindow(void){
    WINDOW* newWindow;
    if (has_colors() == FALSE) {
        puts("Terminal does not support colors!");
        endwin();
        exit(-1);
    } 
    else {
        start_color();
        init_pair(2, COLOR_WHITE, COLOR_MAGENTA);
    }
    refresh();
    newWindow = newwin(6, 80, 18, 0);
    wbkgd(newWindow, COLOR_PAIR(2));

    return newWindow;
}

void printMainMenu(WINDOW *win, int hightlight, char* menu[], int menuNum, int inter){
    int i = 0;
    int x = 11;
    int y = 3;

    for(i = 0; i < menuNum; i++){
            if(hightlight == i+1){
                wattron(win, A_REVERSE);
                mvwprintw(win, y, x, "%s", menu[i]);
                wattroff(win, A_REVERSE);
            }
            else
                mvwprintw(win, y, x, "%s", menu[i]);
            x += inter;
    }
    wrefresh(win);
}

void windowMainBefore(void){
    WINDOW *window1;
    WINDOW *window2;
    char *menu[] = {
        "SIGN IN",
        "SIGN UP",
        "EXIT"
    };
    int ch;
    int hightlight = 1;
    int menuNum = sizeof(menu) / sizeof(char*); // 3

    SendRequest sendMsg = { 0x00, };

    window1 = makeFirstWindow();
    window2 = makeSecondWindow();

    mvwprintw(window1, 18/3, 80/2 - 12, "System Software Practice");
    wattron(window1, A_BOLD);
    mvwprintw(window1, 18/3 + 2 ,80/2 - 4, "OTHELLO");
    wattroff(window1, A_BOLD);
    mvwprintw(window1, 14, 67, "2015202065");
    mvwprintw(window1, 16, 67, "HongChan Yun");

    printMainMenu(window2, hightlight, menu, menuNum, 25);
    curs_set(0);
    
    wrefresh(window1);
    wrefresh(window2);

    while(ch = getch()){
        if(ch == KEY_LEFT){         // User hit left direction
            if(hightlight == 1)
                hightlight = menuNum;
            else
                hightlight--;
        }
        else if(ch == KEY_RIGHT){   // User hit right direction
            if(hightlight == menuNum)
                hightlight = 1;
            else
                hightlight++;
        }
        else if(ch == 10)           // User hit enter
            break;
        printMainMenu(window2, hightlight, menu, menuNum, 25);
    }
    curs_set(2);
    if(hightlight == 1){
        // SIGN IN
        delwin(window1);
        delwin(window2);
        windowSignIn();
    }
    else if(hightlight == 2){
        // SIGN UP
        delwin(window1);
        delwin(window2);
        windowSignUp();
    }
    else if(hightlight == 3){
        // EXIT
        sendMsg.flag = LOGOUT;
        send(clientfd, &sendMsg, sizeof(sendMsg), 0);
        close(clientfd);
        endwin();
        exit(1);
    }
}

void windowMainAfter(void){
    WINDOW *window1;
    WINDOW *window2;
    char *menu[] = {
        "PLAY",
        "SIGN OUT",
        "WITHDRAWAL"
    };
    int ch;
    int hightlight = 1;
    int menuNum = sizeof(menu) / sizeof(char*); // 3
    SendRequest sendMsg = { 0x00, };

    window1 = makeFirstWindow();
    window2 = makeSecondWindow();

    mvwprintw(window1, 18/3, 80/2-10, "PLAYER ID : %s", currentLoginUser.id);

    printMainMenu(window2, hightlight, menu, menuNum, 25);
    curs_set(0);
    
    wrefresh(window1);
    wrefresh(window2);

    while(ch = getch()){
        if(ch == KEY_LEFT){         // User hit left direction
            if(hightlight == 1)
                hightlight = menuNum;
            else
                hightlight--;
        }
        else if(ch == KEY_RIGHT){   // User hit right direction
            if(hightlight == menuNum)
                hightlight = 1;
            else
                hightlight++;
        }
        else if(ch == 10)           // User hit enter
            break;
        printMainMenu(window2, hightlight, menu, menuNum, 25);
    }
    curs_set(2);
    if(hightlight == 1){
        // PLAY
        delwin(window1);
        delwin(window2);
        windowUserStatus();
    }
    else if(hightlight == 2){
        // SIGN OUT
        delwin(window1);
        delwin(window2);
        windowMainBefore();
    }
    else if(hightlight == 3){
        // WITHDRAWAL
        delwin(window1);
        delwin(window2);
        windowWithdrawal();
    }
}

void windowSignIn(void){
    WINDOW *window1;
    WINDOW *window2;
    char *menu[] = {
        "SIGN IN",
        "BACK"
    };
    int ch;
    int hightlight = 0;
    int menuNum = sizeof(menu) / sizeof(char*); // 2
    char msg[] = ">>> Incorrect ID / Password, Please try again! (Press any key...)";
    int i = 0;
    SendRequest sendMsg = { 0x00, };
    int responseValue = 0;

    window1 = makeFirstWindow();
    window2 = makeSecondWindow();

    wattron(window1, A_BOLD);
    mvwprintw(window1, 18/3, 80/2 - 3, "SIGN IN");
    wattroff(window1, A_BOLD);
    mvwprintw(window1, 18/3+3, 80/2-8, "ID : ");
    mvwprintw(window1, 18/3+5, 80/2-14, "PASSWORD : ");
    
    wrefresh(window2);
    printMainMenu(window2, hightlight, menu, menuNum, 50);
    
    wrefresh(window1);
    wrefresh(window2);

    memset(sendMsg.id, '\0', MAX_ID_PASS_SIZE + 1);
    memset(sendMsg.pass, '\0', MAX_ID_PASS_SIZE + 1);
    sendMsg.flag = SIGN_IN;
    
    // get id
    currentY = 9;
    currentX = 37;
    move(currentY, currentX);
    while((ch = getch()) != '\n'){
        if(ch == 127){      
            // KEY_BACKSPACE
            if(currentX == 37)
                continue;

            move(currentY, --currentX);
            mvwprintw(window1, currentY, currentX, " ");
            wrefresh(window1);
            memset(sendMsg.id + --i, '\0', 1);
            continue;
        }
        if(ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
            // ignore
            continue;
        sendMsg.id[i++] = ch;
        mvwprintw(window1, currentY, currentX++, "%c", ch);

        if(i >= MAX_ID_PASS_SIZE)
            break;
        wrefresh(window1);
    }
    i = 0;

    // get password
    currentY += 2;
    currentX = 37;
    move(currentY, currentX);
    while((ch = getch()) != '\n'){
        if(ch == 127){      
            // KEY_BACKSPACE
            if(currentX == 37)
                continue;

            move(currentY, --currentX);
            mvwprintw(window1, currentY, currentX, " ");
            wrefresh(window1);
            memset(sendMsg.pass + --i, '\0', 1);
            continue;
        }
        if(ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
            // ignore
            continue;
        sendMsg.pass[i++] = ch;
        mvwprintw(window1, currentY, currentX++, "*");

        if(i >= MAX_ID_PASS_SIZE)
            break;
        wrefresh(window1);
    }
    i = 0;

    hightlight = 1;
    printMainMenu(window2, hightlight, menu, menuNum, 50);
    curs_set(0);

    while(ch = getch()){
        if(ch == KEY_LEFT){         // User hit left direction
            if(hightlight == 1)
                hightlight = menuNum;
            else
                hightlight--;
        }
        else if(ch == KEY_RIGHT){   // User hit right direction
            if(hightlight == menuNum)
                hightlight = 1;
            else
                hightlight++;
        }
        else if(ch == 10)           // User hit enter
            break;
        printMainMenu(window2, hightlight, menu, menuNum, 50);
    }
    curs_set(2);

    if(hightlight == 1){
        // SIGN IN
        send(clientfd, &sendMsg, sizeof(sendMsg), 0);
        recv(clientfd, &currentLoginUser, sizeof(currentLoginUser), 0);

        if(strlen(currentLoginUser.id) != 0){
            if((strlen(sendMsg.id) == 0) || (strlen(sendMsg.pass) == 0)){
                mvwprintw(window2, 5, 0, msg);

                wrefresh(window2);
                getch();
                delwin(window1);
                delwin(window2);
            
                windowSignIn();
            }
            // Success
            delwin(window1);
            delwin(window2);
            windowMainAfter();
        }
        else{
            // Fail
            mvwprintw(window2, 5, 0, msg);

            wrefresh(window2);
            getch();
            delwin(window1);
            delwin(window2);
            
            windowSignIn();
        }
    }
    else if(hightlight == 2){
        // BACK
        delwin(window1);
        delwin(window2);
        windowMainBefore();
    }
}

void windowSignUp(void){
    WINDOW *window1;
    WINDOW *window2;
    char *menu[] = {
        "SIGN UP",
        "BACK"
    };
    int ch;
    int hightlight = 0;
    int menuNum = sizeof(menu) / sizeof(char*); // 2
    char smsg[] = ">>> Welcome to OTHELLO World! (Press any key...)";
    char fmsg[] = "has already exist in DB! (Press any key...)";
    int i = 0;

    SendRequest sendMsg = { 0x00, };
    int responseValue = 0;

    window1 = makeFirstWindow();
    window2 = makeSecondWindow();

    wattron(window1, A_BOLD);
    mvwprintw(window1, 18/3, 80/2 - 3, "SIGN UP");
    wattroff(window1, A_BOLD);
    mvwprintw(window1, 18/3+3, 80/2-8, "ID : ");
    mvwprintw(window1, 18/3+5, 80/2-14, "PASSWORD : ");

    wrefresh(window2);
    printMainMenu(window2, hightlight, menu, menuNum, 50);
    
    wrefresh(window1);
    wrefresh(window2);

    memset(sendMsg.id, '\0', MAX_ID_PASS_SIZE + 1);
    memset(sendMsg.pass, '\0', MAX_ID_PASS_SIZE + 1);
    sendMsg.flag = SIGN_UP;
    
    // get id
    currentY = 9;
    currentX = 37;
    move(currentY, currentX);
    while((ch = getch()) != '\n'){
        if(ch == 127){      
            // KEY_BACKSPACE
            if(currentX == 37)
                continue;

            move(currentY, --currentX);
            mvwprintw(window1, currentY, currentX, " ");
            wrefresh(window1);
            memset(sendMsg.id + --i, '\0', 1);
            continue;
        }
        if(ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
            // ignore
            continue;
        sendMsg.id[i++] = ch;
        mvwprintw(window1, currentY, currentX++, "%c", ch);

        if(i >= MAX_ID_PASS_SIZE)
            break;
        wrefresh(window1);
    }
    i = 0;

    // get password
    currentY += 2;
    currentX = 37;
    move(currentY, currentX);
    while((ch = getch()) != '\n'){
        if(ch == 127){      
            // KEY_BACKSPACE
            if(currentX == 37)
                continue;

            move(currentY, --currentX);
            mvwprintw(window1, currentY, currentX, " ");
            wrefresh(window1);
            memset(sendMsg.pass + --i, '\0', 1);
            continue;
        }
        if(ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
            // ignore
            continue;
        sendMsg.pass[i++] = ch;
        mvwprintw(window1, currentY, currentX++, "*");

        if(i >= MAX_ID_PASS_SIZE)
            break;
        wrefresh(window1);
    }
    i = 0;

    hightlight = 1;
    printMainMenu(window2, hightlight, menu, menuNum, 50);
    curs_set(0);

    while(ch = getch()){
        if(ch == KEY_LEFT){         // User hit left direction
            if(hightlight == 1)
                hightlight = menuNum;
            else
                hightlight--;
        }
        else if(ch == KEY_RIGHT){   // User hit right direction
            if(hightlight == menuNum)
                hightlight = 1;
            else
                hightlight++;
        }
        else if(ch == 10)           // User hit enter
            break;
        printMainMenu(window2, hightlight, menu, menuNum, 50);
    }
    curs_set(2);

    if(hightlight == 1){
        // SIGN UP
        send(clientfd, &sendMsg, sizeof(sendMsg), 0);
        recv(clientfd, &responseValue, sizeof(int), 0);
        if(responseValue == 1){
            // Success
            mvwprintw(window2, 5, 0, smsg);
            wrefresh(window2);

            getch();
            delwin(window1);
            delwin(window2);
           
            windowMainBefore();
        }else{
            // Fail
            mvwprintw(window2, 5, 0, "<<< %s %s", sendMsg.id, fmsg);
            wrefresh(window2);

            getch();
            delwin(window1);
            delwin(window2);

            windowSignUp();
        }
    }
    else if(hightlight == 2){
        // BACK
        delwin(window1);
        delwin(window2);
        windowMainBefore();
    }
}

void windowUserStatus(void){
    WINDOW *window1;
    WINDOW *window2;
    int ch;
    int userWin = currentLoginUser.win;
    int userLose = currentLoginUser.lose;
    double percent = (double)userWin/(double)(userWin+userLose) * (double)100;

    int anotherWin;
    int anotherLose;
    double anotherPercent;
    SendRequest sendMsg;
    SendRequestGame anotherPlayer;

    int responseValue = 0;

    window1 = makeFirstWindow();
    window2 = makeSecondWindow();

    if(userWin == 0 && userLose == 0)
        percent = 0;
    mvwprintw(window1, 18/3, 10, "PLAYER1 ID : ");
    mvwprintw(window1, 18/3, 23, currentLoginUser.id);
    wattron(window1, A_BOLD);
    mvwprintw(window1, 18/3+2, 15, "STATISTICS");
    wattroff(window1, A_BOLD);
    mvwprintw(window1, 18/3+4, 7, "WIN : %d / LOSE : %d (%0.3f%)", userWin, userLose, percent);

    mvwprintw(window1, 18/3, 50, "PLAYER2 ID : ");
    wattron(window1, A_BOLD);
    mvwprintw(window1, 18/3+2, 55, "STATISTICS");
    wattroff(window1, A_BOLD);
    mvwprintw(window1, 18/3+4, 47, "WIN :   / LOSE :   (     %)");

    curs_set(0);

    mvwprintw(window2, 3, 36, "WAITING");
    
    wrefresh(window1);
    wrefresh(window2);

    sendMsg.flag = PLAY_GAME;
    send(clientfd, &sendMsg, sizeof(sendMsg), 0);
    recv(clientfd, &anotherPlayer, sizeof(anotherPlayer), 0);

    if(anotherPlayer.anotherUser.win > 100)
        anotherUser = previousUser;
    else
        anotherUser = anotherPlayer.anotherUser;
    playerNum = anotherPlayer.flag;
    
    anotherWin = anotherUser.win;
    anotherLose = anotherUser.lose;
    anotherPercent = (double)anotherWin/(double)(anotherWin+anotherLose) * (double)100;
    if(anotherWin == 0 && anotherLose == 0)
        anotherPercent = 0;

    if(playerNum == PLAYER2){
        // PLAYER2
        mvwprintw(window1, 18/3, 10, "PLAYER1 ID :                 ");  // erase
        mvwprintw(window1, 18/3, 23, anotherUser.id);
        wattron(window1, A_BOLD);
        mvwprintw(window1, 18/3+2, 15, "STATISTICS");
        wattroff(window1, A_BOLD);
        mvwprintw(window1, 18/3+4, 7, "WIN : %d / LOSE : %d (%0.3f%)", anotherWin, anotherLose, anotherPercent);

        mvwprintw(window1, 18/3, 50, "PLAYER2 ID : ");
        mvwprintw(window1, 18/3, 63, currentLoginUser.id);
        wattron(window1, A_BOLD);
        mvwprintw(window1, 18/3+2, 55, "STATISTICS");
        wattroff(window1, A_BOLD);
        mvwprintw(window1, 18/3+4, 47, "WIN : %d / LOSE : %d (%0.3f%)", userWin, userLose, percent);
    }
    else{
        // PLAYER1
        mvwprintw(window1, 18/3, 50, "PLAYER2 ID : ");
        mvwprintw(window1, 18/3, 63, anotherUser.id);
        wattron(window1, A_BOLD);
        mvwprintw(window1, 18/3+2, 55, "STATISTICS");
        wattroff(window1, A_BOLD);
        mvwprintw(window1, 18/3+4, 47, "WIN : %d / LOSE : %d (%0.3f%)", anotherWin, anotherLose, anotherPercent);
    }

    wrefresh(window1);
    wrefresh(window2);
    
    mvwprintw(window2, 3, 36, "       ");       // erase
    wattron(window2, A_REVERSE);
    mvwprintw(window2, 3, 39, "OK");
    wattroff(window2, A_REVERSE);

    wrefresh(window1);
    wrefresh(window2);

    sendMsg.flag = GAME_START;
    while(ch = getch()){
        if(ch == 10){
            // send server
            send(clientfd, &sendMsg, sizeof(sendMsg), 0);
            recv(clientfd, &responseValue, sizeof(int), 0);

            mvwprintw(window1, 6, 54, "456789");
            wrefresh(window1);
            mvwprintw(window1, 6, 54, "      ");
            wrefresh(window1);
            curs_set(2);
            delwin(window1);
            delwin(window2);
            windowGame();
        }
    }
}

void windowWithdrawal(void){
    WINDOW *window1;
    WINDOW *window2;
    char *menu[] = {
        "WITHDRAWAL",
        "BACK"
    };
    int ch;
    int hightlight = 0;
    int menuNum = sizeof(menu) / sizeof(char*); // 2
    char smsg[] = ">>> Withdrawal Success! (Press any key...)";
    char fmsg[] = ">>> Withdrawal Fail, Please try again! (Press any key...)";
    int i = 0;

    SendRequest sendMsg = { 0x00, };
    int responseValue = 0;
    myNum = 2;
    anotherNum = 2;

    window1 = makeFirstWindow();
    window2 = makeSecondWindow();

    wattron(window1, A_BOLD);
    mvwprintw(window1, 18/3, 80/2 - 5, "WITHDRAWAL");
    wattroff(window1, A_BOLD);
    mvwprintw(window1, 18/3+3, 80/2-10, "PLAYER ID : ");
    mvwprintw(window1, 18/3+3, 42, currentLoginUser.id);
    mvwprintw(window1, 18/3+5, 80/2-14, "PASSWORD : ");

    wrefresh(window2);
    printMainMenu(window2, hightlight, menu, menuNum, 50);
    
    wrefresh(window1);
    wrefresh(window2);

    memset(sendMsg.id, '\0', MAX_ID_PASS_SIZE + 1);
    memset(sendMsg.pass, '\0', MAX_ID_PASS_SIZE + 1);
    sendMsg.flag = WITHDRAWAL;

    strcpy(sendMsg.id, currentLoginUser.id);
    // get password
    currentY = 11;
    currentX = 37;
    move(currentY, currentX);
    while((ch = getch()) != '\n'){
        if(ch == 127){      
            // KEY_BACKSPACE
            if(currentX == 37)
                continue;

            move(currentY, --currentX);
            mvwprintw(window1, currentY, currentX, " ");
            wrefresh(window1);
            memset(sendMsg.pass + --i, '\0', 1);
            continue;
        }
        if(ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN)
            // ignore
            continue;
        sendMsg.pass[i++] = ch;
        mvwprintw(window1, currentY, currentX++, "*");

        if(i >= MAX_ID_PASS_SIZE)
            break;
        wrefresh(window1);
    }
    i = 0;

    hightlight = 1;
    printMainMenu(window2, hightlight, menu, menuNum, 50);
    curs_set(0);

    while(ch = getch()){
        if(ch == KEY_LEFT){         // User hit left direction
            if(hightlight == 1)
                hightlight = menuNum;
            else
                hightlight--;
        }
        else if(ch == KEY_RIGHT){   // User hit right direction
            if(hightlight == menuNum)
                hightlight = 1;
            else
                hightlight++;
        }
        else if(ch == 10)           // User hit enter
            break;
        printMainMenu(window2, hightlight, menu, menuNum, 50);
    }
    curs_set(2);

    if(hightlight == 1){
        // WITHDRAWAL
        send(clientfd, &sendMsg, sizeof(sendMsg), 0);
        recv(clientfd, &responseValue, sizeof(int), 0);
        if(responseValue == 1){
            // Success
            mvwprintw(window2, 5, 0, smsg);

            wrefresh(window2);
            getch();
            delwin(window1);
            delwin(window2);
            windowMainBefore();
        }
        else{
            // Fail
            mvwprintw(window2, 5, 0, fmsg);

            wrefresh(window2);
            getch();
            delwin(window1);
            delwin(window2);
            
            windowWithdrawal();
        }
        
    }
    else if(hightlight == 2){
        // BACK
        delwin(window1);
        delwin(window2);
        windowMainAfter();
    }
}

void windowGame(void){
    WINDOW* window1;
    WINDOW* window2;
    int ch;
    int gameBoard[6][6];
    int hightlight[6][6];
    int a_currentY;
    int a_currentX;

    /* Add project2 */
    int winHightlight = 0;
    char *menu[] = {
        "NEXT TURN",
        "REGAME",
        "EXIT"
    };
    char *finishMenu[] = {
        "REGAME",
        "EXIT"
    };
    int status = 0;
    int set = 0;
    GameStatus msg;
    int i, j;
    int returnValue;
    int drawStatus;

    if (has_colors() == FALSE) {
        puts("Terminal does not support colors!");
        endwin();
        exit(-1);
    } 
    else {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_WHITE); // font, screen
        init_pair(2, COLOR_WHITE, COLOR_MAGENTA);
    }
    refresh();
    window1 = newwin(24, 60, 0, 0); // height, width, start y, start x
    window2 = newwin(24, 20, 0, 60);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));
    
    myNum = 2;
    anotherNum = 2;
    printGamePlayerStatus(window2, myNum, anotherNum, PLAYER1);
    printGameMenuAllUnderline(window2);

    initArray(gameBoard);   // init 0
    initArray(hightlight);  // init 0

    a_currentY = 2;
    a_currentX = 2;

    gameBoard[2][2] = O;    // 1
    gameBoard[2][3] = X;    // 2
    gameBoard[3][2] = X;
    gameBoard[3][3] = O;
    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;  //1

    currentY = 6;
    currentX = 17;
    
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    mvwprintw(window1, currentY++, currentX, "|   |   |   |   |   |   |");
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    mvwprintw(window1, currentY++, currentX, "|   |   |   |   |   |   |");
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    mvwprintw(window1, currentY++, currentX, "|   |   |   |   |   |   |");
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    mvwprintw(window1, currentY++, currentX, "|   |   |   |   |   |   |");
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    mvwprintw(window1, currentY++, currentX, "|   |   |   |   |   |   |");
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    mvwprintw(window1, currentY++, currentX, "|   |   |   |   |   |   |");
    mvwprintw(window1, currentY++, currentX, "+---+---+---+---+---+---+");
    
    printGame(window1, hightlight, gameBoard);
    curs_set(0);

    wrefresh(window1);
    wrefresh(window2);

    // set player doll num
    if(playerNum == PLAYER1){
        msg.player1Num = myNum;
        msg.player2Num = 2;
        set = O;
    }
    else if(playerNum == PLAYER2){
        msg.player2Num = myNum;
        msg.player1Num = 2;
        set = X;
    }

    // set player's gameborad
    for(i = 0; i < 6; i++){
        for(j = 0; j < 6; j++){
            msg.gameBoard[i][j] = gameBoard[i][j];
        }
    }
    send(clientfd, &msg, sizeof(msg), 0);
    myNum = 2;
    anotherNum = 2;
    while(1){
        recv(clientfd, &msg, sizeof(msg), 0);

        if(msg.flag == GAME_MOVE){
            hightlight[a_currentY][a_currentX] = 0;
            a_currentX = msg.currentX;
            a_currentY = msg.currentY;
            hightlight[a_currentY][a_currentX] = 1;
            
            printGame(window1, hightlight, gameBoard);
            continue;
        }

        /* Draw Board first */
        for(i = 0; i < 6; i++){
            for(j = 0; j < 6; j++){
                gameBoard[i][j] = msg.gameBoard[i][j];
            }
        }
        if(playerNum == PLAYER1){
            myNum = msg.player1Num;
            anotherNum = msg.player2Num;
        }
        else if(playerNum == PLAYER2){
            myNum = msg.player2Num;
            anotherNum = msg.player1Num;
        }
        
        printGame(window1, hightlight, gameBoard);
        printGamePlayerStatus(window2, myNum, anotherNum, msg.currentPlayer);

        if(msg.status == WIN_PLAYER1 || msg.status == WIN_PLAYER2 || msg.status == DRAW){
            if(msg.status == WIN_PLAYER1){
                
                hightlight[a_currentY][a_currentX] = 0;
                printGame(window1, hightlight, gameBoard);
                mvwprintw(window2, 12, 6, "1P Win!    ");
                wrefresh(window2);
                if(playerNum == PLAYER1){
                    // WINNER
                    currentLoginUser.win++;
                    recv(clientfd, &msg, sizeof(msg), 0);
                    // EXIT OR REGAME use flag
                    if(msg.flag == GAME_REGAME){
                        myNum = 2;
                        anotherNum = 2;
                        curs_set(2);
                        delwin(window1);
                        delwin(window2);
                        windowGame();
                    }
                    else if(msg.flag == GAME_EXIT){
                        previousUser = anotherUser;
                        curs_set(2);
                        delwin(window1);
                        delwin(window2);
                        windowMainAfter();
                    }
                }
                else{
                    // LOSER
                    currentLoginUser.lose++;
                    winHightlight = 1;
                    printGameMenu(window2, winHightlight, finishMenu, 2);
                    while(ch = getch()){
                        if(ch == KEY_UP){
                            if(winHightlight == 1)
                                winHightlight = 2;
                            else
                                winHightlight--;
                        }
                        else if(ch == KEY_DOWN){
                            if(winHightlight == 2)
                                winHightlight = 1;
                            else
                                winHightlight++;
                        }
                        else if(ch == 10){
                            // ENTER
                            if(winHightlight == 1){
                                // REGAME
                                msg.flag = GAME_REGAME;
                                msg.player1Num = 2;
                                msg.player2Num = 2;
                                myNum = 2;
                                anotherNum = 2;
                                send(clientfd, &msg, sizeof(msg), 0);
                                curs_set(2);
                                delwin(window1);
                                delwin(window2);
                                windowGame();
                            }
                            else if(winHightlight == 2){
                                // EXIT
                                msg.flag = GAME_EXIT;
                                send(clientfd, &msg, sizeof(msg), 0);
                                previousUser = anotherUser;

                                curs_set(2);
                                delwin(window1);
                                delwin(window2);
                                windowMainAfter();
                            }
                        }
                        printGameMenu(window2, winHightlight, finishMenu, 2);
                    }
                }
            }
            else if(msg.status == WIN_PLAYER2){
                hightlight[a_currentY][a_currentX] = 0;
                printGame(window1, hightlight, gameBoard);
                mvwprintw(window2, 12, 6, "2P Win!    ");
                wrefresh(window2);
                if(playerNum == PLAYER2){
                    // WINNER
                    currentLoginUser.win++;
                    recv(clientfd, &msg, sizeof(msg), 0);
                    // EXIT OR REGAME use flag
                    if(msg.flag == GAME_REGAME){
                        myNum = 2;
                        anotherNum = 2;
                        curs_set(2);
                        delwin(window1);
                        delwin(window2);
                        windowGame();
                    }
                    else if(msg.flag == GAME_EXIT){
                        previousUser = anotherUser;
                        curs_set(2);
                        delwin(window1);
                        delwin(window2);
                        windowMainAfter();
                    }
                }
                else{
                    // LOSER
                    currentLoginUser.lose++;
                    winHightlight = 1;
                    printGameMenu(window2, winHightlight, finishMenu, 2);
                    while(ch = getch()){
                        if(ch == KEY_UP){
                            if(winHightlight == 1)
                                winHightlight = 2;
                            else
                                winHightlight--;
                        }
                        else if(ch == KEY_DOWN){
                            if(winHightlight == 2)
                                winHightlight = 1;
                            else
                                winHightlight++;
                        }
                        else if(ch == 10){
                            // ENTER
                            if(winHightlight == 1){
                                // REGAME
                                msg.flag = GAME_REGAME;
                                msg.player1Num = 2;
                                msg.player2Num = 2;
                                myNum = 2;
                                anotherNum = 2;
                                send(clientfd, &msg, sizeof(msg), 0);
                                curs_set(2);
                                delwin(window1);
                                delwin(window2);
                                windowGame();
                            }
                            else if(winHightlight == 2){
                                // EXIT
                                msg.flag = GAME_EXIT;
                                send(clientfd, &msg, sizeof(msg), 0);
                                previousUser = anotherUser;

                                curs_set(2);
                                delwin(window1);
                                delwin(window2);
                                windowMainAfter();
                            }
                        }
                        printGameMenu(window2, winHightlight, finishMenu, 2);
                    }
                }
            }
            else if(msg.status == DRAW){
                hightlight[a_currentY][a_currentX] = 0;
                printGame(window1, hightlight, gameBoard);
                mvwprintw(window2, 12, 6, "  DRAW!    ");
                wrefresh(window2);

                winHightlight = 1;
                printGameMenu(window2, winHightlight, finishMenu, 2);
                while(ch = getch()){
                    if(ch == KEY_UP){
                        if(winHightlight == 1)
                            winHightlight = 2;
                        else
                            winHightlight--;
                    }
                    else if(ch == KEY_DOWN){
                        if(winHightlight == 2)
                            winHightlight = 1;
                        else
                            winHightlight++;
                    }
                    else if(ch == 10){
                        // ENTER
                        if(winHightlight == 1){
                            // REGAME
                            drawStatus = GAME_REGAME;
                            send(clientfd, &drawStatus, sizeof(int), 0);
                            break;
                        }
                        else if(winHightlight == 2){
                            // EXIT
                            drawStatus = GAME_EXIT;
                            send(clientfd, &drawStatus, sizeof(int), 0);
                            break;
                        }
                    }
                    printGameMenu(window2, winHightlight, finishMenu, 2);
                }
                recv(clientfd, &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    // REGAME
                    myNum = 2;
                    anotherNum = 2;
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowGame();
                }
                else if(msg.flag == GAME_EXIT){
                    // EXIT
                    previousUser = anotherUser;

                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowMainAfter();
                }
            }
        }
        if(msg.flag == GAME_REGAME){
            myNum = 2;
            anotherNum = 2;
            curs_set(2);
            delwin(window1);
            delwin(window2);
            windowGame();
        }
        else if(msg.flag == GAME_EXIT){
            previousUser = anotherUser;
            curs_set(2);
            delwin(window1);
            delwin(window2);
            windowMainAfter();
        }
        
        while(ch = getch()){
            if(ch == KEY_LEFT){
                hightlight[a_currentY][a_currentX] = 0;
                if(a_currentX == 0){
                    if(a_currentY == 0){
                        a_currentY = 5;
                        a_currentX = 5;
                    }
                    else{
                        a_currentY--;
                        a_currentX = 5;
                    }
                }
                else
                    a_currentX--;
                hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                printGame(window1, hightlight, gameBoard);

                msg.flag = GAME_MOVE;
                msg.currentX = a_currentX;
                msg.currentY = a_currentY;
                send(clientfd, &msg, sizeof(msg), 0);
            }
            else if(ch == KEY_RIGHT){
                hightlight[a_currentY][a_currentX] = 0;
                if(a_currentX == 5){
                    if(a_currentY == 5){
                        a_currentY = 0;
                        a_currentX = 0;
                    }
                    else{
                        a_currentY++;
                        a_currentX = 0;
                    }
                }
                else
                    a_currentX++;
                hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                printGame(window1, hightlight, gameBoard);

                msg.flag = GAME_MOVE;
                msg.currentX = a_currentX;
                msg.currentY = a_currentY;
                send(clientfd, &msg, sizeof(msg), 0);
            }
            else if(ch == KEY_UP){
                hightlight[a_currentY][a_currentX] = 0;
                if(a_currentY == 0){
                    a_currentY = 5;
                }
                else
                    a_currentY--;
                hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                printGame(window1, hightlight, gameBoard);

                msg.flag = GAME_MOVE;
                msg.currentX = a_currentX;
                msg.currentY = a_currentY;
                send(clientfd, &msg, sizeof(msg), 0);
            }
            else if(ch == KEY_DOWN){
                hightlight[a_currentY][a_currentX] = 0;
                if(a_currentY == 5){
                    a_currentY = 0;
                }
                else
                    a_currentY++;
                hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                printGame(window1, hightlight, gameBoard);

                msg.flag = GAME_MOVE;
                msg.currentX = a_currentX;
                msg.currentY = a_currentY;
                send(clientfd, &msg, sizeof(msg), 0);
            }
            else if(ch == 10){
                if((gameBoard[a_currentY][a_currentX] == O) || (gameBoard[a_currentY][a_currentX] == X))
                    // Can't set rock
                    continue;

                returnValue = checkBoard(gameBoard, a_currentY, a_currentX);
                if(returnValue  != 1)
                    // Can't set rock
                    continue;

                myNum++;
                gameBoard[a_currentY][a_currentX] = set;
                printGame(window1, hightlight, gameBoard);
                if(playerNum == PLAYER1)
                    printGamePlayerStatus(window2, myNum, anotherNum, PLAYER2);
                else if(playerNum == PLAYER2)
                    printGamePlayerStatus(window2, myNum, anotherNum, PLAYER1);

                // send
                if(playerNum == PLAYER1){
                    msg.player1Num = myNum;
                    msg.player2Num = anotherNum;
                }
                else if(playerNum == PLAYER2){
                    msg.player2Num = myNum;
                    msg.player1Num = anotherNum;
                }

                for(i = 0; i < 6; i++){
                    for(j = 0; j < 6; j++){
                        msg.gameBoard[i][j] = gameBoard[i][j];
                    }
                }
                msg.flag = 0;
                send(clientfd, &msg, sizeof(msg), 0);
                break;
            }
            else if(ch == 'n' || ch == 'N'){
                winHightlight = 1;

                hightlight[a_currentY][a_currentX] = 0;
                printGame(window1, hightlight, gameBoard);
                printGameMenu(window2, winHightlight, menu, 3);

                status = gameMenuInput(window1, window2, winHightlight, menu);

                if(status == 1){
                    // RETURN TO GAME
                    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;

                    printGameMenuAllUnderline(window2);

                    wrefresh(window2);
                    printGame(window1, hightlight, gameBoard);
                }
                else if(status == 2){
                    // NEXT TURN
                    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                    printGame(window1, hightlight, gameBoard);
                    if(playerNum == PLAYER1)
                        printGamePlayerStatus(window2, myNum, anotherNum, PLAYER2);
                    else if(playerNum == PLAYER2)
                        printGamePlayerStatus(window2, myNum, anotherNum, PLAYER1);
                    printGameMenuAllUnderline(window2);
                    wrefresh(window2);
                    
                    msg.flag = GAME_NEXTTURN;
                    if(playerNum == PLAYER1){
                        msg.player1Num = myNum;
                        msg.player2Num = anotherNum;
                    }
                    else if(playerNum == PLAYER2){
                        msg.player2Num = myNum;
                        msg.player1Num = anotherNum;
                    }

                    for(i = 0; i < 6; i++){
                        for(j = 0; j < 6; j++){
                            msg.gameBoard[i][j] = gameBoard[i][j];
                        }
                    }
                    send(clientfd, &msg, sizeof(msg), 0);
                    break;
                }
                else if(status == 3){
                    // REGAME
                    msg.flag = GAME_REGAME;
                    msg.player1Num = 2;
                    msg.player2Num = 2;
                    myNum = 2;
                    anotherNum = 2;
                    send(clientfd, &msg, sizeof(msg), 0);
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowGame();
                }
                else if(status == 4){
                    // EXIT
                    msg.flag = GAME_EXIT;
                    send(clientfd, &msg, sizeof(msg), 0);
                    previousUser = anotherUser;
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowMainAfter();
                }
            }
            else if(ch == 'r' || ch == 'R'){
                winHightlight = 2;

                hightlight[a_currentY][a_currentX] = 0;
                printGame(window1, hightlight, gameBoard);
                printGameMenu(window2, winHightlight, menu, 3);

                status = gameMenuInput(window1, window2, winHightlight, menu);

                if(status == 1){
                    // RETURN TO GAME
                    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;

                    printGameMenuAllUnderline(window2);

                    wrefresh(window2);
                    printGame(window1, hightlight, gameBoard);
                }
                else if(status == 2){
                    // NEXT TURN
                    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                    printGame(window1, hightlight, gameBoard);
                    if(playerNum == PLAYER1)
                        printGamePlayerStatus(window2, myNum, anotherNum, PLAYER2);
                    else if(playerNum == PLAYER2)
                        printGamePlayerStatus(window2, myNum, anotherNum, PLAYER1);
                    printGameMenuAllUnderline(window2);
                    wrefresh(window2);
                    
                    msg.flag = GAME_NEXTTURN;
                    if(playerNum == PLAYER1){
                        msg.player1Num = myNum;
                        msg.player2Num = anotherNum;
                    }
                    else if(playerNum == PLAYER2){
                        msg.player2Num = myNum;
                        msg.player1Num = anotherNum;
                    }

                    for(i = 0; i < 6; i++){
                        for(j = 0; j < 6; j++){
                            msg.gameBoard[i][j] = gameBoard[i][j];
                        }
                    }
                    send(clientfd, &msg, sizeof(msg), 0);
                    break;
                }
                else if(status == 3){
                    // REGAME
                    msg.flag = GAME_REGAME;
                    msg.player1Num = 2;
                    msg.player2Num = 2;
                    myNum = 2;
                    anotherNum = 2;
                    send(clientfd, &msg, sizeof(msg), 0);
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowGame();
                }
                else if(status == 4){
                    // EXIT
                    msg.flag = GAME_EXIT;
                    send(clientfd, &msg, sizeof(msg), 0);
                    previousUser = anotherUser;
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowMainAfter();
                }
            }
            else if(ch == 'x' || ch == 'X'){
                winHightlight = 3;

                hightlight[a_currentY][a_currentX] = 0;
                printGame(window1, hightlight, gameBoard);
                printGameMenu(window2, winHightlight, menu, 3);

                status = gameMenuInput(window1, window2, winHightlight, menu);

                if(status == 1){
                    // RETURN TO GAME
                    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;

                    printGameMenuAllUnderline(window2);

                    wrefresh(window2);
                    printGame(window1, hightlight, gameBoard);
                }
                else if(status == 2){
                    // NEXT TURN
                    hightlight[a_currentY][a_currentX] = HIGHTLIGHT;
                    printGame(window1, hightlight, gameBoard);
                    if(playerNum == PLAYER1)
                        printGamePlayerStatus(window2, myNum, anotherNum, PLAYER2);
                    else if(playerNum == PLAYER2)
                        printGamePlayerStatus(window2, myNum, anotherNum, PLAYER1);
                    printGameMenuAllUnderline(window2);
                    wrefresh(window2);
                    
                    msg.flag = GAME_NEXTTURN;
                    if(playerNum == PLAYER1){
                        msg.player1Num = myNum;
                        msg.player2Num = anotherNum;
                    }
                    else if(playerNum == PLAYER2){
                        msg.player2Num = myNum;
                        msg.player1Num = anotherNum;
                    }

                    for(i = 0; i < 6; i++){
                        for(j = 0; j < 6; j++){
                            msg.gameBoard[i][j] = gameBoard[i][j];
                        }
                    }
                    send(clientfd, &msg, sizeof(msg), 0);
                    break;
                }
                else if(status == 3){
                    // REGAME
                    msg.flag = GAME_REGAME;
                    msg.player1Num = 2;
                    msg.player2Num = 2;
                    myNum = 2;
                    anotherNum = 2;
                    send(clientfd, &msg, sizeof(msg), 0);
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowGame();
                }
                else if(status == 4){
                    // EXIT
                    msg.flag = GAME_EXIT;
                    send(clientfd, &msg, sizeof(msg), 0);
                    previousUser = anotherUser;
                    curs_set(2);
                    delwin(window1);
                    delwin(window2);
                    windowMainAfter();
                }
            }
        }
    }
    endwin();
}

int checkBoard(int (*gameBoard)[6], int currentY, int currentX){
    int myRock;
    int anotherRock;
    int i, j;
    int index = 0;
    int index_x = 0;
    int found = 0;
    int returnValue = 0;

    if(playerNum == PLAYER1){
        myRock = O;
        anotherRock = X;
    }
    else{
        myRock = X;
        anotherRock = O;
    }
    
    if(currentY < 5){
        if(gameBoard[currentY + 1][currentX] == anotherRock){
            // DOWN has another rock
            for(i = currentY + 1; i < 6; i++){
                if(gameBoard[i][currentX] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
                if(gameBoard[i][currentX] == myRock){
                    // Found my rock
                    index = i;
                    found = 1;
                    break;
                }
            }
            if(found == 1){
                for(i = currentY + 1; i < index; i++){
                    // change another rock to my rock
                    gameBoard[i][currentX] = myRock;
                    myNum++;
                    anotherNum--;
                }
                returnValue++;
                index = 0;
                found = 0;
            }
        }
    }

    if(currentY > 0){
        if(gameBoard[currentY - 1][currentX] == anotherRock){
            // UP has another rock
            for(i = currentY - 1; i > -1; i--){
                if(gameBoard[i][currentX] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
                if(gameBoard[i][currentX] == myRock){
                    // Found my rock
                    index = i;
                    found = 1;
                    break;
                }
            }
            if(found == 1){
                for(i = currentY - 1; i > index; i--){
                    // change another rock to my rock
                    gameBoard[i][currentX] = myRock;
                    myNum++;
                    anotherNum--;
                }
                returnValue++;
                index = 0;
                found = 0;
            }
        }
    }

    if(currentX < 5){
        if(gameBoard[currentY][currentX + 1] == anotherRock){
            // RIGHT has another rock
            for(i = currentX + 1; i < 6; i++){
                if(gameBoard[currentY][i] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
                if(gameBoard[currentY][i] == myRock){
                    // Found my rock
                    index = i;
                    found = 1;
                    break;
                }
            }
            if(found == 1){
                for(i = currentX + 1; i < index; i++){
                    // change another rock to my rock
                    gameBoard[currentY][i] = myRock;
                    myNum++;
                    anotherNum--;
                }
                returnValue++;
                index = 0;
                found = 0;
            }
        }
    }

    if(currentX > 0){
        if(gameBoard[currentY][currentX - 1] == anotherRock){
            // LEFT has another rock
            for(i = currentX - 1; i > -1; i--){
                if(gameBoard[currentY][i] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
                if(gameBoard[currentY][i] == myRock){
                    // Found my rock
                    index = i;
                    found = 1;
                    break;
                }
            }
            if(found == 1){
                for(i = currentX - 1; i > index; i--){
                    // change another rock to my rock
                    gameBoard[currentY][i] = myRock;
                    myNum++;
                    anotherNum--;
                }
                returnValue++;
                index = 0;
                found = 0;
            }
        }
    }

    if(gameBoard[currentY + 1][currentX + 1] == anotherRock){
        // DOWN-RIGHT has another rock
        for(i = currentY + 1, j = currentX + 1; i < 6 && j < 6; i++, j++){
            if(gameBoard[i][j] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
            if(gameBoard[i][j] == myRock){
                // Found my rock
                index = i;
                found = 1;
                break;
            }
        }
        if(found == 1){
            for(i = currentY + 1, j = currentX + 1; i < index; i++, j++){
                // change another rock to my rock
                gameBoard[i][j] = myRock;
                myNum++;
                anotherNum--;
            }
            returnValue++;
            index = 0;
            found = 0;
        }
    }

    if(gameBoard[currentY + 1][currentX - 1] == anotherRock){
        // DOWN-LEFT has another rock
        for(i = currentY + 1, j = currentX - 1; i < 6 && j > -1; i++, j--){
            if(gameBoard[i][j] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
            if(gameBoard[i][j] == myRock){
                // Found my rock
                index = i;
                found = 1;
                break;
            }
        }
        if(found == 1){
            for(i = currentY + 1, j = currentX - 1; i < index; i++, j--){
                // change another rock to my rock
                gameBoard[i][j] = myRock;
                myNum++;
                anotherNum--;
            }
                
            returnValue++;
            index = 0;
            found = 0;
        }
    }

    if(gameBoard[currentY - 1][currentX - 1] == anotherRock){
        // UP-LEFT has another rock
        for(i = currentY - 1, j = currentX - 1; i > -1 && j > -1; i--, j--){
            if(gameBoard[i][j] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
            if(gameBoard[i][j] == myRock){
                // Found my rock
                index = i;
                found = 1;
                break;
            }
        }
        if(found == 1){
            for(i = currentY - 1, j = currentX - 1; i > index; i--, j--){
                // change another rock to my rock
                gameBoard[i][j] = myRock;
                myNum++;
                anotherNum--;
            }
                
            returnValue++;
            index = 0;
            found = 0;
        }
    }

    if(gameBoard[currentY - 1][currentX + 1] == anotherRock){
        // UP-RIGHT has another rock
        for(i = currentY - 1, j = currentX + 1; i > -1 && j < 6; i--, j++){
            if(gameBoard[i][j] == 0){
                    // Empty place break
                    found = 0;
                    break;
                }
            if(gameBoard[i][j] == myRock){
                // Found my rock
                index = i;
                found = 1;
                break;
            }
        }
        if(found == 1){
            for(i = currentY - 1, j = currentX + 1; i > index; i--, j++){
                // change another rock to my rock
                gameBoard[i][j] = myRock;
                myNum++;
                anotherNum--;
            }
                
            returnValue++;
            index = 0;
            found = 0;
        }
    }

    if(returnValue > 0){
        // SUCESS!
        return 1;
    }
    else
        return 0;
}

void printGamePlayerStatus(WINDOW* window2, int myNum, int anotherNum, int pn){
    if(pn == PLAYER1){
        if(playerNum == PLAYER1){
            wattron(window2, A_REVERSE);
            mvwprintw(window2, 8, 2, "%s(O) : %d ", currentLoginUser.id, myNum);
            wattroff(window2, A_REVERSE);
            mvwprintw(window2, 9, 2, "%s(X) : %d ", anotherUser.id, anotherNum);
        }
        else if(playerNum == PLAYER2){
            wattron(window2, A_REVERSE);
            mvwprintw(window2, 8, 2, "%s(O) : %d ", anotherUser.id, anotherNum);
            wattroff(window2, A_REVERSE);
            mvwprintw(window2, 9, 2, "%s(X) : %d ", currentLoginUser.id, myNum);
        }
    }
    else if(pn == PLAYER2){
        if(playerNum == PLAYER1){
            mvwprintw(window2, 8, 2, "%s(O) : %d ", currentLoginUser.id, myNum);
            wattron(window2, A_REVERSE);
            mvwprintw(window2, 9, 2, "%s(X) : %d ", anotherUser.id, anotherNum);
            wattroff(window2, A_REVERSE);
        }
        else if(playerNum == PLAYER2){
            mvwprintw(window2, 8, 2, "%s(O) : %d ", anotherUser.id, anotherNum);
            wattron(window2, A_REVERSE);
            mvwprintw(window2, 9, 2, "%s(X) : %d ", currentLoginUser.id, myNum);
            wattroff(window2, A_REVERSE);
        }
    }
    wrefresh(window2);
}

int gameMenuInput(WINDOW* window1, WINDOW* window2, int winHightlight, char* menu[]){
    int ch;
    
    while(ch = getch()){
        if(ch == 'g' || ch == 'G'){
           return 1;
        }
        else if(ch == KEY_UP){
            if(winHightlight == 1)
                winHightlight = 3;
            else
                winHightlight--;
        }
        else if(ch == KEY_DOWN){
            if(winHightlight == 3)
                winHightlight = 1;
            else
                winHightlight++;
        }
        else if(ch == 10){
            // add check hightlight
            if(winHightlight == 1){
                // NEXT TURN
                return 2;
            }
            else if(winHightlight == 2){
                // REGAME
                return 3;
            }
            else if(winHightlight == 3){
                // EXIT
                return 4;
            }
        }
        printGameMenu(window2, winHightlight, menu, 3);
    }
}

void printGameMenuAllUnderline(WINDOW *window2){
    wattron(window2, A_UNDERLINE);
    mvwprintw(window2, 12, 6, "N");
    wattroff(window2, A_UNDERLINE);
    mvwprintw(window2, 12, 7, "EXT TURN");

    wattron(window2, A_UNDERLINE);
    mvwprintw(window2, 14, 6, "R");
    wattroff(window2, A_UNDERLINE);
    mvwprintw(window2, 14, 7, "EGAME");

    mvwprintw(window2, 16, 6, "E");
    wattron(window2, A_UNDERLINE);
    mvwprintw(window2, 16, 7, "X");
    wattroff(window2, A_UNDERLINE);
    mvwprintw(window2, 16, 8, "IT");    
}

void printGameMenu(WINDOW *win, int hightlight, char* menu[], int menuNum){
    int i = 0;
    int x = 6;
    int y = 12;

    if(menuNum == 2)
        // finished menu
        y = 14;

    for(i = 0; i < menuNum; i++){
            if(hightlight == i+1){
                wattron(win, A_REVERSE);
                mvwprintw(win, y, x, "%s", menu[i]);
                wattroff(win, A_REVERSE);
            }
            else
                mvwprintw(win, y, x, "%s", menu[i]);
            y += 2;
    }
    wrefresh(win);
}

void initArray(int (*arr)[6]){
    int i;
    for(i = 0; i < 6; i++){
        memset(arr[i], 0, sizeof(int) * 6);
    }
}

void printGame(WINDOW *win, int (*hightlight)[6], int (*gameBoard)[6]){
    int i;
    int j;
    int hightlightStatus = 0;
    int boardStatus = 0;
    int x = 18;
    int y = 7;

   for(i = 0; i < 6; i++){
       for(j = 0; j <6; j++){
            hightlightStatus = hightlight[i][j];
            boardStatus = gameBoard[i][j];

            if(hightlightStatus == HIGHTLIGHT){
                if(boardStatus == O){
                   // print " O " REVERSE
                   wattron(win, A_REVERSE);
                   mvwprintw(win, y, x, " O ");
                   wattroff(win, A_REVERSE);
                }
                else if(boardStatus == X){
                    // print " X " REVERSE
                    wattron(win, A_REVERSE);
                    mvwprintw(win, y, x, " X ");
                    wattroff(win, A_REVERSE);
                }
                else{
                    // print "   " REVERSE
                    wattron(win, A_REVERSE);
                    mvwprintw(win, y, x, "   ");
                    wattroff(win, A_REVERSE);
                }
            }
            else{
                if(boardStatus == O){
                   // print " O "
                   mvwprintw(win, y, x, " O ");
                }
                else if(boardStatus == X){
                    // print " X "
                    mvwprintw(win, y, x, " X ");
                }
                else{
                    mvwprintw(win, y, x, "   ");
                }
            }
            x += 4;
       }
       x = 18;
       y += 2;
   }
   wrefresh(win);
}
