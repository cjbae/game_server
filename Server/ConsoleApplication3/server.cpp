#include "server.h"
using namespace std;
unordered_map<int, Object*> object;
Map map[100][100];

Server::Server() {
	Initialize();
	_wsetlocale(LC_ALL, L"korean");

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	cout << "서버시작" << endl;

	for (auto i = 0; i < 6; ++i)
		worker_threads.push_back(new thread([&]() { WorkerThreadStart(); }));
	accept_thread.push_back(new thread([&]() { AcceptThreadStart(); }));
	timer_thread.push_back(new thread([&]() { TimerThreadStart(); }));

	while (false == isshutdown) {
		Sleep(1000);
	}

	for (auto th : worker_threads) {
		th->join();
		delete th;
	}
	for (auto th : accept_thread) {
		th->join();
		delete th;
	}
	for (auto th : timer_thread) {
		th->join();
		delete th;
	}
	WSACleanup();
}
Server::~Server() {
}
void Server::err_display(char *msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
void Server::error_display(char *msg, int err_no) {
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("%s", msg);
	wprintf(L"에러%s\n", lpMsgBuf);
	LocalFree(lpMsgBuf);
}
void Server::Initialize() {
	cout << "맵, NPC 생성" << endl;
	new_id = USER_START - 1;
	// 맵정보 초기화
	for (int y = 0; y < BOARD_WIDTH; ++y) {
		for (int x = 0; x < BOARD_HEIGHT; ++x) {
			map[x][y].x = x;
			map[x][y].y = y;
			switch (rand() % 4) {
			case 0: map[x][y].move = MOVE_IMPOSSIBLE; break;
			case 1: map[x][y].move = MOVE_POSSIBLE; break;
			case 2: map[x][y].move = MOVE_POSSIBLE; break;
			case 3: map[x][y].move = MOVE_POSSIBLE; break;
			}
		}
	}

	object.reserve(MAX_NPC + MAX_USER);
	// npc 생성
	for (auto i = 0; i < MAX_NPC; ++i) {
		object.insert(pair<int, Object*>(i, new Monster(i)));
		sector_decision(object[i]);
		object[i]->setType(rand() % 3);
		int object_type = object[i]->getType();
		if (object_type == 0) {
			object[i]->setHp(100);
			object[i]->setGive_exp(MON1_EXP);
			object[i]->setGive_gold(100);
		}
		else if (object_type == 1) {
			object[i]->setHp(200);
			object[i]->setGive_exp(MON2_EXP);
			object[i]->setGive_gold(200);
		}
		else if (object_type == 2) {
			object[i]->setHp(300);
			object[i]->setGive_exp(MON3_EXP);
			object[i]->setGive_gold(300);
		}
		object[i]->setL();
		lua_State *L = object[i]->getL();
		luaL_openlibs(L);
		luaL_loadfile(L, "monster.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, object[i]->getId());
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
		lua_register(L, "API_SendAttackMessage", API_SendAttackMessage);
		lua_register(L, "API_SendMessage", API_SendMessage);
	}
	cout << "npc생성" << endl;
}

void Server::ProcessPacket(Object*& user, unsigned char buf[]) {
	switch (buf[1]) {
	case CS_MOVE: {
		cs_packet_move_player packet;
		memcpy(&packet, buf, buf[0]);
		ProcessPacketMove(user, packet.dir);
		break;
	}
	case CS_ATTACK: {
		cs_packet_attack packet;
		memcpy(&packet, buf, buf[0]);
		ProcessPacketAttack(user);
		break;
	}
	case CS_CHAT: {
		sc_packet_chat packet;
		memcpy(&packet, buf, buf[0]);
		wcscpy(packet.message, packet.message);
		//printf("send_chat_packet : %s\n", packet.message);

		//strcpy(chat_buf, (char*)chat_packet->message);
		for (int i = USER_START; i < new_id + 1; ++i) {
			send_chat_packet(object[i], object[packet.id], packet.message);
		}
		//printf("send_chat_packet :");
		//wcout << packet->message << endl;
		break;
	}
	case CS_STORE: {
		cs_packet_store packet;
		memcpy(&packet, buf, buf[0]);
		switch (packet.store_type) {
		case STORE_OPEN:
			if (user->getStore_open() == false) {
				cout << "상점오픈" << endl;
				user->setStore_open(true);
			} 
			else {
				cout << "상점클로즈" << endl;
				user->setStore_open(false);
			}
			break;
		case STORE_ITEM1: {
			int user_gold = user->getGold();
			if (user_gold >= 500) {
				cout << "hp up" << endl;
				user->setHp(user->getHp() + 100);
				user->setGold(user_gold - 500);
				SendStatChangePacket(user, user);
			}
		}
			break;
		case STORE_ITEM2: {
			int user_gold = user->getGold();
			if (user_gold >= 1000) {
				cout << "attack up" << endl;
				user->setAttack_damage(user->getAttack_damage() + 10);
				user->setGold(user_gold - 1000);
				SendStatChangePacket(user, user);
			}
		}
			break;
		default:
			break;
		}
		break;
	}
	default:
		printf("Invalid Packet Received from Client ID:%d\n", user->getId());
		//exit(-1);
	}
}

void Server::ProcessPacketMove(Object*& user, int dir) {
	User* cast_user = dynamic_cast<User*>(user);
	int x = user->getPoint_x();
	int y = user->getPoint_y();
	switch (dir) {
	case UP: if (y > 0) y -= 1; break;
	case DOWN: if (y < BOARD_HEIGHT - 1) y += 1; break;
	case LEFT: if (x > 0) x -= 1; break;
	case RIGHT: if (x < BOARD_WIDTH - 1) x += 1; break;
	default:
		break;
	}
	user->setPoint(x, y);

	// 움직인 유저 정보 갱신
	sc_packet_pos mov_packet;
	mov_packet.id = user->getId();
	mov_packet.obj_type = USER;
	mov_packet.size = sizeof(mov_packet);
	mov_packet.type = SC_POS;
	mov_packet.x = x;
	mov_packet.y = y;
	SendPacket(user, reinterpret_cast<unsigned char *>(&mov_packet));

	sector_decision(user);

	unordered_set<Object*> near_list;
	near_list_create(user, near_list);

	for (auto i : near_list) {
		user->vl_lock.lock();
		int i_id = i->getId();
		if (0 == user->VLcount(i)) { // 새로 뷰리스트에 들어오는 객체 처리
			user->VLinsert(i);
			user->vl_lock.unlock();
			SendPutPlayerPacket(user, i);
			if (Is_NPC(i_id)) continue;
			i->vl_lock.lock();
			if (0 == i->VLcount(user)) {
				i->VLinsert(user);
				i->vl_lock.unlock();
				SendPutPlayerPacket(i, user);
			}
			else {
				i->vl_lock.unlock();
				SendPacket(i, reinterpret_cast<unsigned char *>(&mov_packet));
			}
		}
		else { // 뷰리스트에 계속 유지되어 있는 객체 처리
			user->vl_lock.unlock();
			if (Is_NPC(i_id)) continue;
			i->vl_lock.lock();
			if (1 == i->VLcount(user)) {
				i->vl_lock.unlock();
				SendPacket(i, reinterpret_cast<unsigned char *>(&mov_packet));
			}
			else {
				i->VLinsert(user);
				i->vl_lock.unlock();
				SendPutPlayerPacket(i, user);
			}
		}
	}

	// 뷰리스트에서 나가는 객체 처리
	vector<Object*> remove_list;
	user->vl_lock.lock();
	//for (auto i : user->view_list) {
	//	if (0 != near_list.count(i)) continue;
	//	remove_list.push_back(i);
	//}
	auto i = user->VLbeginiter();
	while (i != user->VLenditer()) {
		if (0 != near_list.count(*i)) {
			++i;
			continue;
		}
		remove_list.push_back(*i);
		++i;
	}
	for (auto i : remove_list) user->VLerase(i);
	user->vl_lock.unlock();

	for (auto i : remove_list) {
		SendRemovePlayerPacket(user, i);
	}

	for (auto i : remove_list) {
		if (Is_NPC(i->getId())) continue;
		i->vl_lock.lock();
		if (0 != i->VLcount(user)) {
			i->VLerase(user);
			i->vl_lock.unlock();
			SendRemovePlayerPacket(i, user);
		}
		else
			i->vl_lock.unlock();
	}
	// 플레이어가 움직이면 주변 엔피씨에게 플레이어가 움직였다고 알려준다.
	for (auto i : near_list) {
		int i_id = i->getId();
		if (false == Is_NPC(i_id)) continue;
		if (i->getIs_active()) continue;
		i->setIs_active(true);
		Overlap_ex *over = new Overlap_ex;
		over->operation = OP_EVENT_PLAYER_MOVE;
		over->caller_id = user->getId();
		PostQueuedCompletionStatus(IOCP, 1, i_id, &over->original_overlap);
	}
}

void Server::ProcessPacketAttack(Object*& user) {
	chrono::system_clock::time_point curr = chrono::system_clock::now();
	std::chrono::milliseconds mill = std::chrono::duration_cast<std::chrono::milliseconds>(curr - user->getAttack_duration());
	if (mill.count() < USER_ATTACK_DURATION) return;
	unordered_set<Object*> user_near_list;
	near_list_create(user, user_near_list);
	for (auto i : user_near_list) {
		if (Is_NPC(i->getId()) == false) continue;
		if (UserAttackInRange(user, i) == false) continue;
		i->setHp(i->getHp() - user->getAttack_damage());
		int user_id = user->getId();
		int i_type = i->getType();
		int i_aggro = i->getAggro();
		if (i_type == 1 || i_type == 2) {
			if (object[i_aggro] == NULL) i->setAggro(user_id);
			if (i_aggro == -1) i->setAggro(user_id);
		}

		if (i->getHp() > 0) {
			// 공격자에게 피격된 몬스터 바뀐정보 보내기
			SendStatChangePacket(user, i);
			// 피격된 몬스터의 바뀐정보를 몬스터가 보이는 유저에게 보내주기
			unordered_set<Object*> near_list;
			near_list_create(i, near_list);
			for (auto t : near_list) {
				if (Is_NPC(t->getId())) continue;
				SendStatChangePacket(t, i);
			}
		}
		else {
			// 피가 0이되서 죽으면 리무브하고 젠등록시키기, 이몬스터가 보이는 유저의 뷰리스트에서 삭제하기, 경험치부여하기
			user->setExp(user->getExp() + i->getGive_exp());
			user->setGold(user->getGold() + i->getGive_gold());
			if (user->getExp() > user->getNeed_exp()) {
				user->setLevel(user->getLevel() + 1);
				user->setExp(0);
				user->setNeed_exp(user->getNeed_exp() * 2);
				//cout << "유저id:" << user->getId() << "가 레벨업:" << user->getLevel() << "필경:" << user->getNeed_exp() << endl;
			}
			// 죽인 유저의 레벨과 경험치 갱신해서 보내주기
			SendStatChangePacket(user, user);
			for (int j = USER_START; j < new_id + 1; ++j) {
				if (object[j] == nullptr) continue;
				object[j]->vl_lock.lock();
				if (object[j]->VLcount(i) == 0) {
					object[j]->vl_lock.unlock();
				}
				else {
					object[j]->VLerase(i);
					object[j]->vl_lock.unlock();
					SendRemovePlayerPacket(object[j], i);
				}
			}
			add_timer(i->getId(), MON_ZEN_DURATION, OP_EVENT_PLAYER_REZEN, user_id);
		}
	}
	user->setAttack_duration();
}

void Server::WorkerThreadStart() {
	while (false == isshutdown) {
		DWORD iosize;
		DWORD key;
		Overlap_ex *my_overlap;
		PULONG_PTR a;
		BOOL result = GetQueuedCompletionStatus(IOCP, &iosize, reinterpret_cast<PULONG_PTR>(&key), reinterpret_cast<LPOVERLAPPED *>(&my_overlap), INFINITE);

		if (FALSE == result) {
			// 에러 처리
		}
		if (0 == iosize) {
			closesocket(object[key]->getSocket());
			object[key]->setIs_connected(false);
			object[key]->vl_lock.lock();
			unordered_set<Object*>::iterator i = object[key]->VLbeginiter();
			while (i != object[key]->VLenditer()) {
				if (Is_NPC((*i)->getId())) {
					++i;
					continue;
				}
				if ((*i)->VLcount(object[key]) == 1) {
					(*i)->vl_lock.lock();
					(*i)->VLerase(object[key]);
					(*i)->vl_lock.unlock();
					SendRemovePlayerPacket(i, object[key]);
				}
				++i;
			}
			/*for (auto i : object[key]->view_list) {
				if (Is_NPC(i->getId())) continue;
				if (i->VLcount(object[key]) == 1) {
					i->vl_lock.lock();
					i->view_list.erase(object[key]);
					i->vl_lock.unlock();
					SendRemovePlayerPacket(i, object[key]);
				}
			}*/
			object[key]->vl_lock.unlock();
			object[key]->setIs_connected(false);
			delete object[key];
			object.erase(key);
			continue;
		}
		if (OP_RECV == my_overlap->operation) {
			unsigned char *buf_ptr = object[key]->recv_overlap.iocp_buffer;
			int remained = iosize;
			int packet_size = object[key]->getPacket_size();
			int previous_size = object[key]->getPrevious_size();
			unsigned char* packet_buff = object[key]->getPacket_buff();
			while (0 < remained) {
				if (0 == packet_size) {
					//[key]->setPacket_size(buf_ptr[0]);
					packet_size = buf_ptr[0];
				}
				int required = packet_size - previous_size;
				if (remained >= required) {
					memcpy(
						packet_buff + previous_size, buf_ptr, required);
					ProcessPacket(object[key], packet_buff);
					buf_ptr += required;
					remained -= required;
					//object[key]->setPacket_size(0);
					packet_size = 0;
					//object[key]->setPrevious_size(0);
					previous_size = 0;
				}
				else {
					memcpy(packet_buff
						+ previous_size,
						buf_ptr, remained);
					buf_ptr += remained;
					//object[key]->setPrevious_size(object[key]->getPrevious_size() + remained);
					previous_size = previous_size += remained;
					remained = 0;
				}
			}
			object[key]->setPacket_size(packet_size);
			object[key]->setPrevious_size(previous_size);
			DWORD flags = 0;
			WSARecv(object[key]->getSocket(),
				&object[key]->recv_overlap.wsabuf, 1, NULL, &flags,
				&object[key]->recv_overlap.original_overlap, NULL);
		}
		else if (OP_SEND == my_overlap->operation) {
			delete my_overlap;
		}
		else if (OP_EVENT_PLAYER_MOVE == my_overlap->operation) {
			// key는 npc
			Lua_Do_Move(my_overlap->caller_id, key);
			delete my_overlap;
		}
		else if (OP_EVENT_PLAYER_AUTO_RECOVERY == my_overlap->operation) {
			Auto_Recovery(key);
			delete my_overlap;
		}
		else if (OP_EVENT_PLAYER_REZEN == my_overlap->operation) {
			ReZen(key);
			delete my_overlap;
		}
		else if (OP_MOVE == my_overlap->operation) {
			Lua_Do_Move(my_overlap->caller_id, key);
			delete my_overlap;
		}
		else {
			cout << "Unknown IOCP event!\n";
			exit(-1);
		}
	}
}

void Server::AcceptThreadStart() {
	struct sockaddr_in listen_addr;

	SOCKET accept_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(ADDR_ANY);
	listen_addr.sin_port = htons(SERVERPORT);
	// listen_addr.sin_zero = 0;
	::bind(accept_socket, reinterpret_cast<sockaddr *>(&listen_addr), sizeof(listen_addr));
	listen(accept_socket, 10);
	// listen
	cout << "AcceptThread complete" << endl;
	while (false == isshutdown) {
		struct sockaddr_in client_addr;
		int addr_size = sizeof(client_addr);
		SOCKET new_client = WSAAccept(accept_socket, reinterpret_cast<sockaddr *>(&client_addr), &addr_size, NULL, NULL);

		if (INVALID_SOCKET == new_client) {
			int error_no = WSAGetLastError();
			error_display("Accept::WSAAccept", error_no);
			while (true);
		}

		if (object.size() == MAX_USER + MAX_NPC) {
			cout << "Max Concurrent User excceded!\n";
			closesocket(new_client);
			continue;
		}

		// 접속한 유저 정보 초기화
		++new_id;
		object.insert(pair<int, Object*>(new_id, new User(new_id, new_client)));
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), IOCP, new_id, 0);

		// 접속한 유저 섹터 초기화
		sector_decision(object[new_id]);

		// 접속한 유저에게 맵 정보 보내주기
		sc_packet_map_info map_packet;
		map_packet.size = sizeof(map_packet);
		map_packet.type = SC_MAP_INFO;
		for (int y = 0; y < BOARD_HEIGHT; ++y) {
			for (int x = 0; x < BOARD_WIDTH; ++x) {
				map_packet.move = map[x][y].move;
				map_packet.x = x;
				map_packet.y = y;
				SendPacket(object[new_id], reinterpret_cast<unsigned char *>(&map_packet));
			}
		}

		// 접속한 유저에게 자신의 정보 전송
		sc_packet_put_player enter_packet;
		enter_packet.id = new_id;
		enter_packet.obj_type = USER;
		enter_packet.size = sizeof(enter_packet);
		enter_packet.type = SC_PUT_PLAYER;
		enter_packet.x = object[new_id]->getPoint_x();
		enter_packet.y = object[new_id]->getPoint_y();
		enter_packet.hp = object[new_id]->getHp();
		enter_packet.level = object[new_id]->getLevel();
		enter_packet.exp = object[new_id]->getExp();
		enter_packet.gold = object[new_id]->getGold();
		enter_packet.attack_damage = object[new_id]->getAttack_damage();
		SendPacket(object[new_id], reinterpret_cast<unsigned char *>(&enter_packet));

		// 기존 유저에게 근처에 있는 새로 접속한 유저 정보 전송
		unordered_set<Object*> near_list;
		near_list_create(object[new_id], near_list);
		for (auto i : near_list) {
			if (false == i->getIs_connected()) continue;
			if (i == object[new_id]) continue;
			// 기존 유저의 시야에 보일때만 전송 + ViewList추가
			//if (false == Is_InRange(i, new_id)) continue;
			if (Is_NPC(i->getId())) continue;
			i->vl_lock.lock();
			i->VLinsert(object[new_id]);
			i->vl_lock.unlock();
			SendPacket(i, reinterpret_cast<unsigned char *>(&enter_packet));
		}

		// 접속한 유저에게 기존의 유저 + npc정보 전송
		object[new_id]->vl_lock.lock();
		for (auto i : near_list) {
			if (false == i->getIs_connected()) continue;
			//if (i == object[new_id]) continue;
			// 접속한 유저의 시야에 기존의 유저가 있을때 전송
			// ViewList에도 추가
			//if (false == Is_InRange(i, new_id)) continue;

			// 시야에 npc가 있으면 npc활동 시키기
			int i_id = i->getId();
			if (Is_NPC(i_id)) {
				if (i->getIs_active() == false) {
					i->setIs_active(true);
					Overlap_ex *over = new Overlap_ex;
					over->operation = OP_EVENT_PLAYER_MOVE;
					over->caller_id = object[new_id]->getId();
					PostQueuedCompletionStatus(IOCP, 1, i_id, &over->original_overlap);
				}
			}
			object[new_id]->VLinsert(i);
			enter_packet.id = i_id;
			enter_packet.obj_type = i->getType();
			enter_packet.x = i->getPoint_x();
			enter_packet.y = i->getPoint_y();
			enter_packet.hp = i->getHp();
			SendPacket(object[new_id], reinterpret_cast<unsigned char *>(&enter_packet));
		}
		object[new_id]->vl_lock.unlock();

		// 접속 유저의 오토리커버리 실행
		Overlap_ex *over = new Overlap_ex;
		over->operation = OP_EVENT_PLAYER_AUTO_RECOVERY;
		over->caller_id = object[new_id]->getId();
		PostQueuedCompletionStatus(IOCP, 1, object[new_id]->getId(), &over->original_overlap);

		object[new_id]->setIs_connected(true);
		DWORD flags = 0;
		int ret = WSARecv(new_client, &object[new_id]->recv_overlap.wsabuf, 1, NULL,
			&flags, &object[new_id]->recv_overlap.original_overlap, NULL);
		if (0 != ret){
			int error_no = WSAGetLastError();
			if (WSA_IO_PENDING != error_no)
				error_display("Accept:WSARecv", error_no);
		}
	}
}

void Server::SendPacket(Object*& user, unsigned char *packet) {
	User* temp = dynamic_cast<User*>(user);
	Overlap_ex *over = new Overlap_ex;
	memset(over, 0, sizeof(Overlap_ex));
	over->operation = OP_SEND;
	over->wsabuf.buf = reinterpret_cast<CHAR *>(over->iocp_buffer);
	over->wsabuf.len = packet[0];
	memcpy(over->iocp_buffer, packet, packet[0]);
	if (temp != nullptr) {
		int ret = WSASend(temp->getSocket(), &over->wsabuf, 1, NULL, 0,
			&over->original_overlap, NULL);
		if (0 != ret) {
			int error_no = WSAGetLastError();
			error_display("SendPacket:WSASend", error_no);
			while (true);
		}
	}
	else
		cout << "센드에러:유저가아님" << endl;
}
void Server::SendPacket(unordered_set<Object*>::iterator& user, unsigned char *packet) {
	Overlap_ex *over = new Overlap_ex;
	memset(over, 0, sizeof(Overlap_ex));
	over->operation = OP_SEND;
	over->wsabuf.buf = reinterpret_cast<CHAR *>(over->iocp_buffer);
	over->wsabuf.len = packet[0];
	memcpy(over->iocp_buffer, packet, packet[0]);
	int ret = WSASend((*user)->getSocket(), &over->wsabuf, 1, NULL, 0,
		&over->original_overlap, NULL);
	if (0 != ret) {
		int error_no = WSAGetLastError();
		error_display("SendPacket:WSASend", error_no);
		while (true);
	}
}
void Server::SendPacket(User*& user, unsigned char *packet) {
	Overlap_ex *over = new Overlap_ex;
	memset(over, 0, sizeof(Overlap_ex));
	over->operation = OP_SEND;
	over->wsabuf.buf = reinterpret_cast<CHAR *>(over->iocp_buffer);
	over->wsabuf.len = packet[0];
	memcpy(over->iocp_buffer, packet, packet[0]);

	int ret = WSASend(user->getSocket(), &over->wsabuf, 1, NULL, 0,
		&over->original_overlap, NULL);
	if (0 != ret) {
		int error_no = WSAGetLastError();
		error_display("SendPacket:WSASend", error_no);
		while (true);
	}
}

void Server::SendAttackPlayerPacket(Object*& user, Object*& attack_obj) {
	sc_packet_attack packet;
	packet.id = user->getId();
	packet.size = sizeof(packet);
	packet.type = SC_ATTACK;
	packet.attack_type = 0;
	packet.hp = user->getHp();
	SendPacket(user, reinterpret_cast<unsigned char *>(&packet));
}

void Server::send_chat_packet(Object*& to, Object*& teller_id, WCHAR *message) {
	sc_packet_chat chat_packet;
	chat_packet.size = sizeof(chat_packet);
	chat_packet.type = SC_CHAT;
	chat_packet.id = teller_id->getId();
	wcsncpy(chat_packet.message, message, MAX_STR_SIZE);
	SendPacket(object[to->getId()], reinterpret_cast<unsigned char *>(&chat_packet));
}

template<class T>
void Server::SendStatChangePacket(Object*& user, T*& change_obj) {
	sc_packet_stat_change packet;
	packet.id = change_obj->getId();
	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;
	packet.hp = change_obj->getHp();
	if (Is_NPC(change_obj->getId()) == false) {
		auto cast_change_obj = dynamic_cast<User*>(change_obj);
		packet.level = cast_change_obj->getLevel();
		packet.exp = cast_change_obj->getExp();
		packet.gold = cast_change_obj->getGold();
		packet.attack_damage = cast_change_obj->getAttack_damage();
	}
	SendPacket(user, reinterpret_cast<unsigned char *>(&packet));
}

bool Server::Is_InRange(Object*& a, Object*& b) {
	int dist = (a->getPoint_x() - b->getPoint_x())
		*(a->getPoint_x() - b->getPoint_x())
		+ (a->getPoint_y() - b->getPoint_y())
		* (a->getPoint_y() - b->getPoint_y());
	return dist <= VIEW_RADIUS * VIEW_RADIUS;
}

bool Server::Npc_Aggro_InRange(Object*& a, Object*& b) {
	int dist = (a->getPoint_x() - b->getPoint_x())
		*(a->getPoint_x() - b->getPoint_x())
		+ (a->getPoint_y() - b->getPoint_y())
		* (a->getPoint_y() - b->getPoint_y());
	return dist <= AGGRO_RADIUS * AGGRO_RADIUS;
}

bool Server::UserAttackInRange(Object*&a, Object*& b) {
	int dist = (a->getPoint_x() - b->getPoint_x())
		*(a->getPoint_x() - b->getPoint_x())
		+ (a->getPoint_y() - b->getPoint_y())
		* (a->getPoint_y() - b->getPoint_y());
	return dist <= ATTACK_RADIUS * ATTACK_RADIUS;
}

// ViewList에 추가하기
void Server::SendPutPlayerPacket(Object*& obj, Object*& put_obj) {
	sc_packet_put_player packet;
	packet.id = put_obj->getId();
	packet.obj_type = put_obj->getType();
	packet.size = sizeof(packet);
	packet.type = SC_PUT_PLAYER;
	packet.x = put_obj->getPoint_x();
	packet.y = put_obj->getPoint_y();
	packet.hp = put_obj->getHp();
	SendPacket(obj, reinterpret_cast<unsigned char *>(&packet));
}

// ViewList에서 삭제하기
void Server::SendRemovePlayerPacket(Object*& obj, Object*& rem_obj) {
	sc_packet_remove_player packet;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;
	packet.id = rem_obj->getId();

	SendPacket(obj, reinterpret_cast<unsigned char *>(&packet));
}
void Server::SendRemovePlayerPacket(unordered_set<Object*>::iterator& obj, Object*& rem_obj) {
	sc_packet_remove_player packet;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;
	packet.id = rem_obj->getId();

	SendPacket(obj, reinterpret_cast<unsigned char *>(&packet));
}
void Server::sector_decision(Object*& obj) {
	//int x = obj->point.x / 10;
	//int y = obj->point.y / 10;
	//// 유저의 첫 로그인일때 섹터 정해주기
	//Invocation inv;
	//if (obj->sector.x == -1 && obj->sector.y == -1) {
	//	obj->sector.x = x;
	//	obj->sector.y = y;
	//	inv.method = METHOD::SET;
	//	inv.sector.Set(obj->id);
	//	sector[y][x].Apply(inv, 0); // set = 0, 1, 2 get = 3, 4, 5, remove = 6
	//								//sector[y][x].put_object(obj);
	//}
	//if (false == (obj->sector.x == x && obj->sector.y == y)) {
	//	inv.method = METHOD::SET;
	//	inv.sector.Set(obj->id);
	//	sector[y][x].Apply(inv, 0);
	//	//sector[y][x].put_object(obj);
	//	inv.method = METHOD::REMOVE;
	//	inv.sector.Remove(obj->id);
	//	sector[obj->sector.y][obj->sector.x].Apply(inv, 8);
	//	//sector[obj->sector.y][obj->sector.x].remove_objet(obj);

	//	obj->sector.x = x;
	//	obj->sector.y = y;
	//}
	//cout << "섹터 id:" << id << " x:" << clients[id].avatar.sector_x << ",y:" << clients[id].avatar.sector_y << endl;
}

unordered_set<Object*> Server::near_list_create(Object*& obj, unordered_set<Object*>& list) {
	for (auto i : object) {
		if (i.second == NULL) continue;
		if (Is_InRange(i.second, obj) == false) continue;
		if (i.second->getId() == obj->getId()) continue;
		list.insert(i.second);
	}
	//int x = obj->sector.x;
	//int y = obj->sector.y;

	//Invocation inv;
	//Response res[9];
	//inv.method = METHOD::GET;
	//// 내가 속한 곳의 유저들 다 넣고
	////sector[y][x].all_get_id(obj, list);
	//res[0] = sector[y][x].Apply(inv, 4);
	//
	//// 주위에 있는 섹터의 유저들 다 넣기
	//if (x - 1 > -1) res[1] = sector[y][x - 1].Apply(inv, 4); 
	//if (x + 1 < BOARD_WIDTH) res[2] = sector[y][x + 1].Apply(inv, 4);
	//if (y - 1 > -1) res[3] = sector[y - 1][x].Apply(inv, 5);
	//if (y + 1 < BOARD_HEIGHT) res[4] = sector[y + 1][x].Apply(inv, 5);
	//if (x - 1 > -1 && y - 1 > -1) res[5] = sector[y - 1][x - 1].Apply(inv, 5);
	//if (x + 1 < BOARD_WIDTH && y + 1 < BOARD_HEIGHT) res[6] = sector[y + 1][x + 1].Apply(inv, 6);
	//if (x + 1 < BOARD_WIDTH && y - 1 > -1) res[7] = sector[y - 1][x + 1].Apply(inv, 6);
	//if (x - 1 > -1 && y + 1 < BOARD_HEIGHT) res[8] = sector[y + 1][x - 1].Apply(inv, 6);
	///*if (x - 1 > -1) sector[y][x - 1].all_get_id(obj, list);
	//if (x + 1 < BOARD_WIDTH) sector[y][x + 1].all_get_id(obj, list);
	//if (y - 1 > -1) sector[y - 1][x].all_get_id(obj, list);
	//if (y + 1 < BOARD_HEIGHT) sector[y + 1][x].all_get_id(obj, list);
	//if (x - 1 > -1 && y - 1 > -1) sector[y - 1][x - 1].all_get_id(obj, list);
	//if (x + 1 < BOARD_WIDTH && y + 1 < BOARD_HEIGHT) sector[y + 1][x + 1].all_get_id(obj, list);
	//if (x + 1 < BOARD_WIDTH && y - 1 > -1) sector[y - 1][x + 1].all_get_id(obj, list);
	//if (x - 1 > -1 && y + 1 < BOARD_HEIGHT) sector[y + 1][x - 1].all_get_id(obj, list);*/

	//for (auto j : res) {
	//	if (j.sector.players.size() == 0) continue;
	//	for (auto i : j.sector.players) {
	//		auto iter = object.find(i);
	//		if (iter != object.end())
	//			list.insert(iter->second);
	//	}
	//}
	//
	//// 주위에 있는 섹터의 유저들중에 시야에 없는애들은 빼기
	//vector<Object*> remove;
	//for (auto iter : list) {
	//	if (Is_InRange(obj, iter) == false)
	//		remove.push_back(iter);
	//}
	//for(auto iter : remove)
	//	list.erase(iter);

	return list;
}

void Server::TimerThreadStart() {
	while (true) {
		Sleep(1);
		timer_lock.lock();
		while (false == timer_queue.empty()) {
			if (timer_queue.top().wakeup_time > GetTickCount()) break;
			event_type ev = timer_queue.top();
			timer_queue.pop();
			timer_lock.unlock();

			// 유저와 몬스터 나누기
			Overlap_ex *over = new Overlap_ex;
			over->operation = ev.event_id;
			over->caller_id = ev.caller_id;
			PostQueuedCompletionStatus(IOCP, 1,
				ev.obj_id,
				&(over->original_overlap));
			timer_lock.lock();
		}
		timer_lock.unlock();
	}
}

void Server::add_timer(int obj_id, int m_sec, int event_type, int caller_id) {
	struct event_type event_object = { obj_id, m_sec + GetTickCount(), event_type, caller_id };
	timer_lock.lock();
	timer_queue.push(event_object);
	timer_lock.unlock();
}

// 콜러가 인자로 드루감
void Server::Lua_Do_Move(int i, int key) { // i - 콜러,타겟,유저   key - npc
	if (false == object[key]->getIs_active()) {
		object[key]->setAggro(-1);
		return;
	}
	if (NULL == object[i]) {
		object[key]->setAggro(-1);
		return;
	}
	auto t = object.find(i);
	if (t == object.end()) { 
		object[key]->setIs_active(false);
		return; 
	}
	//if (GetTickCount() - clients[key].avatar.last_move_time < 1000) return;

	int x = object[key]->getPoint_x();
	int y = object[key]->getPoint_y();

	// i의 near들
	unordered_set<Object*> near_list;
	near_list_create(object[key], near_list);
	int key_aggro = object[key]->getAggro();
	int key_type = object[key]->getType();
	if (key_aggro != -1) {
		switch (key_type) {
		case 0:
			if (Npc_Aggro_InRange(object[key], object[key_aggro]) == false) {
				for (auto ne : near_list) {
					if (Npc_Aggro_InRange(ne, object[key])) {
						int ne_id = ne->getId();
						if (Is_NPC(ne_id)) continue;
						//object[key]->setAggro(ne_id);
						key_aggro = ne_id;
						break;
					}
					else {
						//object[key]->setAggro(-1);
						key_aggro = -1;
					}
				}
			}
			break;
		case 1:
			if (Is_InRange(object[key], object[key_aggro]) == false) {
				//object[key]->setAggro(-1);
				key_aggro = -1;
			}
			break;
		case 2:
			if (Is_InRange(object[key], object[key_aggro]) == false) {
				//object[key]->setAggro(-1);
				key_aggro = -1;
			}
			break;
		default:
			break;
		}
	}
	else {
		if (key_type == 0) {
			for (auto ne : near_list) {
				if (Npc_Aggro_InRange(ne, object[key])) {
					int ne_id = ne->getId();
					if (Is_NPC(ne->getId())) continue;
					key_aggro = ne_id;
					break;
				}
				else {
					key_aggro = -1;
				}
			}
		}
	}

	// x, y값 받아온다
	lua_getglobal(object[key]->getL(), "event_player_astar_moveX");
	lua_pushnumber(object[key]->getL(), key_aggro);
	lua_pushnumber(object[key]->getL(), key_type);
	lua_pcall(object[key]->getL(), 2, 1, 0);

	object[key]->setAggro(key_aggro);
	x = (int)lua_tonumber(object[key]->getL(), -1);

	// 길을 다 찾거나 길이 막혔을땨
	if (x == -1) {
		//int now = GetTickCount();
		add_timer(key, MON_MOVE_DURATION, OP_MOVE, i);
		lua_pop(object[key]->getL(), 1);
		return;
	}

	lua_getglobal(object[key]->getL(), "event_player_astar_moveY");
	lua_pushnumber(object[key]->getL(), key_aggro);
	lua_pushnumber(object[key]->getL(), key_type);
	lua_pcall(object[key]->getL(), 2, 1, 0);

	y = (int)lua_tonumber(object[key]->getL(), -1);

	lua_pop(object[key]->getL(), 1);


	object[key]->setPoint(x, y);
	sector_decision(object[key]);
	// 움직인 직후의 near리스트
	unordered_set<Object*> new_near_list;
	near_list_create(object[key], new_near_list);

	// view=near new=new_near
	for (auto pl : near_list) {
		int pl_id = pl->getId();
		if (0 == new_near_list.count(pl)) {
			if (Is_NPC(pl_id)) continue;
			pl->vl_lock.lock();
			pl->VLerase(object[key]);
			pl->vl_lock.unlock();
			sc_packet_remove_player packet;
			packet.id = key;
			packet.size = sizeof(packet);
			packet.type = SC_REMOVE_PLAYER;
			SendPacket(pl, reinterpret_cast<unsigned char *>(&packet));
		}
		else {
			if (Is_NPC(pl_id)) continue;
			sc_packet_pos packet;
			packet.id = key;
			packet.obj_type = key_type;
			packet.size = sizeof(packet);
			packet.type = SC_POS;
			packet.x = x;
			packet.y = y;
			SendPacket(pl, reinterpret_cast<unsigned char *>(&packet));
		}
	}
	for (auto pl : new_near_list) {
		if (Is_NPC(pl->getId())) continue;
		if (0 != near_list.count(pl)) continue;
		sc_packet_put_player packet;
		packet.id = key;
		packet.obj_type = key_type;
		packet.size = sizeof(packet);
		packet.type = SC_PUT_PLAYER;
		packet.x = x;
		packet.y = y;
		packet.hp = object[key]->getHp();
		SendPacket(pl, reinterpret_cast<unsigned char *>(&packet));
	}

	vector<int> remove;
	for (auto i : new_near_list) {
		int i_id = i->getId();
		if (Is_NPC(i_id)) continue;
		remove.push_back(i_id);
	}
	if (true == remove.empty()) {
		object[key]->setIs_active(false);
	}
	else
		add_timer(key, MON_MOVE_DURATION, OP_MOVE, i);
}

void Server::Auto_Recovery(int key) {
	if (NULL == object[key]) return;
	auto t = object.find(key);
	if (t == object.end()) return;
	auto cast_key = dynamic_cast<User*>(object[key]);
	object[key]->setHp(object[key]->getHp() + 10);

	SendStatChangePacket(object[key], object[key]);

	//for (auto i : cast_key->view_list) {
	//	if (Is_NPC(i->id)) continue;
	//	SendStatChangePacket(object[key], object[key]);
	//}
	unordered_set<Object*>::iterator i = object[key]->VLbeginiter();
	while (i != object[key]->VLenditer()) {
		if (Is_NPC((*i)->getId())) {
			++i;
			continue;
		}
		SendStatChangePacket(object[key], object[key]);
		++i;
	}
	add_timer(key, AUTO_RECOVERY_DURATION, OP_EVENT_PLAYER_AUTO_RECOVERY, key);
}

void Server::ReZen(int key) {
	if (Is_NPC(key)) {
		object[key]->setPoint(rand() % BOARD_WIDTH, rand() % BOARD_HEIGHT);
		//cout << "rezen " << key << ", " << object[key]->point.x << ", " << object[key]->point.y << endl;
		object[key]->setAttack_duration();
		object[key]->sector.x = -1;
		object[key]->sector.y = -1;
		int key_type = object[key]->getType();
		if (key_type == 0) {
			object[key]->setHp(100);
		}
		else if (key_type == 1) {
			object[key]->setHp(200);
		}
		else if (key_type == 2) {
			object[key]->setHp(300);
		}
		sector_decision(object[key]);
		unordered_set<Object*> near_list;
		near_list_create(object[key], near_list);
		int user_count = 0;
		int caller = -1;
		for (auto i : near_list) {
			int i_id = i->getId();
			if (Is_NPC(i_id)) continue;
			i->vl_lock.lock();
			if (0 == i->VLcount(object[key])) {
				i->VLinsert(object[key]);
				i->vl_lock.unlock();
				SendPutPlayerPacket(i, object[key]);
				caller = i_id;
			}
			else {
				i->vl_lock.unlock();
				sc_packet_pos mov_packet;
				mov_packet.id = key;
				mov_packet.obj_type = key_type;
				mov_packet.size = sizeof(mov_packet);
				mov_packet.type = SC_POS;
				mov_packet.x = object[key]->getPoint_x();
				mov_packet.y = object[key]->getPoint_y();
				SendPacket(i, reinterpret_cast<unsigned char *>(&mov_packet));
				caller = i_id;
			}
			user_count++;
		}

		if (user_count == 0) {
			object[key]->setIs_active(false);
		}
		else {
			add_timer(key, MON_MOVE_DURATION, OP_MOVE, caller);
		}
	}
	else {

	}
}

const bool Server::Is_NPC(const int id) {
	return id < USER_START;
}

const bool Server::Is_Active(Monster*& npc) {
	return npc->is_active;
}

int Server::API_SendMessage(lua_State *L) {
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);

	char *message = (char *)lua_tostring(L, -1);

	// 형변환의 과정
	lua_pop(L, 3);
	if (user_id < USER_START) {
		return 0;
	}
	size_t wlen;
	size_t len = strlen(message) + 1;
	wchar_t wmessage[MAX_STR_SIZE];
	len = (MAX_STR_SIZE - 1 < len) ? MAX_STR_SIZE - 1 : len;
	mbstowcs_s(&wlen, wmessage, len, message, _TRUNCATE);
	wmessage[MAX_STR_SIZE - 1] = (wchar_t)0;
	Server* s;
	s->send_chat_packet(object[user_id], object[my_id], wmessage);
	return 0;
}

int Server::API_get_x(lua_State *L) {
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto temp = object.find(user_id);
	auto temp_obj = temp->second;
	int x = temp_obj->getPoint_x();
	lua_pushnumber(L, x);
	return 1;
}

int Server::API_get_y(lua_State *L) {
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto temp = object.find(user_id);
	auto temp_obj = temp->second;
	int y = temp_obj->getPoint_y();
	lua_pushnumber(L, y);
	return 1;
}

int Server::API_SendAttackMessage(lua_State *L) {
	int my_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);

	// 형변환의 과정
	lua_pop(L, 2);
	object[user_id]->setHp(object[user_id]->getHp() - 5);
	Server* s;
	if (object[user_id]->getHp() > 0) {
		sc_packet_attack packet;
		packet.id = user_id;
		packet.size = sizeof(packet);
		packet.type = SC_ATTACK;
		packet.attack_type = 0;
		packet.hp = object[user_id]->getHp();
		s->SendPacket(object[user_id], reinterpret_cast<unsigned char *>(&packet));
		unordered_set<Object*> near_list;
		s->near_list_create(object[user_id], near_list);
		for (auto i : near_list) {
			if (s->Is_NPC(i->getId())) continue;
			s->SendPacket(i, reinterpret_cast<unsigned char *>(&packet));
		}
	}
	else {
		object[user_id]->setHp(100);
		object[user_id]->setPoint(4, 4);
		object[user_id]->sector.x = -1;
		object[user_id]->sector.y = -1;
		s->sector_decision(object[user_id]);
		object[user_id]->vl_lock.lock();

		//for (auto i : cast_obj->view_list) {
		//	if (s->Is_NPC(i->id)) continue;
		//	auto cast_i = dynamic_cast<User*>(i);
		//	cast_i->vl_lock.lock();
		//	if (cast_i->view_list.count(object[user_id]) == 1) {
		//		cast_i->view_list.erase(object[user_id]);
		//		cast_i->vl_lock.unlock();
		//		s->SendRemovePlayerPacket(i, object[user_id]);
		//	} else cast_i->vl_lock.unlock();
		//}

		object[user_id]->VLclear();
		unordered_set<Object*> near_list;
		s->near_list_create(object[user_id], near_list);
		for (auto i : near_list) {
			object[user_id]->VLinsert(i);
			if (s->Is_NPC(i->getId())) continue;
			i->vl_lock.lock();
			i->VLinsert(object[user_id]);
			i->vl_lock.unlock();
			s->SendPutPlayerPacket(i, object[user_id]);
		}
		object[user_id]->vl_lock.unlock();

		s->SendStatChangePacket(object[user_id], object[user_id]);
		sc_packet_pos mov_packet;
		mov_packet.id = object[user_id]->getId();
		mov_packet.obj_type = USER;
		mov_packet.size = sizeof(mov_packet);
		mov_packet.type = SC_POS;
		mov_packet.x = object[user_id]->getPoint_x();
		mov_packet.y = object[user_id]->getPoint_y();
		s->SendPacket(object[user_id], reinterpret_cast<unsigned char *>(&mov_packet));
	}
	return 0;
}