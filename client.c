// Client side C/C++ program to demonstrate Socket
// programming
#include "client.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>
#include <pthread.h>
#define PORT 8989
#define DISTANCE_FROM_SCREEN_BORDER 3
char key='p';


int main(int argc, char const* argv[])
{
    setlocale(LC_ALL,"");
    initscr();
    start_color();
    init_pair(1,6,6);
    init_pair(2,COLOR_WHITE,5);
    init_pair(3,COLOR_BLACK,COLOR_YELLOW);
    init_pair(4,COLOR_RED,COLOR_BLACK);
    init_pair(5,COLOR_WHITE,COLOR_GREEN);
    init_pair(6,COLOR_WHITE,COLOR_CYAN);
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
    char msg[1024];
    int check = (int)recv(sock,(void*)msg, sizeof(msg),0);
    if(check<0){
        return -2;
    }
    printf("%s\n",msg);
    if(strcmp("Server full",msg)==0){
        return -2;
    }
    pthread_t key_listenner,gracz;
    pthread_create(&key_listenner, NULL, &getKeys, NULL);
    pthread_create(&gracz, NULL, &gameClientCommunication, &sock);
    pthread_join(gracz,NULL);
    pthread_join(key_listenner,NULL);
    endwin();
    printf("Goodbye :)\n");
    close(client_fd);
    return 0;
}

void displayInfo(struct player_t *p){
    int w = 10;
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 2, DISTANCE_FROM_SCREEN_BORDER + w, "Server's PID : %d", PORT);

    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 3, DISTANCE_FROM_SCREEN_BORDER + w, "Campsite : Unknown");
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 4, DISTANCE_FROM_SCREEN_BORDER + w, "Round number: %d", p->round_num);
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 6, DISTANCE_FROM_SCREEN_BORDER + w, "Parameters : ");
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 7, DISTANCE_FROM_SCREEN_BORDER + w, "Number : %02d", p->num_player);
    refresh();
    if(p->type == 'H')
        mvprintw(DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + w, "Type : HUMAN");
    else
        mvprintw(DISTANCE_FROM_SCREEN_BORDER + 8, DISTANCE_FROM_SCREEN_BORDER + w, "Type : BOT");
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 9, DISTANCE_FROM_SCREEN_BORDER + w, "Curr X/Y : %02d/%02d", p->x, p->y);
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 10, DISTANCE_FROM_SCREEN_BORDER + w, "Deaths : %02d", p->death);
    refresh();

    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 11, DISTANCE_FROM_SCREEN_BORDER + w, "Coins found : %02d", p->coins);
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 12, DISTANCE_FROM_SCREEN_BORDER + w, "Coins brought %02d", p->coins_b);
    refresh();


    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 13, DISTANCE_FROM_SCREEN_BORDER + 12, "Legend : ");
    refresh();
    attron(COLOR_PAIR(2));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 14, DISTANCE_FROM_SCREEN_BORDER + 12, "%d", 1234);
    attroff(COLOR_PAIR(2));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 14, DISTANCE_FROM_SCREEN_BORDER + 18, "-players");
    refresh();
    attron(COLOR_PAIR(1));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 15, DISTANCE_FROM_SCREEN_BORDER + 12, "%s", "█");
    attroff(COLOR_PAIR(1));
    refresh();
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 15, DISTANCE_FROM_SCREEN_BORDER + 18, "- wall");

    attron(COLOR_PAIR(4));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 16, DISTANCE_FROM_SCREEN_BORDER + 12, "%c", '*');
    attroff(COLOR_PAIR(4));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 16, DISTANCE_FROM_SCREEN_BORDER + 18, "-WIld best");
    refresh();

    attron(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 17, DISTANCE_FROM_SCREEN_BORDER + 12, "%c", 'c');
    attroff(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 17, DISTANCE_FROM_SCREEN_BORDER + 18, "-Coins");
    refresh();

    attron(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 18, DISTANCE_FROM_SCREEN_BORDER + 12, "%c", 't');
    attroff(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 18, DISTANCE_FROM_SCREEN_BORDER + 18, "- treasure (10 coins)");
    refresh();

    attron(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 19, DISTANCE_FROM_SCREEN_BORDER + 12, "%c", 'T');
    attroff(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 19, DISTANCE_FROM_SCREEN_BORDER + 18, "-large treasure (50 coins)");
    refresh();

    attron(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 20, DISTANCE_FROM_SCREEN_BORDER + 12, "%c", 'D');
    attroff(COLOR_PAIR(3));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 20, DISTANCE_FROM_SCREEN_BORDER + 18, "-Dropped treasure");
    refresh();

    attron(COLOR_PAIR(5));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 21, DISTANCE_FROM_SCREEN_BORDER + 12, "%c", 'A');
    attroff(COLOR_PAIR(5));
    mvprintw(DISTANCE_FROM_SCREEN_BORDER + 21, DISTANCE_FROM_SCREEN_BORDER + 18, "-Campsite");
    refresh();
}

void *getKeys(void *arg){
    while(key!='q'){
        char c = (char)getchar();
        if(c=='\033'){
            getchar();
            c = (char)getchar();
            switch(c) { // the real value
                case 'A':
                    // code for arrow up
                    key = 'w';
                    break;
                case 'B':
                    // code for arrow down
                    key = 's';
                    break;
                case 'C':
                    // code for arrow right
                    key = 'd';
                    break;
                case 'D':
                    // code for arrow left
                    key = 'a';
                    break;
                default:
                    break;
            }
        }
        else if(c=='q'){
            key='q';
        }
        else
            key = c;
    }
    endwin();
    return NULL;

}

void displayMapPlayer(const char * map_t, struct player_t *p){
    if(map_t!=NULL){


        int i,j,l=0;
        for(i =0;i<7;i++){

            for(j = 0;j<7;j++){
                if(i==0 ||j==0 || i==6 || j==6){

                    mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "█");

                    refresh();
                }
                else{
                    if(*(map_t + l) == '0')
                    {
                        attron(COLOR_PAIR(1));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "█");
                        attroff(COLOR_PAIR(1));
                    }

                    else if(*(map_t + l) == '.'){
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", 32);
                        refresh();
                    } else if(*(map_t + l) == p->display){
                        attron(COLOR_PAIR(2));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", p->display);
                        attroff(COLOR_PAIR(2));
                    }
                    else if(*(map_t + l)=='c'){
                        attron(COLOR_PAIR(3));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "$");
                        attroff(COLOR_PAIR(3));
                    }
                    else if(*(map_t + l)=='t'){
                        attron(COLOR_PAIR(3));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "t");
                        attroff(COLOR_PAIR(3));
                    }
                    else if(*(map_t + l)=='T' || *(map_t + l)=='D'){
                        attron(COLOR_PAIR(3));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(map_t + l));
                        attroff(COLOR_PAIR(3));
                    }
                    else if(*(map_t + l)=='A'){
                        attron(COLOR_PAIR(5));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "A");
                        attroff(COLOR_PAIR(5));
                    }else if(*(map_t + l)=='*'){
                        attron(COLOR_PAIR(4));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "*");
                        attroff(COLOR_PAIR(4));
                    }
                    else if((*(map_t + l)>=49 && *(map_t + l)<=52) && *(map_t + l)!=p->display){
                        attron(COLOR_PAIR(2));
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(map_t + l));
                        attroff(COLOR_PAIR(2));
                    }
                    else{
                        mvprintw(DISTANCE_FROM_SCREEN_BORDER + i, DISTANCE_FROM_SCREEN_BORDER + j, "%c", *(map_t + l));
                    }
                    l++;
                    refresh();
                }

            }


        }
    }
    else printw("ERROR map not loaded");
}

void *gameClientCommunication(void * arg){

    char buf[1024] = {0 };
    int sock = *((int*)arg);
    char Type = 'H';
    struct player_t p ;
    int check = (int)recv(sock,( void*)&p,sizeof( p),0);
    if(check<0){
        pthread_exit(NULL);
    }

    p.type = Type;
    check = (int)send(sock,(void *)&p,sizeof(p),0);
    if(check<0){
        pthread_exit(NULL);
    }
    while(1){
        check = (int) recv(sock, (void *)buf, sizeof(buf), 0);
        if(check<0)break;
        displayMapPlayer((char *) buf, &p);
        check = (int) recv(sock,( void*)&p,sizeof( p),0);
        if(check<0)break;
        displayInfo(&p);
        check = (int) send(sock,(void*)&key,sizeof(key),0);
        if(check<0 || key=='q' || key=='Q')break;
        key='p';
    }
    endwin();
    pthread_exit(NULL);
    //return NULL;
}
