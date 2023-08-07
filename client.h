//
// Created by joki-004 on 10/25/22.
//

#ifndef JEUX_CLIENT_H
#define JEUX_CLIENT_H
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
void *handle_connection_cli(void *arg);
void *getKeys(void*arg);
void displayInfo(struct player_t *p);
void displayMapPlayer(const char * map_t, struct player_t *p);
void *gameClientCommunication(void * arg);
char get_key();
#endif //JEUX_CLIENT_H
