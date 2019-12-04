#pragma once
#pragma comment(lib, "ws2_32")
#include "map.h"
#include "server.h"

#define OP_RECV  1
#define OP_SEND  2
#define OP_MOVE  3
#define OP_EVENT_PLAYER_MOVE 4
#define OP_EVENT_PLAYER_AUTO_RECOVERY	5
#define OP_EVENT_PLAYER_REZEN	6

#define USER 10
#define MON1 0
#define MON2 1
#define MON3 2
#define STORE_NPC	3

#define VIEW_RADIUS   5
#define AGGRO_RADIUS 3
#define ATTACK_RADIUS 1

#define BOARD_WIDTH   100
#define BOARD_HEIGHT  100
#define MONSTER_DURATION 1000

#define MAX_PACKET_SIZE 1024
#define SERVERPORT 9000
#define MAX_BUFF_SIZE    4000
#define MAX_STR_SIZE  60

#define MAX_USER   1000
#define USER_START 1000
#define MAX_NPC 1000

#define MON1_EXP 10
#define MON2_EXP 20
#define MON3_EXP 30

#define USER_ATTACK_DURATION 200
#define USER_MOVE_DURATION 1000
#define MON_ATTACK_DURATION 2000
#define MON_MOVE_DURATION 1000
#define AUTO_RECOVERY_DURATION 5000
#define USER_ATTACK_DAMAGE	50
#define MON_ZEN_DURATION 100


#define CS_MOVE		1
#define CS_ATTACK	2
#define CS_CHAT		3
#define CS_STORE	4

#define STORE_OPEN	1
#define STORE_ITEM1 2
#define STORE_ITEM2 3

#define UP		1
#define DOWN      2
#define LEFT      3
#define RIGHT      4

// Server -> Client
#define SC_PUT_PLAYER      1
#define SC_POS    2
#define SC_REMOVE_PLAYER 3
#define LOGIN            4
#define SC_MAP_INFO 5
#define SC_CHAT		6
#define SC_ATTACK	7
#define SC_STAT_CHANGE	8


#pragma pack(push, 1)
struct cs_packet_dir {
	BYTE size;
	BYTE type;
};
struct cs_packet_attack {
	BYTE size;
	BYTE type;
	int id;
};
struct sc_packet_attack {
	BYTE size;
	BYTE type;
	int id;
	int attack_type;
	int hp;
};
struct cs_packet_store {
	BYTE size;
	BYTE type;
	int id;
	int store_type;
};
struct sc_packet_put_player {
	BYTE size;
	BYTE type;
	int id;
	int obj_type;
	int x;
	int y;
	int hp;
	int level;
	int exp;
	int gold;
	int attack_damage;
};
struct sc_packet_stat_change {
	BYTE size;
	BYTE type;
	int id;
	int hp;
	int level;
	int exp;
	int gold;
	int attack_damage;
};
struct sc_packet_pos {
	BYTE size;
	BYTE type;
	int obj_type;
	int id;
	int x;
	int y;
};
struct sc_packet_remove_player {
	BYTE size;
	BYTE type;
	int id;
};
struct cs_packet_move_player {
	BYTE size;
	BYTE type;
	int id;
	int dir;
};
struct sc_packet_map_info {
	BYTE size;
	BYTE type;
	bool move;
	int x;
	int y;
};
struct sc_packet_chat {
	BYTE size;
	BYTE type;
	int id;
	WCHAR message[MAX_STR_SIZE];
};
#pragma pack(pop)