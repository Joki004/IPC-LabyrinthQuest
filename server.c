//
// Created by joki-004 on 10/25/22.
//

#include "server.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <locale.h>
#include <ncurses.h>
#include <fcntl.h>
#include <wchar.h>
#include <time.h>
#include <signal.h>
#include <math.h>

int CLI_CO = 0;
#define MAX_CONNECTION 4
#define MAX_BEAST 20
#define DISTANCE_FROM_SCREEN_BORDER 3
#define T 5
#define PORT 8989
struct player_t players[4];
struct beast_t beasts[MAX_BEAST];
struct death coins;
pthread_t counter,keyListenner,beast[20],playersThread[4];
int beastInGame=0;
int RoundsCounter = 0;
char **map;
char key= 'p';
sem_t sem,sem_b;
int campCoordinatesX, campCoordinatesY;
int stateOfTheGame = 0;




int main()
{
    setlocale(LC_ALL,"");
    initscr();
    if(has_colors() == TRUE) start_color();
    int j =0;
    while(j<MAX_CONNECTION){
        players[j].is_active=0;
        j++;
    }
    sem_init(&sem,0,0);
    sem_init(&sem_b,0,1);
    createColision(&coins, 20);
    init_pair(1,COLOR_WHITE,5);
    init_pair(2,COLOR_BLACK,COLOR_YELLOW);
    init_pair(3,COLOR_RED,COLOR_BLACK);
    init_pair(4,COLOR_WHITE,COLOR_GREEN);

    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    int opt = 1;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    signal(SIGPIPE, SIG_IGN);

    if (setsockopt(serverSocket, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    int check =bind(serverSocket, (struct sockaddr*)&serverAddr,
                    sizeof(serverAddr));
    if(check!=0){

        perror("bind failed");
        exit(EXIT_FAILURE);

    }

    if (listen(serverSocket, 10) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
    }


    int error=5;
    map = loadMapFromFile("map.txt", &error);
    ///----------second map-------///
    // map = loadMapFromFile("pacman.txt",&error);
    if(error!=0){
        if(error==-2){
            printf("File not found\n");
            return 2;
        } else if(error==-3){
            printf("File Corrupted\n");
            return 3;
        } else if(error==-4)
        {
            printf("Allocation failed\n");
            return 4;
        }
    }

    setParametersOnMap('A', map);

    displayMap(map);

    int map_w = getMapWidth(map);
    int map_h = getMapHeight(map);
    displayGameInfo(map_h, map_w);

   while(beastInGame < 5){
       beasts[beastInGame].display='*';
       beasts[beastInGame].num = beastInGame + 1;
       beasts[beastInGame].on = 0;
       beasts[beastInGame].under = 'p';
        pthread_create(&beast[beastInGame], NULL, &handleBeastMovement, &beasts[beastInGame]);
        beastInGame++;
    }
   int start_game=0;

   while(1){
       addr_size = sizeof(serverStorage);
       if ((newSocket = accept(serverSocket, (struct sockaddr*)&serverStorage,
                                    (socklen_t*)&addr_size))
               < 0) {
               perror("accept");
               exit(EXIT_FAILURE);
       }

       if(stateOfTheGame == 0){
           if(CLI_CO==MAX_CONNECTION){
               char full[1024]="Server full";
               send(newSocket,(void*)full,sizeof(full),0);
               close(newSocket);
           }

           char access[1024] = "Connection established";
           send(newSocket,(void*)access, sizeof(access),0);


           if(start_game==0 && CLI_CO==0){
               sem_post(&sem);
               pthread_create(&counter, NULL, &gameTimerThread, NULL);
               pthread_create(&keyListenner, NULL, &handleKeyboardInput, NULL);
               start_game=1;
           }

           j =0;
           while(j<MAX_CONNECTION){
               if(players[j].is_active==0) {
                   //mvprintw(j+1,55,"player %d, active [%d]",j+1,players[j].is_active);
                   break;
               }

               j++;
           }


           refresh();
           players[j].socket_number = newSocket;
           initializePlayer(&players[j], j);
           int num = j;

           pthread_create(&playersThread[j], NULL, &playerConnectionThread, &num);
           CLI_CO++;
       }
       if(stateOfTheGame)break;

   }
   clear();
    endwin();
    printf("Game is finished--\n");
    int i = 0;
    while(i < beastInGame){
        pthread_join(beast[i],NULL);
        i++;
    }
    i=0;
    while (i<MAX_CONNECTION){
        if(players[i].is_active==1){
            pthread_join(playersThread[i], NULL);
        }
        i++;
    }


    pthread_join(counter,NULL);
    pthread_join(keyListenner, NULL);

    sem_destroy(&sem);
    sem_destroy(&sem_b);
    destroyCollisionStruct(&coins);
    deallocateMapMemory(map);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
int endGameServer(){
    int sock = 0, client_fd;
    struct sockaddr_in serv_addr;


    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
        <= 0) {
        printf(
                "\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client_fd
                 = connect(sock, (struct sockaddr*)&serv_addr,
                           sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return 1;
    }
    return 0;
}

void* handleKeyboardInput(void * arg){
    while(key!='q' && key!='Q'){
        key = (char) getchar();

        if(key=='c' || key == 't' || key == 'T' ){
            int i = 0;
            while(i<3){
                setParametersOnMap(key, map);
                i++;
            }
        }

        if (key=='b' || key =='B'){
            if(beastInGame < MAX_BEAST){
                beasts[beastInGame].display='*';
                beasts[beastInGame].num = beastInGame + 1;
                beasts[beastInGame].on = 0;
                beasts[beastInGame].under = 'p';
                pthread_create(&beast[beastInGame], NULL, &handleBeastMovement, &beasts[beastInGame]);
                beastInGame++;
            }
        }

        if(key=='q' || key=='Q'){
            stateOfTheGame=1;
            endGameServer();
        }


    }
    key='q';
    clear();
    endwin();
    return NULL;
}

void* gameTimerThread(void *arg){

    int h = getMapHeight(map);
    int w = getMapWidth(map);
    while(key!='q' && key!='Q'){
        //pthread_mutex_lock(&mutex);

        sem_wait(&sem);

        usleep(500000);

        RoundsCounter++;
        UpdateMap(&coins, map);
        displayGameInfo(h, w);
        displayMap(map);
        for(int i = 0;i<MAX_CONNECTION;i++){
            if(players[i].is_active==1){
                displayPlayerInfo(&players[i]);
            }
            else {
                displayDisconnectedPlayerGraphics(i);
            }
        }
        //pthread_mutex_unlock(&mutex);
        sem_post(&sem);
        //usleep(50000);
        struct timeval tp = { .tv_sec = 0, .tv_usec = 10000 };
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        switch( select(STDIN_FILENO + 1, &fds, NULL, NULL, &tp) ){
            case 1:
                if( FD_ISSET(STDIN_FILENO, &fds) ){}
                break;
            case 0:
                FD_ZERO(&fds);
                FD_SET(0, &fds);
                break;
        }
    }
    clear();
    endwin();
    return NULL;
}

void* playerConnectionThread(void *arg){


    int num = *(int *)arg;
    struct player_t  temp =players[num];
    //if(temp.UpdateMap=='1')sem_post(&sem);

    int check = (int)send(temp.socket_number,(void *)&temp,sizeof(temp),0);
    if(check<0){
        disconnectPlayer(num, 0);
        return NULL;
    }
    check = (int)recv(temp.socket_number,(void*)&temp, sizeof(temp),0);
    if(check<0){
        disconnectPlayer(num, 0);
        return NULL;
    }
    players[num].type =temp.type;
    setPositionOnMap(&players[num], map);

    while (1){

        if(key=='q' || key == 'Q') {
            break;
        }
        check = SendMapToPlayer(&players[num], map);
        if(check<0){
            break;
        }
        check = (int)send(players[num].socket_number,(void *)&players[num],sizeof(players[num]),0);
        if(check<0){
            break;
        }
        //pthread_mutex_lock(&mutex);
        sem_wait(&sem);

        char c;
        check = (int)recv(players[num].socket_number,(void*)&c,sizeof(c),0);
        if(check<0){
            break;
        }
        /*if (c=='q'){
            break;
        }*/


        handlePlayerMovement(&players[num], map, c);
        players[num].round_num = RoundsCounter;
        sem_post(&sem);
        //pthread_mutex_unlock(&mutex);
        //usleep(50000);
        struct timeval tp = { .tv_sec = 0, .tv_usec = 10000 };

        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        switch( select(STDIN_FILENO + 1, &fds, NULL, NULL, &tp) ){
            case 1:
                if( FD_ISSET(STDIN_FILENO, &fds) ){}
                break;
            case 0:
                FD_ZERO(&fds);
                FD_SET(0, &fds);
                break;
        }


    }

    disconnectPlayer(num, 0);
   // pthread_detach(pthread_self());
    return NULL;
}

void* handleBeastMovement(void *arg){
    sem_wait(&sem_b);
    struct beast_t p = *(struct beast_t*)arg;
    setPositionBeast(&p, map);
    sem_post(&sem_b);
    while(key!='q' && key!='Q'){

        char c= chasePlayerByBeast(&p, map);
        if(c!='a'  && c !='w' && c!='s'  && c !='d'){

            int j  = (rand() % ((4) - 1 + 1)) + 1;
            if(j==1)c='a';
            if(j==2)c='w';
            if(j==3)c='d';
            if(j==4)c='s';
        }
        //pthread_mutex_lock(&mutex);
        sem_wait(&sem);

        handleBeastMovements(&p, map, c);
        // pthread_mutex_unlock(&mutex);
        sem_post(&sem);
        //usleep(50000);
        struct timeval tp = { .tv_sec = 0, .tv_usec = 10000 };
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        switch( select(STDIN_FILENO + 1, &fds, NULL, NULL, &tp) ){
            case 1:
                if( FD_ISSET(STDIN_FILENO, &fds) ){}
                break;
            case 0:
                FD_ZERO(&fds);
                FD_SET(0, &fds);
                break;
        }
    }
    clear();
    endwin();
    return NULL;
}

void disconnectPlayer(int num, int post){

    players[num].is_active=0;
    players[num].display = 'p';
    *(*(map+ players[num].y)+ players[num].x)=32;
    CLI_CO--;
    shutdown(players[num].socket_number,SHUT_WR);
    close(players[num].socket_number);
    if(post==1){
        sem_post(&sem);
    }

}

void displayDisconnectedPlayerGraphics(int num){


    int max_height = getMapHeight(map);
    int dec = 23;
    if(players[num].num_player==1){

        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "-/-    ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-     ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-      ");
        refresh();
    }
    else if(players[num].num_player==2){
        dec = 31;
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "-/-    ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-     ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-      ");
        refresh();
    }
    else if(players[num].num_player==3){
        dec = 40;
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "-/-    ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-     ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-      ");
        refresh();
    }
    else if(players[num].num_player==4){
        dec = 48;
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "-/-    ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "-      ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-     ");
        refresh();
        mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "-      ");
        refresh();
    }

}

int initializePlayer(struct player_t*p, int num){


    p->coins=0;
    p->coins_b=0;
    p->death=0;
    p->round_num =0;
    p->num_player = num+1;
    p->is_active = 1;
    p->display = (char)(49+num);
    p->bush='p';
    p->count_bush=0;
    return 0;

}

int CollideBeastWithPlayer(struct beast_t *p, struct player_t *pl, char **mapT){
    if(p==NULL || pl==NULL )return -1;

    int x = p->x;
    int y = p->y;

    p->on = 1;
    p->under = 'D';
    if(pl->coins>0)pushInCollisionStructure(&coins, x, y, 'D', pl->coins);

    pl->coins=0;

    pl->y = pl->y_0;
    pl->x = pl->x_0;
    pl->death++;
    if(*(*(mapT+pl->y)+pl->x)=='c'){
        pl->coins+=1;
    }
    else  if(*(*(mapT+pl->y)+pl->x)=='t'){
        pl->coins+=10;
    }
    else  if(*(*(mapT+pl->y)+pl->x)=='T'){
        pl->coins+=50;

    }
    else  if(*(*(mapT+pl->y)+pl->x)=='D'){
        pop(&coins,pl->x_0,pl->y_0,pl);
    }
    *(*(mapT+pl->y_0)+pl->x_0)=pl->display;
    return 0;
}

int CollidePlayerWithPlayer(struct player_t *p1, struct player_t *p2, char **mapT){
    if(p1 ==NULL || p2 == NULL)return -1;

    p1->death++;
    p2->death++;

    int val = p1->coins + p2->coins;
    if(val>0){
        *(*(mapT+p1->y)+p1->x)='D';
        pushInCollisionStructure(&coins, p1->x, p1->y, 'D', val);
    }
    else{
        *(*(mapT+p1->y)+p1->x)=' ';
    }
    p1->coins = 0;
    p2->coins = 0;

    if(*(*(mapT+p1->y_0)+p1->x_0)=='c'){
        p1->coins+=1;
    }
    else  if(*(*(mapT+p1->y_0)+p1->x_0)=='t'){
        p1->coins+=10;
    }
    else  if(*(*(mapT+p1->y_0)+p1->x_0)=='T'){
        p1->coins+=50;
    }
    else  if(*(*(mapT+p1->y_0)+p1->x_0)=='D'){
        pop(&coins,p1->x_0,p1->y_0,p1);
    }

    /////////////////PLayer //////////////////////////////

    if(*(*(mapT+p2->y_0)+p2->x_0)=='c'){
        p2->coins+=1;
    }
    else  if(*(*(mapT+p2->y_0)+p2->x_0)=='t'){
        p2->coins+=10;
    }
    else  if(*(*(mapT+p2->y_0)+p2->x_0)=='T'){
        p2->coins+=50;
    }
    else  if(*(*(mapT+p2->y_0)+p2->x_0)=='D'){
        pop(&coins,p2->x_0,p2->y_0,p2);
    }
    p1->y = p1->y_0;
    p1->x = p1->x_0;

    p2->y = p2->y_0;
    p2->x = p2->x_0;
    return 0;
}

int CollidePlayerWithBeast(struct player_t *p, struct beast_t *bete, char **mapT){
    if(p==NULL || bete==NULL)return -1;
    p->death++;

    int val = p->coins;
    if(val>0){
        if(bete->on == 1){
            if(bete->under=='c')pushInCollisionStructure(&coins, p->x, p->y, 'D', val + 1);
            if(bete->under=='t')pushInCollisionStructure(&coins, p->x, p->y, 'D', val + 10);
            if(bete->under=='T')pushInCollisionStructure(&coins, p->x, p->y, 'D', val + 50);
            bete->under = 'D';
        }
        else{
            bete->under = 'D';
            bete->on = 1;
            pushInCollisionStructure(&coins, p->x, p->y, 'D', val);
        }

    }
    p->coins = 0;
    if(*(*(mapT+p->y_0)+p->x_0)=='c'){
        p->coins+=1;
    }
    else  if(*(*(mapT+p->y_0)+p->x_0)=='t'){
        p->coins+=10;
    }
    else  if(*(*(mapT+p->y_0)+p->x_0)=='T'){
        p->coins+=50;
    }
    else  if(*(*(mapT+p->y_0)+p->x_0)=='D'){
        pop(&coins,p->x_0,p->y_0,p);
    }


    p->y = p->y_0;
    p->x = p->x_0;
    return 0;

}

void handleBeastMovements(struct beast_t *p, char **mapT, char c){
    if(p==NULL || mapT == NULL){
        clear();
        printw("ERROR Player");
        refresh();
        return;
    }
    switch (c) {
        case 'a':
            if(*(*(mapT + p->y ) + p->x - 1 ) != '0' && *(*(mapT + p->y) + p->x - 1 ) != '\0'){
                if(p->on==1){
                    *(*(mapT+p->y)+p->x)=p->under;
                    p->under = 'p';
                    p->on = 0;
                }
                else *(*(mapT+p->y)+p->x)='.';
                p->x-=1;
                break;
            }
            else return;
        case 'd':
            if(*(*(mapT + p->y) + p->x + 1 ) != '0' && *(*(mapT + p->y ) + p->x + 1) != '\0'){
                if(p->on==1){
                    *(*(mapT+p->y)+p->x)=p->under;
                    p->under = 'p';
                    p->on = 0;
                }
                else *(*(mapT+p->y)+p->x)='.';
                p->x+=1;
                break;
            }
            else return;
        case 'w':
            if(*(*(mapT + p->y-1 ) + p->x ) != '0' && *(*(mapT + p->y-1) + p->x ) != '\0'){
                if(p->on==1){
                    *(*(mapT+p->y)+p->x)=p->under;
                    p->under = 'p';
                    p->on = 0;
                }
                else *(*(mapT+p->y)+p->x)='.';
                p->y-=1;
                break;
            }
            else return;
        case 's':
            if(*(*(mapT + p->y+1 ) + p->x ) != '0' && *(*(mapT + p->y+1) + p->x ) != '\0'){
                if(p->on==1){
                    *(*(mapT+p->y)+p->x)=p->under;
                    p->under = 'p';
                    p->on = 0;
                }
                else *(*(mapT+p->y)+p->x)='.';
                p->y+=1;
                break;
            }
            else return;
        default:
            return;


    }
    /////////////////////Best Collide with player//////////////////////////
    if( *(*(mapT+p->y)+p->x)>='1'  &&  *(*(mapT+p->y)+p->x)<='4'){
        int i =0;
        while(i<CLI_CO){
            if(*(*(mapT+p->y)+p->x)==players[i].display)break;
            i++;
        }
        CollideBeastWithPlayer(p, &players[i], mapT);

    }
    else if( *(*(mapT+p->y)+p->x) == 'c' ||  *(*(mapT+p->y)+p->x)=='T' ||  *(*(mapT+p->y)+p->x)=='t' ||  *(*(mapT+p->y)+p->x)=='#'  ||  *(*(mapT+p->y)+p->x)=='D' ||  *(*(mapT+p->y)+p->x)=='A'){
        p->on=1;
        p->under = *(*(mapT+p->y)+p->x);
    }

    *(*(mapT+p->y)+p->x)=p->display;
    UpdateMap(&coins, mapT);
}

void handlePlayerMovement(struct player_t *p , char ** mapT, char c){
    if(p==NULL || mapT == NULL){
        clear();
        printw("ERROR Player");
        refresh();
        return;
    }

    if(p->bush=='#' && p->count_bush==0){
        p->count_bush++;
    }
    else{


        switch (c) {
            case 'a':
                if(*(*(mapT + p->y ) + p->x - 1 ) != '0' && *(*(mapT + p->y) + p->x - 1 ) != '\0'){
                    *(*(mapT+p->y)+p->x)='.';
                    if(p->bush=='#'){
                        p->bush = 'p';
                        p->count_bush = 0;
                        *(*(mapT+p->y)+p->x)='#';
                    }
                    p->x-=1;
                    break;
                }
                else return;
            case 'd':
                if(*(*(mapT + p->y) + p->x + 1 ) != '0' && *(*(mapT + p->y ) + p->x + 1) != '\0'){
                    *(*(mapT+p->y)+p->x)='.';
                    if(p->bush=='#'){
                        p->bush = 'p';
                        p->count_bush = 0;
                        *(*(mapT+p->y)+p->x)='#';
                    }
                    p->x+=1;

                    break;
                }
                else return;
            case 'w':
                if(*(*(mapT + p->y-1 ) + p->x ) != '0' && *(*(mapT + p->y-1) + p->x ) != '\0'){
                    *(*(mapT+p->y)+p->x)='.';
                    if(p->bush=='#'){
                        p->bush = 'p';
                        p->count_bush = 0;
                        *(*(mapT+p->y)+p->x)='#';
                    }
                    p->y-=1;
                    break;
                }
                else return;
            case 's':
                if(*(*(mapT + p->y+1 ) + p->x ) != '0' && *(*(mapT + p->y+1) + p->x ) != '\0'){
                    *(*(mapT+p->y)+p->x)='.';
                    if(p->bush=='#'){
                        p->bush = 'p';
                        p->count_bush = 0;
                        *(*(mapT+p->y)+p->x)='#';
                    }
                    p->y+=1;
                    break;
                }
                else return;
            default:
                return;


        }

        if(*(*(mapT + p->y ) + p->x)=='*'){
            int i;
            for( i = 0; i < beastInGame; i++){
                if(p->x == beasts[i].x && p->y == beasts[i].y)break;
            }
            CollidePlayerWithBeast(p, &beasts[i], mapT);
            p->x=p->x_0;
            p->y=p->y_0;
        }
        if(*(*(mapT + p->y ) + p->x )=='c')p->coins++;
        if(*(*(mapT + p->y ) + p->x )=='t')p->coins+=10;
        if(*(*(mapT + p->y ) + p->x )=='T')p->coins+=50;
        if(*(*(mapT + p->y ) + p->x )=='A'){
            p->coins_b+=p->coins;
            p->coins=0;
        }
        if(*(*(mapT + p->y ) + p->x )=='#'){
            p->bush='#';
            p->count_bush=0;
            pushInCollisionStructure(&coins, p->x, p->y, '#', 0);
        }
        if(*(*(mapT + p->y ) + p->x )=='D'){
            pop(&coins,p->x,p->y,p);
        }

        if((*(*(mapT+p->y)+p->x)>='1'  &&  *(*(mapT+p->y)+p->x)<='4') && (*(*(mapT+p->y)+p->x)!=p->display)){
            int i =0;
            while(i<CLI_CO){
                if(*(*(mapT+p->y)+p->x)==players[i].display)break;
                i++;
            }
            CollidePlayerWithPlayer(p, &players[i], mapT);
        }
    }

    if(p->bush!='#')*(*(mapT+p->y)+p->x)=p->display;
    UpdateMap(&coins, mapT);
}

int setPositionBeast(struct beast_t*p, char**mapt){
    srand(time(NULL));
    if (p == NULL)return -1;
    int i, j;

    int map_w = getMapWidth(mapt) - 1;
    int map_h = getMapHeight(mapt) - 1;


    while (1) {
        i = (rand() % ((map_w) - 0 + 1)) + 0;

        j = (rand() % ((map_h-1) - 1 + 1)) + 0;
        char c = (char)(*(*(map+j)+i));
        if ( c == '.')break;

    }

    p->y=j;
    p->x=i;


    *(*(mapt + j) + i) = p->display;


    return 0;
}

char ** loadMapFromFile(const char *filename, int *error){

    if(filename==NULL){
        if(error!=NULL)
            *error =-2;
        return NULL;
    }

    char buf[1024];

    FILE *f = fopen(filename,"r");
    if(f==NULL)return NULL;

    int max_width=0, max_height=0,len,check;

    while(1){
        check =  fscanf(f,"%1023[^\n]\n",buf);
        if(check==EOF)break;
        else if(check!=1){
            fclose(f);
            if(error!=NULL)
                *error =-3;
            return NULL;
        }

        len = (int) strlen(buf);
        if(len>max_width){
            max_width=len;
        }
        max_height++;
    }
    max_height++;

    char ** map_t = calloc(max_height, sizeof(char *));
    if(map_t==NULL){
        fclose(f);
        if(error!=NULL)
            *error =-4;
        return NULL;
    }


    fseek(f,0,SEEK_SET);
    int row=0,col;
    while(1){
        check =  fscanf(f,"%1023[^\n]\n",buf);
        if(check==EOF)break;
        else if(check!=1){
            fclose(f);
            if(error!=NULL)
                *error =-3;
            return NULL;
        }

        len = (int)strlen(buf);
        *(map_t+row) = calloc(len+1, sizeof(char));
        if(*(map_t+row)==NULL){
            fclose(f);
            if(error!=NULL)
                *error =-4;
            return NULL;
        }

        col=0;
        while(col<len){
            if(*(buf+col) == '0'){
                *(*(map_t+row)+col)='0';
            } else if(*(buf+col)=='c'){
                *(*(map_t+row)+col)='c';
            }
            else if(*(buf+col)=='t'){
                *(*(map_t+row)+col)='t';
            }
            else if(*(buf+col)=='T'){
                *(*(map_t+row)+col)='T';
            }
            else if(*(buf+col)=='#'){
                *(*(map_t+row)+col)='#';
            }
            else{
                *(*(map_t+row)+col)='.';
            }
            col++;
        }
        *(*(map_t+row)+col)='\0';
        row+=1;

    }
    *(map_t+row)=NULL;
    fclose(f);
    if(error!=NULL)
        *error =0;
    return map_t;
}

void deallocateMapMemory(char **map_t){
    int h = getMapHeight(map_t);

    int i = 0;

    while(i<h){
        free(*(map_t + i));
        i++;
    }
    free(map_t);
    map_t=NULL;
}

void displayMap(char ** map_t){
    if(map_t!=NULL){
        init_pair(5,COLOR_WHITE,COLOR_CYAN);
        int i,j;
        for(i =0;*(map_t+i);i++){

            for(j = 0;*(*(map_t+i)+j);j++){
                if(*(*(map_t+i)+j)=='0')
                {
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "█");
                }

                else if(*(*(map_t+i)+j)=='.'){
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", 32);

                }
                else if(*(*(map_t+i)+j)=='c'){
                    attron(COLOR_PAIR(2));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "$");
                    attroff(COLOR_PAIR(2));
                }
                else if(*(*(map_t+i)+j)=='t'){
                    attron(COLOR_PAIR(2));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "t");
                    attroff(COLOR_PAIR(2));
                }
                else if(*(*(map_t+i)+j)=='T'){
                    attron(COLOR_PAIR(2));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "T");
                    attroff(COLOR_PAIR(2));
                }
                else if(*(*(map_t+i)+j)>=49 && *(*(map_t+i)+j)<=52){
                    attron(COLOR_PAIR(1));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(*(map_t + i) + j));
                    attroff(COLOR_PAIR(1));
                }
                else if(*(*(map_t+i)+j)=='*'){
                    attron(COLOR_PAIR(3));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(*(map_t + i) + j));
                    attroff(COLOR_PAIR(3));
                }
                else if(*(*(map_t+i)+j)=='A'){
                    attron(COLOR_PAIR(4));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(*(map_t + i) + j));
                    attroff(COLOR_PAIR(4));
                }
                else if(*(*(map_t+i)+j)=='D'){
                    attron(COLOR_PAIR(5));
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(*(map_t + i) + j));
                    attroff(COLOR_PAIR(5));
                }
                else{
                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(*(map_t + i) + j));
                }
                refresh();
            }
        }
    }
    else printw("ERROR map not loaded");
}

int getMapWidth(char **carte){
    if(carte==NULL)return -1;

    int width;
    for(int i = 0;*(carte+i);i++){
        for(width=0;*(*(carte+i)+width);width++){

        }
    }
    return width;
}

int getMapHeight(char ** carte){
    if(carte==NULL)return -1;

    int width;
    int height;
    for(height = 0;*(carte+height);height++){
        for(width=0;*(*(carte+height)+width);width++){

        }
    }
    return height;
}

void displayGameInfo(int max_height, int max_width){
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 2, DISTANCE_FROM_SCREEN_BORDER, "Server's PID : %d", PORT);
    refresh();

    findPositionOnMap('A', map, &campCoordinatesX, &campCoordinatesY);
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 3, DISTANCE_FROM_SCREEN_BORDER, "Campsite X/Y : %d/%d", campCoordinatesX, campCoordinatesY);
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 4, DISTANCE_FROM_SCREEN_BORDER, "Round number: 0");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 6, DISTANCE_FROM_SCREEN_BORDER, "Parameters : ");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER, "PID : ");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER, "Type : ");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER, "Curr X/Y : ");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER, "Deaths : ");
    refresh();

    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 11, DISTANCE_FROM_SCREEN_BORDER, "Coins : ");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3, "Carried : ");
    refresh();
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3, "Brought: ");
    refresh();


    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 10, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "Legend : ");
    refresh();
    attron(COLOR_PAIR(1));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 9, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%d", 1234);
    attroff(COLOR_PAIR(1));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 9, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "-players");
    refresh();

    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 8, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%s     - wall ", "█");
    refresh();
    attron(COLOR_PAIR(3));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 7, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%c", '*');
    attroff(COLOR_PAIR(3));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 7, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "-WIld best");
    refresh();
    attron(COLOR_PAIR(2));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 6, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%c", 'c');
    attroff(COLOR_PAIR(2));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 6, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "-Coins");
    refresh();
    attron(COLOR_PAIR(2));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 5, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%c", 't');
    attroff(COLOR_PAIR(2));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 5, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "- treasure (10 coins)");
    refresh();
    attron(COLOR_PAIR(2));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 4, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%c", 'T');
    attroff(COLOR_PAIR(2));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 4, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "-large treasure (50 coins)");
    refresh();
    attron(COLOR_PAIR(5));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 3, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%c", 'D');
    attroff(COLOR_PAIR(5));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 3, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "-Dropped treasure");
    refresh();
    attron(COLOR_PAIR(4));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 2, DISTANCE_FROM_SCREEN_BORDER + 3 + max_width, "%c", 'A');
    attroff(COLOR_PAIR(4));
    mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER - 2, DISTANCE_FROM_SCREEN_BORDER + 9 + max_width, "-Campsite");
    refresh();
}

int setPositionOnMap(struct player_t *p, char**mapt) {
    srand(time(NULL));
    if (p == NULL)return -1;
    int i, j;

    int map_w = getMapWidth(mapt) - 1;
    int map_h = getMapHeight(mapt) - 1;

    while (1) {

        i = (rand() % ((map_w) - 0 + 1)) + 0;

        j = (rand() % ((map_h) - 1 + 1)) + 0;
        char c = (char)(*(*(map+j)+i));
        if ( c == '.')break;

    }

    p->y=j;
    p->x=i;
    p->x_0=i;
    p->y_0=j;

    *(*(mapt + j) + i) = p->display;


    return 0;
}

int setParametersOnMap(char param, char **mapt){
    srand(time(NULL));
    if (map == NULL)return -1;
    int i, j;

    int map_w = getMapWidth(mapt) - 1;
    int map_h = getMapHeight(mapt) - 1;

    while (1) {
        i = (rand() % ((map_w) - 0 + 1)) + 0;
        j = (rand() % ((map_h) - 1 + 1)) + 0;
        char c = (char)(*(*(map+j)+i));
        if ( c == '.')break;

    }

    *(*(mapt + j) + i) =param;
    return 0;
}

int SendMapToPlayer(struct player_t *p, char **map_t){
    int i,j;

    int x = p->x;
    int y = p->y;

    int h = getMapHeight(map_t) - 1;
    int w = getMapWidth(map_t) - 1;
    int x_min= x - 2;
    int y_min= y - 2;

    int x_max = x + 2;
    int y_max = y+2;
    int a;

    if(x_min<0 && y_min>=0 &&y_max<h){
        a = -x_min;
        x_max+=a;
        x_min = 0;
    }
    else if(x_min<0 && y_min<0 && y_max<h && y_min<w){
        a = -x_min;
        x_max+=a;
        x_min = 0;

        a = -y_min;
        y_max+=a;
        y_min = 0;
    }
    else if(y_min<0 && x_min>=0 && x_max<w && y_max<h){
        a = -y_min;
        y_max+=a;
        y_min = 0;
    }
    else if(y_min<0 && x_max>=w && y_max<h && x_min>=0 ){
        a = -y_min;
        y_max+=a;
        y_min = 0;

        x_min -= x_max-w;
        x_max = w;
    }
    else if(x_max>=w && y_min>=0 && y_max<h && x_min>=0){
        x_min -= x_max-w;
        x_max = w;
    }
    else if(x_max>=w && y_max>=h && x_min>=0 && y_min>=0){
        x_min -= x_max-w;
        x_max = w;
        y_min -= y_max-h;
        y_max = h;
    }
    else if(y_max>=h && x_max<w && x_min>=0 && y_min>=0){
        y_min -= y_max-h;
        y_max = h;
    }
    else if(y_max>=h && x_min<0 && x_max>=0 &&y_min>=0){
        y_min -= y_max-h;
        y_max = h;

        a = -x_min;
        x_max+=a;
        x_min = 0;
    }




    int l=0;
    char copy [1024];
    for(i=y_min; i <= y_max; i++){

        for(j=x_min; j <= x_max; j++){
            copy[l] = *(*(map_t + i) + j);
            l++;
        }
    }
    int check = (int)send(p->socket_number,(void*)copy, sizeof(copy),0);
    if(check<0){

        return -1;
    }
    *(*(map + campCoordinatesY) + campCoordinatesX)='A';
    return 0;
}

void displayPlayerInfo(struct player_t *p){
    int max_height = getMapHeight(map);
    int dec = 23;
        if(p->num_player==1){
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 4, DISTANCE_FROM_SCREEN_BORDER, "Round number: %d", RoundsCounter);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 6, DISTANCE_FROM_SCREEN_BORDER + dec, "Player%d", p->num_player);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "%d", PORT);
            refresh();
            if(p->type == 'H')
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "HUMAN");
            else
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "BOT");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d/%02d", p->x, p->y);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d", p->death);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins_b);
            refresh();
        }
        else if(p->num_player==2){
            dec = 31;
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 4, DISTANCE_FROM_SCREEN_BORDER, "Round number: %d", RoundsCounter);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 6, DISTANCE_FROM_SCREEN_BORDER + dec, "Player%d", p->num_player);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "%d", PORT);
            refresh();
            if(p->type == 'H')
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "HUMAN");
            else
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "BOT");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d/%02d", p->x, p->y);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d", p->death);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins_b);
            refresh();
        }
        else if(p->num_player==3){
            dec = 40;
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 4, DISTANCE_FROM_SCREEN_BORDER, "Round number: %d", RoundsCounter);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 6, DISTANCE_FROM_SCREEN_BORDER + dec, "Player%d", p->num_player);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "%d", PORT);
            refresh();
            if(p->type == 'H')
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "HUMAN");
            else
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "BOT");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d/%02d", p->x, p->y);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d", p->death);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins_b);
            refresh();
        }
        else if(p->num_player==4){
            dec = 48;
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 4, DISTANCE_FROM_SCREEN_BORDER, "Round number: %d", RoundsCounter);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 6, DISTANCE_FROM_SCREEN_BORDER + dec, "Player%d", p->num_player);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + dec, "%d", PORT);
            refresh();
            if(p->type == 'H')
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "HUMAN");
            else
                mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + dec, "BOT");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d/%02d", p->x, p->y);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + dec, "%02d", p->death);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins);
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "      ");
            refresh();
            mvprintw(max_height + DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 3 + dec, "%02d", p->coins_b);
            refresh();
        }



}

int findPositionOnMap(char c, char ** map_t, int *x, int *y){
    if(map_t==NULL)return 1;

    for(int i = 0;*(map+i);i++){
        for(int j= 0;*(*(map+i)+j);j++){
            if(*(*(map_t+i)+j)==c){
                *x = j;
                *y = i;
                break;
            }
        }
    }
    return 0;
}

int createColision(struct death * t, int N){
    if(t == NULL || N < 1)return -1;
    t->tab = (struct money*)malloc(N* sizeof(struct money));
    if(t->tab==NULL)return -2;
    t->size = 0;
    t->capacity=N;
    return 0;
}

void destroyCollisionStruct(struct death * t){
    if(t!=NULL)free(t->tab);

}

int resizeCollisionStructure(struct death *t, int N){
    if(t == NULL || N < 1)return -1;

    struct money *new = realloc(t->tab,N*sizeof (struct money));
    if(new == NULL)return 2;
    t->tab = new;
    t->capacity = N;
    return 0;
}

int pushInCollisionStructure(struct death *t, int x, int y, char c, int value){

    if(t==NULL || x<0 || y<0 || value<1)return -1;
    for(int i = 0;i<t->size;i++){
        if ((t->tab + i)->x == x && (t->tab + i)->y == y){
            (t->tab+t->size)->value+=value;
            return 0;
        }
    }
    (t->tab+t->size)->x=x;
    (t->tab+t->size)->y=y;
    (t->tab+t->size)->c=c;

    if(c=='c')(t->tab+t->size)->value=1;
    else if(c=='t')(t->tab+t->size)->value=10;
    else if(c=='T')(t->tab+t->size)->value=50;
    else  {

        (t->tab+t->size)->value=value;
    }
   // mvprintw(4,57,"key : %c", (t->tab+t->size)->c);
    if(t->size+1 == t->capacity){
        resizeCollisionStructure(t, t->capacity * 2);
    }
    t->size++;
    return 0;
}

int pop(struct death *t,int x, int y, struct player_t *p) {
    if (t == NULL || x < 0 || y < 0)return -1;
    int i;
    for (i = 0; i < t->size; i++) {
        if ((t->tab + i)->x == x && (t->tab + i)->y == y)break;
    }
        int piece = (t->tab+i)->value;
        p->coins+= piece;
    for (; i < t->size; i++) {
        (t->tab + i)->x = (t->tab + i + 1)->x;
        (t->tab + i)->y = (t->tab + i + 1)->y;
        (t->tab + i)->c = (t->tab + i + 1)->c;
        (t->tab + i)->value = (t->tab + i + 1)->value;
    }
    t->size--;
  return piece;
}

void UpdateMap(struct death *t, char **map_t){
    if(t!=NULL || map_t!=NULL)
    {
        for(int i = 0;i<t->size;i++){
            int x = (t->tab+i)->x;
            int y = (t->tab+i)->y;
            if(*(*(map_t+y)+x)=='.'){
                    *(*(map_t+y)+x)=(t->tab+i)->c;
            }
            else if(*(*(map_t+y)+x)=='c' ){
                *(*(map_t+y)+x)='D';
                (t->tab+i)->value+=1;
            }
            else if(*(*(map_t+y)+x)=='t' ){
                *(*(map_t+y)+x)='D';
                (t->tab+i)->value+=10;
            }
            else if(*(*(map_t+y)+x)=='T' ){
                *(*(map_t+y)+x)='D';
                (t->tab+i)->value+=50;
            }
        }
    }

}

char chasePlayerByBeast(struct beast_t *bete, char **map_t){
    int i,j;

    int x = bete->x;
    int y = bete->y;

    int h = getMapHeight(map_t) - 1;
    int w = getMapWidth(map_t) - 1;
    int x_min= x - 2;
    int y_min= y - 2;

    int x_max = x + 2;
    int y_max = y+2;
    int a;

    if(x_min<0 && y_min>=0 &&y_max<h){
        a = -x_min;
        x_max+=a;
        x_min = 0;
    }
    else if(x_min<0 && y_min<0 && y_max<h && y_min<w){
        a = -x_min;
        x_max+=a;
        x_min = 0;

        a = -y_min;
        y_max+=a;
        y_min = 0;
    }
    else if(y_min<0 && x_min>=0 && x_max<w && y_max<h){
        a = -y_min;
        y_max+=a;
        y_min = 0;
    }
    else if(y_min<0 && x_max>=w && y_max<h && x_min>=0 ){
        a = -y_min;
        y_max+=a;
        y_min = 0;

        x_min -= x_max-w;
        x_max = w;
    }
    else if(x_max>=w && y_min>=0 && y_max<h && x_min>=0){
        x_min -= x_max-w;
        x_max = w;
    }
    else if(x_max>=w && y_max>=h && x_min>=0 && y_min>=0){
        x_min -= x_max-w;
        x_max = w;
        y_min -= y_max-h;
        y_max = h;
    }
    else if(y_max>=h && x_max<w && x_min>=0 && y_min>=0){
        y_min -= y_max-h;
        y_max = h;
    }
    else if(y_max>=h && x_min<0 && x_max>=0 &&y_min>=0){
        y_min -= y_max-h;
        y_max = h;

        a = -x_min;
        x_max+=a;
        x_min = 0;
    }


    int l=0;
    char copy [1024];
    for(i=y_min; i <= y_max; i++){

        for(j=x_min; j <= x_max; j++){
            copy[l] = *(*(map_t + i) + j);
            l++;
        }
    }

    int chase_mode=0,count = 0;
    for(i=y_min; i <= y_max; i++){

        for(j=x_min; j <= x_max; j++){
            if(*(*(map_t+i)+j)>=49 && *(*(map_t+i)+j)<=52){
                count++;
                chase_mode  = 1;
            }

        }
    }

    char direction='p';

    if(chase_mode== 1) {
        for(i=y_min; i <= y_max; i++){
            for(j=x_min; j <= x_max; j++){
                if(*(*(map_t+i)+j)>=49 && *(*(map_t+i)+j)<=52){
                    break;
                }

            }
        }
        char track[5][5];
        l = 0;
        for(i=0;i<5;i++){

            for(j=0;j<5;j++){
                if(*(copy+l)=='0') *(*(track+i)+j)='0';
                else  if(*(copy+l)=='*') *(*(track+i)+j)='*';
                else  if(*(copy+l)>='1' && *(copy+l)<='4') *(*(track+i)+j)='+';
                else  *(*(track+i)+j)='1';
                l++;
            }
        }

        for(i=0;i<5;i++){
            int flag = 0;
            for(j=0;j<5;j++){
                char t = *(*(track+i)+j);
                if(t=='+'){
                    *(*(track+i)+j) =49;
                    flag = 1;
                    break;
                }

            }
            if(flag == 1)break;
        }
        int pl_x = j;
        int pl_y = i;
        for(i=0;i<5;i++){
            int flag = 0;
            for(j=0;j<5;j++){
                char t = *(*(track+i)+j);
                if(t=='*'){
                    *(*(track+i)+j) =49;
                    flag = 1;
                    break;
                }

            }
            if(flag == 1)break;
        }
        int b_x = j;
        int b_y = i;
        float d = (float)((bete->x-j)*(bete->x-j)+(bete->y-i)*(bete->y-i));
        float dist = sqrtf(d);

        if(dist==1 || dist==2){
            if(dist == 1){
                if (b_x == pl_x && b_y < pl_y) {
                    direction = 's';
                } else if (b_x == pl_x && b_y > pl_y) {
                    direction = 'w';
                } else if (b_x < pl_x && b_y == pl_y) {
                    direction = 'd';
                } else if (b_x > pl_x && b_y == pl_y) {
                    direction = 'a';
                }
            }
            if(dist==2){
                if (b_x == pl_x && b_y < pl_y) {
                    if (*(*(track+ b_y + 1) + b_x) != '0')direction = 's';
                } else if (b_x == pl_x && b_y > pl_y) {
                    if (*(*(track+ b_y - 1) + b_x) != '0')direction = 'w';
                } else if (b_x < pl_x && b_y == pl_y) {
                    if (*(*(track+ b_y) + b_x + 1) != '0')direction = 'd';
                } else if (b_x > pl_x && b_y == pl_y) {
                    if (*(*(track+ b_y) + b_x - 1) != '0')direction = 'a';
                }
            }
        }else {
            direction = solveMaze(track, b_x, b_y, pl_x, pl_y);
        }


    }

    *(*(map + campCoordinatesY) + campCoordinatesX)='A';
    return direction;
}

char  findPath(char sol[T][T], int b_x, int b_y)
{
    for (int i = 0; i < T; i++) {
        for (int j = 0; j < T; j++)
           if(sol[i][j]==49){
               float d = (float)((b_x-j)*(b_x-j)+(b_y-i)*(b_y-i));
               float dist = sqrtf(d);
               if(dist==1.00){
                   if(b_x==j && b_y<i)return 's';
                   if(b_x==j && b_y>i)return 'w';
                   if(b_x>j && b_y==i)return 'a';
                   if(b_x>j && b_y==i)return 'd';
               }
           }
    }
    return 'p';
}

bool isSafe(char maze[T][T], int x, int y)
{
    // if (x, y outside maze) return false
    if (x >= 0 && x < T && y >= 0 && y < T && maze[x][y] =='1')
        return true;
    return false;
}

char solveMaze(char maze[T][T],int b_x, int b_y, int p_x, int p_y)
{
    char sol[T][T] = { { '0', '0', '0', '0','0'},
                       { '0', '0', '0', '0','0' },
                       { '0', '0', '0', '0','0' },
                       { '0', '0', '0', '0','0'},
                       { '0', '0', '0', '0','0'},
    };

    if (solveMazeUtil(maze, b_x, b_y, sol, p_y,p_x) == false) {
        return 'p';
    }
    char c = findPath(sol, b_x, b_y);

    return c;
}

bool solveMazeUtil(char maze[T][T], int x, int y, char sol[T][T], int dest_x, int dest_y)
{
    // if (x, y is goal) return true
    if (x == dest_x && y == dest_y && maze[x][y] == '1') {
        sol[x][y] = '1';
        return true;
    }
    // Check if maze[x][y] is valid
    if (isSafe(maze, x, y) == true) {
        // Check if the current block is already part of
        // solution path.
        if (sol[x][y] == '1')
            return false;
        // mark x, y as part of solution path
        sol[x][y] = '1';
        /* Move forward in x direction */
        if (solveMazeUtil(maze, x + 1, y, sol,dest_x,dest_y) == true)
            return true;
        // If moving in x direction doesn't give solution
        // then Move down in y direction
        if (solveMazeUtil(maze, x, y + 1, sol,dest_x,dest_y) == true)
            return true;
        // If none of the above movements work then
        // BACKTRACK: unmark x, y as part of solution path
        if (solveMazeUtil(maze, x -1, y, sol,dest_x,dest_y) == true)
            return true;
        // If moving in x direction doesn't give solution
        // then Move down in y direction
        if (solveMazeUtil(maze, x, y -1, sol,dest_x,dest_y) == true)
            return true;
        sol[x][y] = '0';
        return false;
    }
    return false;
}


