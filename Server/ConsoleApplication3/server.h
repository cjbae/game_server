#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <set>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <queue>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <iterator>
#include "Sector.h"
#include "protocol.h"
#include "object.h"
#include "player.h"
#include "monster.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h" 
#include "lualib.h" 
}

using namespace std;
using namespace stdext;

struct event_type {
	int obj_id;
	unsigned int wakeup_time;
	int event_id; // �̺�Ʈ�� �Ͼ�� id
	int caller_id; // npc�� �θ��� ������ id
};

class mycomp {
public:
	bool operator() (const event_type lhs, const event_type rhs) const {
		return (lhs.wakeup_time > rhs.wakeup_time);
	}
};

extern unordered_map<int, Object*> object;
extern Map map[100][100];

class Server {
	HANDLE IOCP;
	int new_id; // ������ ������ �����Ҷ� �ο��ϴ� ���̵� ���������� ������ �þ
	bool isshutdown = false;
	vector <thread *> worker_threads;
	//thread* accept_thread;
	vector <thread *> accept_thread;
	vector <thread *> timer_thread;
	priority_queue<event_type, vector<event_type>, mycomp> timer_queue;
	mutex timer_lock;
	
	// ���� �˻��� ȿ�������� �ϱ� ���� ���� ���θ� ���ͷ� �����Ѵ�
	// ������ ũ��� 10x10�̴� �þߴ� 6x6 ��ü ���� 100x100
	// ���ʹ� 10��
	// ��ü ������ 1000���̸� ���Ͱ� ���ٸ� õ���� �� �˻��ؾߵ�����
	// ���͸� �̿��ϸ� �þ߿� ��ġ�� �ƽø� 4�������� �˻��ϸ� �ȴ�.
	//LF_Sector sector[11][11];
public:
	Server();
	~Server();

	// ���� �Լ� ���� ���
	void err_display(char *msg);
	void error_display(char *msg, int err_no);

	// ���� �ʱ�ȭ �� ����
	void Initialize();

	// ��Ŷ ó��
	void ProcessPacket(Object*& user, unsigned char buf[]);
	void ProcessPacketMove(Object*& user, int dir);
	void ProcessPacketAttack(Object*& user);

	// ��Ŀ������
	void WorkerThreadStart();

	// ���Ʈ������
	void AcceptThreadStart();

	// ��Ŷ ����
	void SendPacket(Object*& obj, unsigned char *packet);
	void SendPacket(User*& obj, unsigned char *packet);
	void Server::SendPacket(unordered_set<Object*>::iterator& user, unsigned char *packet);
	// ������Ʈ �߰�
	void SendPutPlayerPacket(Object*& obj, Object*& put_obj);
	// ������Ʈ ����
	void SendRemovePlayerPacket(Object*& obj, Object*& rem_obj);
	void SendRemovePlayerPacket(unordered_set<Object*>::iterator& obj, Object*& rem_obj);
	// ������Ŷ ������
	void SendAttackPlayerPacket(Object*& obj, Object*& attack_obj);
	// ä����Ŷ ������
	void send_chat_packet(Object*& to, Object*& teller_id, WCHAR *message);
	// ���ݺ�ȭ��Ŷ ������
	template<class T>
	void SendStatChangePacket(Object*& user, T*& change_obj);

	// ������ �þ� �˻�
	bool Is_InRange(Object*& a, Object*& b);

	// npc�� ��׷� �þ� �˻�
	bool Npc_Aggro_InRange(Object*& a, Object*& b);

	// ������ ���� ���� �Լ�
	bool UserAttackInRange(Object*&a, Object*& b);

	// ������ ���ؾ� �Ǵ� ���͸� �˻��ϴ� �Լ�
	void sector_decision(Object*& obj);
	
	// near����Ʈ ���� �Լ�
	unordered_set<Object*> near_list_create(Object*& obj, unordered_set<Object*>& list);
	// Ÿ�̸ӽ�����
	void TimerThreadStart();
	// Ÿ�̸ӽ����忡 �߰��ϴ� �Լ�
	void add_timer(int obj_id, int m_sec, int event_type, int caller_id);
	// ���� �̵� �Լ�
	void Lua_Do_Move(int i, int key);
	// NPC ���� Ȯ�� �Լ�
	const bool Is_NPC(const int id);
	// �ڵ�ȸ���Լ�
	void Auto_Recovery(int key);
	// �����Լ�
	void ReZen(int key);

	// npc�� �����̴��� �ƴ���
	const bool Is_Active(Monster*& npc);

	void Logout(int user) {
	}
	
	// ��� API �Լ���
	static int API_SendMessage(lua_State *L);
	static int API_get_x(lua_State *L);
	static int API_get_y(lua_State *L);
	static int API_SendAttackMessage(lua_State *L);
};

