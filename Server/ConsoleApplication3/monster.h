#pragma once
#include "object.h"
#include "protocol.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h" 
#include "lualib.h" 
}

class Monster : public Object {
public:
	bool is_active;
	int aggro;
	
	int give_exp;
	int give_gold;
	Monster(int t_id) {
		//cout << "몬생성" << endl;
		L = luaL_newstate();
		id = t_id;
		is_connected = true;
		is_active = false;
		point.x = rand() % BOARD_WIDTH;
		point.y = rand() % BOARD_HEIGHT;
		attack_duration = std::chrono::system_clock::now();
		sector.x = -1;
		sector.y = -1;
		aggro = -1;
		/*
		L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "monster.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, id);
		lua_pcall(L, 1, 1, 0);

		for (int y = 0; y < BOARD_HEIGHT; ++y) {
		for (int x = 0; x < BOARD_WIDTH; ++x) {
		lua_getglobal(L, "set_map");
		lua_pushnumber(L, map[x][y].x);
		lua_pushnumber(L, map[x][y].y);
		lua_pushnumber(L, map[x][y].move);
		lua_pcall(L, 3, 1, 0);
		}
		}

		lua_pop(L, 1);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		*/
		// 첫 접속일때 섹터가 정해지지 않음
	}
	void temp_f() {}
	bool getIs_active() {
		return is_active;
	}
	void setIs_active(bool n) {
		is_active = n;
	}
	int getAggro() {
		return aggro;
	}
	void setAggro(int n) {
		aggro = n;
	}
	int getGive_exp() {
		return give_exp;
	}
	void setGive_exp(int n) {
		give_exp = n;
	}
	int getGive_gold() {
		return give_gold;
	}
	void setGive_gold(int n) {
		give_gold = n;
	}
};
