#pragma once
#include "server.h"
#define GOLD	1
#define BUFF	2

class Object;


class Item {
public:
	int type;
	int price;
	Item();
	~Item();
	virtual void active_buff() = 0;
};

class Buff : public Item {
public:
	int buf_type;
	void active_buff(Object*& obj) {
		switch (buf_type) {
		case 0: {
			auto cast_obj = dynamic_cast<User*>(obj);
			cast_obj->setAttack_damage(cast_obj->getAttack_damage() + 20) ;
			break;
		}
		case 1: {
			auto cast_obj = dynamic_cast<User*>(obj);
			cast_obj->setHp(100);
			break;
		}
		default:
			break;
		}
	}
};

class Store {
public:
	Store() {
	}
	~Store() {

	}
};
