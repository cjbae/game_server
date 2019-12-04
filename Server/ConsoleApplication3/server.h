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
	int event_id; // 이벤트가 일어나는 id
	int caller_id; // npc를 부르는 유저의 id
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
	int new_id; // 서버가 유저가 접속할때 부여하는 아이디 점검전까진 무한히 늘어남
	bool isshutdown = false;
	vector <thread *> worker_threads;
	//thread* accept_thread;
	vector <thread *> accept_thread;
	vector <thread *> timer_thread;
	priority_queue<event_type, vector<event_type>, mycomp> timer_queue;
	mutex timer_lock;
	
	// 유저 검색을 효율적으로 하기 위해 서버 내부를 섹터로 관리한다
	// 섹터의 크기는 10x10이다 시야는 6x6 전체 맵이 100x100
	// 섹터는 10개
	// 전체 유저가 1000명이면 섹터가 없다면 천명을 다 검색해야되지만
	// 섹터를 이용하면 시야에 걸치는 맥시멈 4개까지만 검색하면 된다.
	//LF_Sector sector[11][11];
public:
	Server();
	~Server();

	// 소켓 함수 오류 출력
	void err_display(char *msg);
	void error_display(char *msg, int err_no);

	// 서버 초기화 및 세팅
	void Initialize();

	// 패킷 처리
	void ProcessPacket(Object*& user, unsigned char buf[]);
	void ProcessPacketMove(Object*& user, int dir);
	void ProcessPacketAttack(Object*& user);

	// 워커스레드
	void WorkerThreadStart();

	// 억셉트스레드
	void AcceptThreadStart();

	// 패킷 전송
	void SendPacket(Object*& obj, unsigned char *packet);
	void SendPacket(User*& obj, unsigned char *packet);
	void Server::SendPacket(unordered_set<Object*>::iterator& user, unsigned char *packet);
	// 오브젝트 추가
	void SendPutPlayerPacket(Object*& obj, Object*& put_obj);
	// 오브젝트 삭제
	void SendRemovePlayerPacket(Object*& obj, Object*& rem_obj);
	void SendRemovePlayerPacket(unordered_set<Object*>::iterator& obj, Object*& rem_obj);
	// 어택패킷 보내기
	void SendAttackPlayerPacket(Object*& obj, Object*& attack_obj);
	// 채팅패킷 보내기
	void send_chat_packet(Object*& to, Object*& teller_id, WCHAR *message);
	// 스텟변화패킷 보내기
	template<class T>
	void SendStatChangePacket(Object*& user, T*& change_obj);

	// 유저의 시야 검색
	bool Is_InRange(Object*& a, Object*& b);

	// npc의 어그로 시야 검색
	bool Npc_Aggro_InRange(Object*& a, Object*& b);

	// 유저의 공격 범위 함수
	bool UserAttackInRange(Object*&a, Object*& b);

	// 유저가 속해야 되는 섹터를 검색하는 함수
	void sector_decision(Object*& obj);
	
	// near리스트 생성 함수
	unordered_set<Object*> near_list_create(Object*& obj, unordered_set<Object*>& list);
	// 타이머스레드
	void TimerThreadStart();
	// 타이머스레드에 추가하는 함수
	void add_timer(int obj_id, int m_sec, int event_type, int caller_id);
	// 몬스터 이동 함수
	void Lua_Do_Move(int i, int key);
	// NPC 여부 확인 함수
	const bool Is_NPC(const int id);
	// 자동회복함수
	void Auto_Recovery(int key);
	// 리젠함수
	void ReZen(int key);

	// npc가 움직이는지 아닌지
	const bool Is_Active(Monster*& npc);

	void Logout(int user) {
	}
	
	// 루아 API 함수들
	static int API_SendMessage(lua_State *L);
	static int API_get_x(lua_State *L);
	static int API_get_y(lua_State *L);
	static int API_SendAttackMessage(lua_State *L);
};

