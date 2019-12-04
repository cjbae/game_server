#pragma once
#include "object.h"
#include "protocol.h"

class User : public Object {
	int level;
	int exp;
	int need_exp;
	int gold;
	int attack_damage;
	bool store_open;
	int	packet_size;
	int previous_size;
	unsigned char packet_buff[MAX_PACKET_SIZE];
public:
	
	unsigned char* getPacket_buff() {
		return packet_buff;
	}
	void setPacket_buff(int n) {
		//packet_buff = n;
	}
	User(int t_id, SOCKET& t_s) {
		id = t_id;
		hp = 100;
		attack_damage = USER_ATTACK_DAMAGE;
		level = 1;
		exp = 0;
		need_exp = 100;
		store_open = false;
		type = USER;
		gold = 0;
		s = new SOCKET;
		*s = t_s;
		point.x = 4;
		point.y = 4;
		attack_duration = std::chrono::system_clock::now();
		is_connected = false;
		recv_overlap.operation = OP_RECV;
		recv_overlap.wsabuf.buf = reinterpret_cast<CHAR *>(recv_overlap.iocp_buffer);
		recv_overlap.wsabuf.len = sizeof(recv_overlap.iocp_buffer);
		packet_size = 0;
		previous_size = 0;
		memset(&recv_overlap.original_overlap, 0, sizeof(recv_overlap.original_overlap));
		view_list = new unordered_set<Object*>;
		// 첫 접속일때 섹터가 정해지지 않음
		sector.x = -1;
		sector.y = -1;

		// 뷰리스트 초기화
		vl_lock.lock();
		view_list->clear();
		vl_lock.unlock();
	}
	~User() {
		delete s;
	}
	void temp_f() {}
	int getLevel() {
		return level;
	}
	void setLevel(int n) {
		level = n;
	}
	int getExp() {
		return exp;
	}
	void setExp(int n) {
		exp = n;
	}
	int getNeed_exp() {
		return need_exp;
	}
	void setNeed_exp(int n) {
		need_exp = n;
	}
	int getGold() {
		return gold;
	}
	void setGold(int n) {
		gold = n;
	}
	int getAttack_damage() {
		return attack_damage;
	}
	void setAttack_damage(int n) {
		attack_damage = n;
	}
	bool getStore_open() {
		return store_open;
	}
	void setStore_open(bool n) {
		store_open = n;
	}
	int getPacket_size() {
		return packet_size;
	}
	void setPacket_size(int n) {
		packet_size = n;
	}
	int getPrevious_size() {
		return previous_size;
	}
	void setPrevious_size(int n) {
		previous_size = n;
	}
};