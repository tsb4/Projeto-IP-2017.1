#include "../lib/server.h"
#include "common.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// TODO: implement time spent to beat game?

#define MAX_LOGIN_SIZE 13
#define MAX_GAME_CLIENTS 4

void readMapHitbox(char **map);
bool hitbox(char **map, int y, int x);
void *mobLogicThread(void *entity_data);

int main(){
	char client_names[MAX_GAME_CLIENTS][MAX_LOGIN_SIZE];

	Entity *entity_data = (Entity *) calloc(MAX_ENTITIES, sizeof(Entity));
	Entity *entityBuffer = (Entity *) malloc(sizeof(Entity));

	for (int i = 0; i < MAX_ENTITIES; i++)
		entity_data->isAlive = false;

	char **mapHitbox = (char **) malloc(MAP_Y * sizeof(char *));
	for (int i = 0; i < MAP_Y; i++)
		mapHitbox[i] = (char *) malloc(MAP_X * sizeof(char));

	readMapHitbox(mapHitbox);

	serverInit(MAX_GAME_CLIENTS);

	// start curses
	initscr();
	// disable line buffering
	cbreak();
	// don't echo keys
	noecho();
	// hide cursor
	curs_set(0);
	// enable special keys
	keypad(stdscr, true);
	// waits for a key for PACKET_WAIT milliseconds.
	// that means it sends a packet every PACKET_WAIT
	timeout(PACKET_WAIT);

	pthread_t mob_thread;
	pthread_create(&mob_thread, NULL, &mobLogicThread, (void *) entity_data);

	printw("Server Running!\n");

	//box(0, 0);

	while(true){
		int id = acceptConnection();

		if(id != NO_CONNECTION){
			recvMsgFromClient(client_names[id], id, WAIT_FOR_IT);
			
			Entity player = newPlayer(WARRIOR, id, 25, 40 + (id * 3));

			// entity_data[0] to entity_data[MAX_CLIENTS] are reserved for players
			entity_data[id] = player;

			// send player entity to client
			sendMsgToClient(&player, sizeof(Entity), id);

			printw("%s connected id = %d\n", client_names[id], id);
		}

		// receive a message from client
		struct msg_ret_t msg_ret = recvMsg(entityBuffer);
		if(msg_ret.status == MESSAGE_OK){
			// if player doesn't walk into a wall, update entity_data array with new
			// entityBuffer.
			// this part should be redone as to update each part of an entity separately
			if (!hitbox(mapHitbox, entityBuffer->pos[POS_Y], entityBuffer->pos[POS_X])) {
				entity_data[msg_ret.client_id] = *entityBuffer;

				// print message on server console
				mvprintw(entityBuffer->id + 5, 0,"%d pos: %.2d %.2d", entityBuffer->id, entityBuffer->pos[POS_Y], entityBuffer->pos[POS_X]);
			}
		}
		else if(msg_ret.status == DISCONNECT_MSG){
			printw("%s disconnected, id = %d is free\n", client_names[msg_ret.client_id], msg_ret.client_id);
		}


		// stop server on backspace
		if (getch() == KEY_BACKSPACE){
			printw("Server closed!");
			break;
		}
		
		refresh();

		// send entity data to clients
		broadcast(entity_data, MAX_ENTITIES * sizeof(Entity));
	}

	endwin();

	return 0;
}

/*
 * separate thread that calculates monster ai and spawning every second
 */
void *mobLogicThread(void *entityData) {
	// TODO: while program is running!
	// TODO: monsters should spawn in waves
	srand(time(NULL));

	Entity *entity_data = (Entity *) entityData;

	int loops = 0;
    while(true) {
        sleep(1);
		// how many entities have been spawned in this cycle
		bool entitiesSpawned = 0;
		// 0 to MAX_CLIENTS entity ids are reserved for players
        for (int i = MAX_CLIENTS; i < MAX_ENTITIES; i++) {
			if (entity_data[i].isAlive) {
				// update AI function based on type
			} else if ( !entity_data[i].isAlive && (rand() % 200) <= 3 && entitiesSpawned < 5) {
				entity_data[i] = newMonster(BERSERK, rand() % MAP_Y, rand() % MAP_X);
				entitiesSpawned++;
			}
		}
    }
    return 0;
}

bool hitbox(char **map, int y, int x) {
	return (map[y][x] - '0' == 1)
		|| (y > MAP_Y - 3)
		|| (y < 0)
		|| (x > MAP_X - 4)
		|| (x < 0);
}

/*
	Reads map hitbox from resource files and saves it on a bool array
	Only needs to be used once per initialization
	TODO: CONVERT MAP FROM CHAR TO BOOL ARRAY
*/
void readMapHitbox(char **map) {
	FILE *map_hitbox = fopen("res/map_hitbox.rtxt", "r");

	for (int i = 0; i < MAP_Y; i++) {
		fgets(map[i], MAP_X, map_hitbox);
		strtok(map[i], "\n");
	}

	fclose(map_hitbox);
}
