//
// Created by joki-004 on 10/25/22.
//

#ifndef JEUX_SERVER_H
#define JEUX_SERVER_H

#include <sys/socket.h>
#include <stdbool.h>
struct  beast_t{
    int x;
    int y;
    char display;
    int num;
    char under;
    int on;
};
struct player_t{
    int num_player;
    char type;
    int round_num;
    int x;
    int y;
    int x_0;
    int y_0;
    int coins;
    int coins_b;
    int death;
    int socket_number;
    char display;
    int is_active;
    char bush;
    int count_bush;

};
struct money{
    int x;
    int y;
    char c;
    int value;
};
struct death{
    int size;
    int capacity;
    struct money *tab;
};


void disconnectPlayer(int num, int post);
void displayDisconnectedPlayerGraphics(int num);

void deallocateMapMemory(char **map_t);
void* handleBeastMovement(void *arg);
void handleBeastMovements(struct beast_t *p, char **mapT, char c);
int setPositionBeast(struct beast_t*p, char**mapt);
void *gameTimerThread(void *arg);
int setParametersOnMap(char c, char **mapt);
void displayPlayerInfo(struct player_t *p);
void handlePlayerMovement(struct player_t *p , char ** mapT, char c);
void *playerConnectionThread(void *arg);
char ** loadMapFromFile(const char * filename, int *error);
void * handleKeyboardInput(void * arg);
int endGameServer();
//free map;
void displayMap(char ** map);
int getMapWidth(char ** map);
int getMapHeight(char ** map);
void displayGameInfo(int max_height, int max_width);
int initializePlayer(struct player_t*p, int num);
int setPositionOnMap(struct player_t *p, char**mapt);
int SendMapToPlayer(struct player_t *p, char **map_t);
int CollideBeastWithPlayer(struct beast_t *p, struct player_t *pl, char **mapT);
int CollidePlayerWithPlayer(struct player_t *p1, struct player_t *p2, char **mapT);
int CollidePlayerWithBeast(struct player_t *p, struct beast_t *bete, char **mapT);
int findPositionOnMap(char c, char ** map_t, int *x, int *y);
// free i_players;
int createColision(struct death * t, int N);
void destroyCollisionStruct(struct death * t);
int pushInCollisionStructure(struct death *t, int x, int y, char c, int value);
int resizeCollisionStructure(struct death *t, int N);
int pop(struct death *t,int x, int y, struct player_t *p);
void pop_smoke(struct death *t,int x, int y);
void UpdateMap(struct death *t, char **map_t);
char solveMaze(char maze[5][5],int b_x, int b_y, int p_x, int p_y);
char  findPath(char sol[5][5], int b_x, int b_y);
bool isSafe(char maze[5][5], int x, int y);
bool solveMazeUtil(char maze[5][5], int x, int y, char sol[5][5], int dest_x, int dest_y);
char chasePlayerByBeast(struct beast_t *bete, char **map_t);
#endif //JEUX_SERVER_H
