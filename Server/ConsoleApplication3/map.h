#pragma once
#include <stdio.h>
#include <Windows.h>
#include <iostream>

#define MOVE_POSSIBLE true
#define MOVE_IMPOSSIBLE false
class Map
{
public:
	bool move;
	int x;
	int y;


	Map();
	Map(int&a, int&b) {
		x = a;
		y = b;
	}
	~Map();
};

class AstarMap : public Map {
public:
	// ��ã�� �ʿ� ����
	Map parents; // �θ� ����
	int f;
	int g;
	int h;
	int distance;

	AstarMap() {
		f = 0;
		g = 0;
		h = 0;
		distance = 0;
	}
	~AstarMap() {}
	AstarMap(Map& m) {
		x = m.x;
		y = m.y;
		move = m.move;
		f = 0;
		g = 0;
		h = 0;
		distance = 0;
	}
	AstarMap(Map& m, AstarMap& p, Map& target) {
		x = m.x;
		y = m.y;
		std::cout << "������" << x << ", " << y << std::endl;
		move = m.move;

		parents.x = p.x;
		parents.y = p.y;
		parents.move = p.move;

		h = abs(x - target.x) + abs(y - target.y);
		g = p.f + 10;
		f = h + g;
		std::cout << "���" << h << ", " << g <<  ", " << f << std::endl;
	}
};

