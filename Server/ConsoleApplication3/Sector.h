#pragma once
#include <iostream>
#include <unordered_set>
#include <atomic>
using namespace std;

class Object;
class Sector {
public:
	int temp; // v로 전달할 때 잠깐 저장하는 변수
	unordered_set<int> players;
	Sector() {}

	void Set(int id) {
		temp = id;
		players.insert(id);
	}
	void Remove(int id) {
		temp = id;
		players.erase(id);
	}

	void Get(int* outTemp, unordered_set<int>* sector_id) {
		*outTemp = temp;
		for (auto i : players)
			sector_id->insert(i);
	}
};

enum METHOD {
	SET,
	GET,
	REMOVE
};

class Invocation {
public:
	Sector sector;
	METHOD method;
};

class Response {
public:
	Sector sector;
};


class Node {
public:
	Invocation invoc;
	Node* pNext = nullptr;
	int seqNum = 0;
	Node() : pNext(nullptr), seqNum(0) {}
	Node(Invocation inv) : invoc(inv), pNext(nullptr), seqNum(0) {}
};

class SeqObject {
private:
	Sector sector;
public:
	Response Apply(Invocation inv) {
		Response response;
		switch (inv.method) {
		case METHOD::SET:
			sector.temp = inv.sector.temp;
			sector.players.insert(inv.sector.temp);
			break;
		case METHOD::GET:
			response.sector.temp = sector.temp;
			for (auto i : sector.players)
				response.sector.players.insert(i);
			break;
		case METHOD::REMOVE:
			sector.players.erase(inv.sector.temp);
			break;
		default:
			cout << "ERROR!\n";
			exit(-1);
			break;
		}
		return response;
	}
};

class LF_Sector {
private:
	Node* nodeArray[10];
	Node* pTail;

	void NodeArrayInit() {
		pTail = new Node();
		pTail->seqNum = 1;
		for (int i = 0; i < 10; ++i) {
			nodeArray[i] = pTail;
		}
	}

	Node* GetLastNode() {
		int lastSeqNum = nodeArray[0]->seqNum;
		int retNodeIndex = 0;
		for (int i = 1; i < 10; ++i) {
			if (nodeArray[i]->seqNum > lastSeqNum) {
				lastSeqNum = nodeArray[i]->seqNum;
				retNodeIndex = i;
			}
		}
		return nodeArray[retNodeIndex];
	}

	Node* CAS_and_GET(Node** addr, Node* oldNode, Node* newNode) {
		atomic_compare_exchange_strong(
			reinterpret_cast<atomic_int*>(addr),
			reinterpret_cast<int*>(&oldNode),
			reinterpret_cast<int>(newNode)
		);
		return *addr;
	}

public:
	LF_Sector() {
		NodeArrayInit();
	}

	Response Apply(Invocation invoc, int n) {
		Node* pPrefer = new Node(invoc);
		while (pPrefer->seqNum == 0) {
			Node* pBefore = GetLastNode();
			Node* pAfter = CAS_and_GET(&(pBefore->pNext), nullptr, pPrefer);
			pBefore->pNext = pAfter;
			pAfter->seqNum = pBefore->seqNum + 1;
			nodeArray[n] = pAfter;
		}
		SeqObject myObject;
		Node* pCurrent = pTail->pNext;
		while (pCurrent != pPrefer) {
			myObject.Apply(pCurrent->invoc);
			pCurrent = pCurrent->pNext;
		}
		return myObject.Apply(pCurrent->invoc);
	}
};
