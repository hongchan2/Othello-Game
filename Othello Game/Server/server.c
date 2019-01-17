/*
 * 학번 : 2015202065
 * 이름 : 윤홍찬
 */

/* Socket Headers */
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* Thread Headers */
#include <pthread.h>

/* System call Headers */
#include <sys/stat.h>
#include <string.h>

/* Socket Flags */
#define MAX_LQ_SIZE 2
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

/* System Call Flags */
#define MAX_ID_PASS_SIZE 32
#define PERMS 0666
#define SEARCH_ID 0
#define SEARCH_ID_AND_PASS 1
#define SEARCH_ID_AND_PASS_OFFSET 2

/* Game Flags */
#define O 1
#define X 2

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

void signalHandler(int signum);

/* System Call Functions */
int signUp(char userId[], char userPass[], User* user);
int signIn(char userId[], char userPass[]);
int withdrawal(char userId[], char userPass[]);
int searchID(int fd, char userId[], char userPass[], int flag);
int checkPass(char userPass[], User* user);

/* Thread Function */
void* runServer(void*);

/* Game Function */
void gameStart(int clientfd);
void updateStatus(int winner);

/* Socket Global var */
int serverfd = 0;
int clientfd = 0;

int loginClient[2];        // Store current client fd
int loginClientCnt = 0;

int gameClient[2];         // Store current game client fd (in order)
int gameClientCnt = 0;
User gameUser[2];           // Store current game User status (in order)
int gameUserCnt = 0;

/* Game Global var */
int statusWait = 0;
int player2Status = 0;
int gameWait = 0;
User Player1;
User Player2;
int gameTerminate = 0;
int draw = 0;

/* System call Global var */
const char* USER_DATA_FILE = "./hongchan";
User currentLoginUser;

int main(int argc, char const *argv[]){
    struct sockaddr_in serverAddr = { 0x00, };
    struct sockaddr_in client1Addr = { 0x00, };
    struct sockaddr_in client2Addr = { 0x00, };
    int client1AddrSize = 0;
    int client2AddrSize = 0;
    pthread_t tid = 0;

    if(argc < 2){
        printf("Usage: %s [port]\n", argv[0]);
        return -1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGPIPE, signalHandler);

    if((serverfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket error!");
        exit(-1);
    }

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(atoi(argv[1]));

    if(bind(serverfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))){
        perror("bind error!");
        exit(-1);
    }

    if(listen(serverfd, MAX_LQ_SIZE) == -1){
        perror("listen error!");
        exit(-1);
    }

    while(1){
        // MAX_CLIENT == 2
        while(loginClientCnt > 1);
        if((clientfd = accept(serverfd, (struct sockaddr*)&client1Addr, &client1AddrSize)) == -1){
            perror("accept error!");
            exit(-1);
        }
        loginClient[loginClientCnt++] = clientfd;
        if(pthread_create(&tid, NULL, runServer, &clientfd) != 0){
            perror("pthread_create error!");
            exit(-1);
        }
        pthread_detach(tid);
        printf("CONNECT: %d\n", loginClientCnt);
    }
}

void* runServer(void* arg){
    int clientfd = *((int*)arg);        // current thread's clientfd
    int responseData = 0;
    SendRequest recvMsg = { 0x00, };
    SendRequestGame sendStatus;
    User currentPlayer;                 // current thread's user status

    memset(&currentPlayer, '\0', sizeof(currentPlayer));

    while(1){
        memset(&recvMsg, '\0', sizeof(recvMsg));
        memset(&responseData, '\0', sizeof(int));

        recv(clientfd, &recvMsg, sizeof(recvMsg), 0);

        if(recvMsg.flag == SIGN_UP){
//printf("[ %s Request : Sign up ]\n", currentPlayer.id);
            responseData = signUp(recvMsg.id, recvMsg.pass, NULL);
            send(clientfd, &responseData, sizeof(int), 0);
        }
        else if(recvMsg.flag == SIGN_IN){
//printf("[ %s Request : Sign in ]\n", currentPlayer.id);
            responseData = signIn(recvMsg.id, recvMsg.pass);

            if(responseData != 1)
                // Fail
                memset(&currentLoginUser.id, 0x00, sizeof(currentLoginUser.id));
            else{
                // Success
                memcpy(&currentPlayer, &currentLoginUser, sizeof(User));
            }
            send(clientfd, &currentLoginUser, sizeof(currentLoginUser), 0);
        }
        else if(recvMsg.flag == WITHDRAWAL){
//printf("[ %s Request : Withdrawal ]\n", currentPlayer.id);
            responseData = withdrawal(recvMsg.id, recvMsg.pass);
            send(clientfd, &responseData, sizeof(int), 0);
        }
        else if(recvMsg.flag == PLAY_GAME){
//printf("[ %s Request : Play Game ]\n", currentPlayer.id);
            statusWait++;
            if(statusWait == 1){
                // player 1
                Player1 = currentPlayer;
                gameClient[gameClientCnt++] = clientfd;
                gameUser[gameUserCnt++] = currentPlayer;

                while((statusWait != 2) && (player2Status != 1));

                sendStatus.flag = PLAYER1;
                sendStatus.anotherUser = Player2;

                send(clientfd, &sendStatus, sizeof(sendStatus), 0);
                recv(clientfd, &recvMsg, sizeof(recvMsg), 0);
                gameWait++;
                
                while(gameWait != 2);
                send(clientfd, &responseData, sizeof(int), 0);
                gameStart(clientfd);

                // UPDATE
                currentPlayer = Player1;
            }
            else if(statusWait == 2){
                // player 2
                Player2 = currentPlayer;
                gameClient[gameClientCnt++] = clientfd;
                gameUser[gameUserCnt++] = currentPlayer;

                sendStatus.flag = PLAYER2;
                sendStatus.anotherUser = Player1;

                send(clientfd, &sendStatus, sizeof(sendStatus), 0);
                player2Status++;

                recv(clientfd, &recvMsg, sizeof(recvMsg), 0);
                gameWait++;

                while(gameWait != 2);
                send(clientfd, &responseData, sizeof(int), 0);

                while(gameTerminate != 1);
                gameTerminate = 0;

                // UPDATE
                currentPlayer = Player2;
            }
        }
        else if(recvMsg.flag == LOGOUT){
//printf("[ %s Request : Logout ]\n", currentPlayer.id);
            memset(&currentPlayer, '\0', sizeof(User));
            
            close(clientfd);
            loginClientCnt--;
            pthread_exit(&responseData);
            // delete array;
        }
        
        else{
            puts("Server ERROR! : Unkwon flag - Terminate Thread");
            loginClientCnt--;
            close(clientfd);
            pthread_exit(&responseData);
        }
    }
    loginClientCnt--;
}

void gameStart(int clientfd){
    // clientfd == PLAYER1 
    int gameBoard[6][6];
    int currentPlayer = PLAYER1;
    int player1Num = 2;
    int player2Num = 2;
    GameStatus msg;
    int draw1 = 0, draw2 = 0;

    recv(gameClient[0], &msg, sizeof(msg), 0); // 1
    recv(gameClient[1], &msg, sizeof(msg), 0); // 2

    draw = 0;
    while(1){
//puts("== Player1 ==");
        msg.currentPlayer = PLAYER1;
        send(gameClient[0], &msg, sizeof(msg), 0);
        //recv(gameClient[0], &msg, sizeof(msg), 0);

        while(1){
            recv(gameClient[0], &msg, sizeof(msg), 0);
            if(msg.flag == GAME_MOVE)
                send(gameClient[1], &msg, sizeof(msg), 0);
            else
                break;
        }
//puts("turn end.");

        // PLAYER1 CHECK
        if(msg.flag == GAME_NEXTTURN){
            msg.flag = 0;
            draw++;
        }
        else{
            draw = 0;
        }
        if(draw == 2){
//puts("game done - DRAW");
            msg.status = DRAW;
            send(gameClient[0], &msg, sizeof(msg), 0);
            send(gameClient[1], &msg, sizeof(msg), 0);

            recv(gameClient[0], &draw1, sizeof(int), 0);
            recv(gameClient[1], &draw2, sizeof(int), 0);

            if(draw1 == GAME_EXIT || draw2 == GAME_EXIT){
                // EXIT
                msg.flag = GAME_EXIT;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                gameWait = 0;
                statusWait = 0;
                player2Status = 0;
                gameTerminate = 1;
                gameClientCnt = 0;
                return;
            }
            if(draw1 == GAME_REGAME || draw2 == GAME_REGAME){
                // REGAME
                msg.flag = GAME_REGAME;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);
                gameStart(clientfd);
                return;
            }
        }
        if(msg.player1Num == 0 || msg.player2Num == 0){
//puts("game done - Zero Rock");
            if(msg.player1Num == 0){
                // player2 Win
                updateStatus(PLAYER2);
                msg.status = WIN_PLAYER2;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
            else if(msg.player2Num == 0){
                // player1 Win
                updateStatus(PLAYER1);
                msg.status = WIN_PLAYER1;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[1], &msg, sizeof(msg), 0);
                send(gameClient[0], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
        }
        if(msg.player1Num + msg.player2Num == 36){
//puts("game done - End of game");
            if(msg.player1Num > msg.player2Num){
                // PLAYER 1 Win
                updateStatus(PLAYER1);
                msg.status = WIN_PLAYER1;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[1], &msg, sizeof(msg), 0);
                send(gameClient[0], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
            else if(msg.player1Num < msg.player2Num){
                // PLAYER 2 Win
                updateStatus(PLAYER2);
                msg.status = WIN_PLAYER2;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
            else{
                // DRAW
                msg.status = DRAW;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[0], &draw1, sizeof(int), 0);
                recv(gameClient[1], &draw2, sizeof(int), 0);

                if(draw1 == GAME_EXIT || draw2 == GAME_EXIT){
                    // EXIT
                    msg.flag = GAME_EXIT;
                    send(gameClient[0], &msg, sizeof(msg), 0);
                    send(gameClient[1], &msg, sizeof(msg), 0);

                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
                if(draw1 == GAME_REGAME || draw2 == GAME_REGAME){
                    // REGAME
                    msg.flag = GAME_REGAME;
                    send(gameClient[0], &msg, sizeof(msg), 0);
                    send(gameClient[1], &msg, sizeof(msg), 0);
                    gameStart(clientfd);
                    return;
                }
            }
        }
        if(msg.flag == GAME_REGAME){
//puts("PLAYER1 REGAME");
            send(gameClient[1], &msg, sizeof(msg), 0);
            gameStart(clientfd);
            return;
        }
        if(msg.flag == GAME_EXIT){
//puts("PLAYER1 EXIT");
            send(gameClient[1], &msg, sizeof(msg), 0);
            gameWait = 0;
            statusWait = 0;
            player2Status = 0;
            gameTerminate = 1;
            gameClientCnt = 0;
            return;
        }

//puts("== Player2 ==");
        msg.currentPlayer = PLAYER2;
        send(gameClient[1], &msg, sizeof(msg), 0);

        while(1){
            recv(gameClient[1], &msg, sizeof(msg), 0);
            if(msg.flag == GAME_MOVE)
                send(gameClient[0], &msg, sizeof(msg), 0);
            else
                break;
        }

        //recv(gameClient[1], &msg, sizeof(msg), 0);
//puts("turn end.");

        // PLAYER2 CHECK
        if(msg.flag == GAME_NEXTTURN){
            msg.flag = 0;
            draw++;
        }
        else{
            draw = 0;
        }
        if(draw == 2){
//puts("game done - DRAW");
            msg.status = DRAW;
            send(gameClient[0], &msg, sizeof(msg), 0);
            send(gameClient[1], &msg, sizeof(msg), 0);

            recv(gameClient[0], &draw1, sizeof(int), 0);
            recv(gameClient[1], &draw2, sizeof(int), 0);

            if(draw1 == GAME_EXIT || draw2 == GAME_EXIT){
                // EXIT
                msg.flag = GAME_EXIT;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                gameWait = 0;
                statusWait = 0;
                player2Status = 0;
                gameTerminate = 1;
                gameClientCnt = 0;
                return;
            }
            if(draw1 == GAME_REGAME || draw2 == GAME_REGAME){
                // REGAME
                msg.flag = GAME_REGAME;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);
                gameStart(clientfd);
                return;
            }
        }
        if(msg.player1Num == 0 || msg.player2Num == 0){
//puts("game done - Zero Rock");
            if(msg.player1Num == 0){
                // player2 Win
                updateStatus(PLAYER2);
                msg.status = WIN_PLAYER2;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
            else if(msg.player2Num == 0){
                // player1 Win
                updateStatus(PLAYER1);
                msg.status = WIN_PLAYER1;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[1], &msg, sizeof(msg), 0);
                send(gameClient[0], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
        }
        if(msg.player1Num + msg.player2Num == 36){
//puts("game done - End of game");
            if(msg.player1Num > msg.player2Num){
                // PLAYER 1 Win
                updateStatus(PLAYER1);
                msg.status = WIN_PLAYER1;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[1], &msg, sizeof(msg), 0);
                send(gameClient[0], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
            else if(msg.player1Num < msg.player2Num){
                // PLAYER 2 Win
                updateStatus(PLAYER2);
                msg.status = WIN_PLAYER2;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);
                if(msg.flag == GAME_REGAME){
                    gameStart(clientfd);
                    return;
                }
                else if(msg.flag == GAME_EXIT){
                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
            }
            else{
                // DRAW
                msg.status = DRAW;
                send(gameClient[0], &msg, sizeof(msg), 0);
                send(gameClient[1], &msg, sizeof(msg), 0);

                recv(gameClient[0], &draw1, sizeof(int), 0);
                recv(gameClient[1], &draw2, sizeof(int), 0);

                if(draw1 == GAME_EXIT || draw2 == GAME_EXIT){
                    // EXIT
                    msg.flag = GAME_EXIT;
                    send(gameClient[0], &msg, sizeof(msg), 0);
                    send(gameClient[1], &msg, sizeof(msg), 0);

                    gameWait = 0;
                    statusWait = 0;
                    player2Status = 0;
                    gameTerminate = 1;
                    gameClientCnt = 0;
                    return;
                }
                if(draw1 == GAME_REGAME || draw2 == GAME_REGAME){
                    // REGAME
                    msg.flag = GAME_REGAME;
                    send(gameClient[0], &msg, sizeof(msg), 0);
                    send(gameClient[1], &msg, sizeof(msg), 0);
                    gameStart(clientfd);
                    return;
                }
            }
        }
        if(msg.flag == GAME_REGAME){
//puts("PLAYER2 REGAME");
            send(gameClient[0], &msg, sizeof(msg), 0);
            gameStart(clientfd);
            return;
        }
        if(msg.flag == GAME_EXIT){
//puts("PLAYER2 EXIT");
            send(gameClient[0], &msg, sizeof(msg), 0);
            gameWait = 0;
            statusWait = 0;
            player2Status = 0;
            gameTerminate = 1;
            gameClientCnt = 0;
            return;
        }
    }

    /* move this variable well!! */
    gameWait = 0;
    statusWait = 0;
    player2Status = 0;
    gameTerminate = 1;
}

void updateStatus(int winner){
    // erase
    withdrawal(Player1.id, Player1.pass);
    withdrawal(Player2.id, Player2.pass);
    if(winner == PLAYER1){
        Player1.win++;
        Player2.lose++;
    }
    else if(winner == PLAYER2){
        Player2.win++;
        Player1.lose++;
    }
    signUp(NULL, NULL, &Player1);
    signUp(NULL, NULL, &Player2);
}

void signalHandler(int signum){
    if(signum == SIGINT){
        close(serverfd);
        close(clientfd);
        exit(-1);
    }
    else if(signum == SIGPIPE){
        puts("Disconnected");
        loginClientCnt--;
        //close(serverfd);
        close(clientfd);
        //exit(-1);
    }
}

int signUp(char userId[], char userPass[], User* update){
    int fd = 0;    // USER_DATA_FILE
    ssize_t wsize = 0;
    User* user = (User*)malloc(sizeof(User));

    fd = open(USER_DATA_FILE, O_CREAT | O_RDWR, PERMS);
    if(fd == -1){
        perror("signUp open error");
        exit(-1);
    }

    memset(user->id, '\0', MAX_ID_PASS_SIZE + 1);
    memset(user->pass, '\0', MAX_ID_PASS_SIZE + 1);

    if(update == NULL){
        user->win = 0;
        user->lose = 0;

        strcpy(user->id, userId);
        strcpy(user->pass, userPass);
    }
    else{
        user->win = update->win;
        user->lose = update ->lose;
        
        strcpy(user->id, update->id);
        strcpy(user->pass, update->pass);
    }
    if(searchID(fd, user->id, NULL, SEARCH_ID) == 1)
        // already exist id! return 0;
        return 0;

    // to append file
    lseek(fd, 0, SEEK_END);
    wsize = write(fd, user, sizeof(User));
    if(wsize == -1){
        perror("signUp write error");
        exit(-1);
    }
    free(user);
    user = NULL;
    close(fd);
    
    return 1;
}

int signIn(char userId[], char userPass[]){
    // search id struct and check password
    int fd = 0;

    fd = open(USER_DATA_FILE, O_CREAT | O_RDWR, PERMS);
    if(fd == -1){
        perror("signIn open error");
        exit(-1);
    }

    if(searchID(fd, userId, userPass, SEARCH_ID_AND_PASS) == 1){
        // correct ID & password & store login status
        close(fd);
        return 1;
    }
    else{
        // fail to login
        close(fd);
        return 0;
    }
}

int withdrawal(char userId[], char userPass[]){
    int fd = 0;
    off_t offset = 0;
    ssize_t wsize = 0;
    User* user;

    fd = open(USER_DATA_FILE, O_CREAT | O_RDWR, PERMS);
    if(fd == -1){
        perror("signIn open error");
        exit(-1);
    }

   // store offset
   //offset = searchID(fd, currentLoginUser.id, userPass, SEARCH_ID_AND_PASS_OFFSET);
   offset = searchID(fd, userId, userPass, SEARCH_ID_AND_PASS_OFFSET);
    
    if(offset > -1){
        // delete user data

        user = (User*)malloc(sizeof(User));
        memset(user, '\0', sizeof(User));
        wsize = write(fd, user, sizeof(User));
        if(wsize == -1){
            perror("withdrawal write error!");
            exit(-1);
        }
        close(fd);
        return 1;
    }
    else{
        close(fd);
        return 0;
    }
}

int searchID(int fd, char userId[], char userPass[], int flag){
    // if flag == SEARCH_ID, only search userID
    // if flag == SEARCH_ID_AND_PASS, search userID and check password
    // if flag == SEARCH_ID_AND_PASS_OFFSET, check ID and password finally return offset
    ssize_t rsize = 0;
    struct stat buf;
    off_t offset = 0;
    User* user = (User*)malloc(sizeof(User));

    fstat(fd, &buf);
    if(buf.st_size == 0)
        // empty file
        return 0;
    
    do{
        memset(user, '\0', sizeof(User));
        rsize = read(fd, user, sizeof(User));
        if(rsize == -1){
            perror("searchID read error");
            exit(-1);
        }
        if(strcmp(user->id, userId) == 0)
            if(flag == SEARCH_ID)
                // found id, return 1
                return 1;
            else if (flag == SEARCH_ID_AND_PASS){
                // now check password
                if(checkPass(userPass, user) == 1){
                    // correct password
                    currentLoginUser = *user;
                    return 1;
                }
                else   
                    // incorrect password
                    return 0;
            }
            else if(flag == SEARCH_ID_AND_PASS_OFFSET){
                if(checkPass(userPass, user) == 1){
                    // move offset

                    offset = lseek(fd, -sizeof(User), SEEK_CUR);
                    return offset;
                }
                else   
                    // incorrect password
                    return -1;
            }
    }while(rsize > 0);

    free(user);
    user = NULL;
    return -1;
}

int checkPass(char userPass[], User* user){
    // check password
    if(strcmp(user->pass, userPass) == 0)
        return 1;
    else
        return 0;
}
