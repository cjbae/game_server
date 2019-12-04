#pragma once

// 오브젝트 클래스를 몬스터와 유저가 상속받음

extern "C" {
#include "lua.h"
#include "lauxlib.h" 
#include "lualib.h" 
}

struct Overlap_ex {
	WSAOVERLAPPED original_overlap;
	int operation;
	int caller_id;
	WSABUF wsabuf;
	unsigned char iocp_buffer[MAX_BUFF_SIZE];
};

struct Coordinate {
	int x;
	int y;
};

class Object {
protected:
	int id;
	int hp;
	int type;
	bool is_connected;
	std::chrono::system_clock::time_point attack_duration;
	Coordinate point; // x,y 좌표
	unordered_set<Object*>* view_list;
	lua_State* L;
	SOCKET* s;
public:
	mutex vl_lock;
	Coordinate sector;
	Overlap_ex recv_overlap;
	Object() {}
	virtual ~Object() {}
	virtual void temp_f() = 0;

	SOCKET getSocket() {
		return *s;
	}

	std::chrono::system_clock::time_point getAttack_duration() {
		return attack_duration;
	}
	void setAttack_duration() {
		attack_duration = std::chrono::system_clock::now();
	}
	int getPoint_x() {
		return point.x;
	}int getPoint_y() {
		return point.y;
	}
	void setPoint(int x, int y) {
		point.x = x;
		point.y = y;
	}
	int getId() {
		return id;
	}
	void setId(int n) {
		id = n;
	}
	int getHp() {
		return hp;
	}
	void setHp(int n) {
		hp = n;
	}
	int getType() {
		return type;
	}
	void setType(int n) {
		type = n;
	}
	bool getIs_connected() {
		return is_connected;
	}
	void setIs_connected(bool n) {
		is_connected = n;
	}

	// 유저 상속 함수
	virtual int getLevel() { return -1; };
	virtual void setLevel(int n) {};
	virtual int getExp() { return -1; };
	virtual void setExp(int n) {};
	virtual int getNeed_exp() { return -1; }
	virtual void setNeed_exp(int n) {};
	virtual int getGold() { return -1; }
	virtual void setGold(int n) {};
	virtual int getAttack_damage() { return -1; }
	virtual void setAttack_damage(int n) {};
	virtual bool getStore_open() { return -1; }
	virtual void setStore_open(bool n) {};
	virtual unsigned char* getPacket_buff() { 
		unsigned char* arr;
		return arr;
	}
	virtual void setPacket_buff(int n) {};
	virtual int getPacket_size() { return -1; }
	virtual void setPacket_size(int n) {};
	virtual int getPrevious_size() { return -1; }
	virtual void setPrevious_size(int n) {};
	// 뷰리스트 함수
	// 카운트, 인서트, 이레이즈, 비긴, 엔드 이터 구하기
	unordered_set<Object*>::iterator VLbeginiter() {
		return view_list->begin();
	}
	unordered_set<Object*>::iterator VLenditer() {
		return view_list->end();
	}
	int VLcount(Object*& obj) {
		return view_list->count(obj);
	}
	void VLinsert(Object*& obj) {
		view_list->insert(obj);
	}
	void VLerase(Object*& obj) {
		view_list->erase(obj);
	}
	void VLclear() {
		view_list->clear();
	}

	// 몬스터 상속 함수
	virtual bool getIs_active() { return -1; }
	virtual void setIs_active(bool n) {};
	virtual int getAggro() { return -1; }
	virtual void setAggro(int n) {};

	lua_State* getL() {
		return L;
	};

	void setL() { L = luaL_newstate(); };
	virtual int getGive_exp() { return -1; }
	virtual void setGive_exp(int n) {};
	virtual int getGive_gold() { return -1; }
	virtual void setGive_gold(int n) {};
};