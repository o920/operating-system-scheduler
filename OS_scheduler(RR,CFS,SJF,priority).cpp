#define _CRT_SEECURE_NO_WARNINGS
#include <iostream>
#include <queue>
#include <deque>
#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#define MAX 20
using namespace std;

queue<int> roundrobin;	//Roundrobin에 대한 큐
queue<int> CFS;			//completely fair scheduling에 대한 큐
queue<int> SJF;			//short job first에 대한 큐

int cla[MAX] = {};
int pri[MAX] = {};		//프로세스의 우선순위
int pid[MAX] = {};		//프로세스의 id
int cpuburst[MAX] = {};	//cpu burst length

int c, id, p, b;
int a = 0;

void *RoundRobin(void *arg);
void *C_F_S(void* arg);
void *S_J_F(void* arg);

int main(int argc, char*argv[]) {
	freopen( "input.txt", "r", stdin);		//파일로 입력받기
	freopen( "output.txt", "w", stdout);	//파일로 출력하기

	while (scanf("%d %d %d %d", &c, &id, &p, &b) != (int)EOF) {
		cla[a] = c;
		pri[a] = p;
		if (c == 0) roundrobin.push(a);
		if (c == 1) CFS.push(a);
		if (c == 2) SJF.push(a);
		pid[a] = id;
		cpuburst[a] = b;
		a++;
	}
	pthread_t t0, t1, t2;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_create(&t0, &attr, RoundRobin, NULL);
	pthread_join(t0, NULL);
	pthread_create(&t1, &attr, C_F_S, NULL);
	pthread_join(t1, NULL);
	pthread_create(&t2, &attr, S_J_F, NULL);
	pthread_join(t2, NULL);

	return 0;


}
void *RoundRobin(void* arg) {
	//roundrobin
	deque<int> round;				//timequantum 만큼 수행 후 맨 뒤에서 기다리는 큐
	int tq = 4;						//time quantum
	//round 큐에 프로세스 넣기
	for (int j = 0; j < a; j++) {
		if (cla[j] != 0) continue;
		int min = 987654321;
		int min_idx = 0;
		for (int i = 0; i < a; i++) {
			if (cla[i] != 0) continue;
			if (pri[i] < min) {
				min = pri[i];
				min_idx = i;
			}
		}
		for (int i = 0; i < cpuburst[min_idx]; i++) 
			round.push_back(min_idx);
		pri[min_idx] = 987654321;
	}
	int idx = 0;
	while (round.size() > 0) {
		for (int i = 0; i < tq; i++) {						// timequantum 만큼 수행하고 큐의 뒤로 보내기
			if (cpuburst[round.front()] > tq) {
				idx = round.front();
				for (int j = 0; j < tq; j++) {
					cout << pid[idx] << " ";
					round.pop_front();
				}
				cout << endl;
				cpuburst[idx] -= tq;
				for (int j = 0; j < cpuburst[idx]; j++) {
					round.push_back(idx);
					round.pop_front();
				}
				break;
			}
			else if (cpuburst[round.front()] == tq) {
				idx = round.front();
				for (int j = 0; j < tq; j++) {
					cout << pid[idx] << " ";
					round.pop_front();
				}
				cout << endl;
				break;
			}
			else if (cpuburst[round.front()] < tq) {
				idx = round.front();
				for (int j = 0; j < cpuburst[idx]; j++) {
					cout << pid[idx] << " ";
					round.pop_front();
				}
				cout << endl;
				break;
			}
		}
	}
	pthread_exit(0);
}

void *C_F_S(void* arg) {
	//CFS
	int vruntime[MAX] = { 0, };
	int weight[MAX] = { 0, };
	int delta_exec[MAX] = { 0, };
	int ready[MAX] = { 0, };
	bool ready_check[MAX] = { 0, };
	bool check[MAX] = { 0, }; // 중복확인

	for (int i = 0; i < a; i++) {
		if (cla[i] != 1)continue;
		if (pri[i] == 7) weight[i] = 12;
		if (pri[i] == 8) weight[i] = 10;
		if (pri[i] == 9) weight[i] = 8;
		if (pri[i] == 10) weight[i] = 6;
		if (pri[i] == 11) weight[i] = 4;
		if (pri[i] == 12) weight[i] = 2;
		ready[i] = 0;
	}
	int prior = 0;
	while (1) {
		//초기화
		for (int i = 0; i < a; i++) {
			check[i] = 0;
			ready_check[i] = 0;
		}
		//vruntime 업데이트
		for (int i = 0; i < a; i++) {
			if (cla[i] != 1)continue;
			vruntime[i] += (12 / weight[i]) * delta_exec[i];
		}

		//vruntime 최소값 찾기
		int vrunmin = 987654321;
		int vrunmin_idx = 0;
		for (int i = 0; i < a; i++) {
			if (cla[i] != 1)continue;
			if (cpuburst[i] == 0)continue;
			if (vruntime[i]<=vrunmin) {
				vrunmin_idx = i;
				vrunmin = vruntime[i];
			}
		}
		// vruntime 값이 중복인 경우 찾기
		int count = 0;
		for (int i = 0; i < a; i++) {
			if (cla[i] != 1)continue;
			if (cpuburst[i] == 0)continue;
			if (vruntime[i] == vrunmin) {
				count++;
				check[i] = true;
			}
		}
		
		// vruntime 값이 중복이 있는 경우
		if (count > 1) {
			//ready확인 -> priority 확인
			int readymax = 0;		// ready 최댓값 찾기
			int readymax_idx = 0;	// 그때의 인덱스
			int ready_count = 0;	// ready도 중복이 있는지 확인하기
			//ready 최댓값 찾기
			for (int i = 0; i < a; i++) {
				if (cla[i] != 1)continue;
				if (cpuburst[i] == 0)continue;
				if (check[i] == false)continue;
				if (ready[i] > readymax) {
					readymax = ready[i];
					readymax_idx = i;
				}
			}
			//ready 중복값 찾기
			for (int i = 0; i < a; i++) {
				if (cla[i] != 1)continue;
				if (cpuburst[i] == 0)continue;
				if (ready[i] == readymax) {
					ready_count++;
					ready_check[i] = true;
				}
			}
			//ready 중복이 있는 경우 -> priority가 가장 높은프로세스 수행
			if (ready_count > 1) {
				int primin = 12;
				int primin_idx = 0;
				for (int i = 0; i < a; i++) {
					if (cla[i] != 1)continue;
					if (cpuburst[i] == 0) continue;
					if (check[i] == false) continue;
					if (ready_check[i] == false) continue;
					if (pri[i] < primin) {
						primin = pri[i];
						primin_idx = i;
					}
				}
				if (prior != readymax_idx) cout << endl;
				cout << pid[primin_idx] << " ";
				prior = primin_idx;
				delta_exec[primin_idx]++;
				cpuburst[primin_idx]--;
				for (int i = 0; i < a; i++) {
					if (cla[i] != 1)continue;
					if (i == primin_idx) continue;
					ready[i]++;
				}
			}
			//ready 중복이 없는 경우, ready 값이 큰 프로세스 수행
			else {
				if (prior != readymax_idx) cout << endl;
				cout << pid[readymax_idx] << " ";
				prior = readymax_idx;
				delta_exec[readymax_idx]++;
				cpuburst[readymax_idx]--;
				for (int i = 0; i < a; i++) {
					if (cla[i] != 1)continue;
					if (i == readymax_idx) continue;
					ready[i]++;
				}
			}

		}
		// vruntime 값의 중복이 없는 경우
		else {
			if (prior != vrunmin_idx) cout << endl;
			cout << pid[vrunmin_idx] <<" ";
			prior = vrunmin_idx;
			delta_exec[vrunmin_idx]++;
			cpuburst[vrunmin_idx]--;
			for (int i = 0; i < a; i++) {
				if (cla[i] != 1)continue;
				if (i == vrunmin_idx) continue;
				ready[i]++;
			}
		}
		int rest = 0;
		for (int i = 0; i < a; i++) {
			if (cla[i] != 1)continue;
			rest += cpuburst[i];
		}
		if (rest == 0) break;
		
	}
	cout << endl;
	pthread_exit(0);
}

void *S_J_F(void* arg) {
	pair<int, int> s[MAX];
	for (int i = 0; i < a; i++) {
		if (cla[i] != 2) continue;
		s[i].first = i;
		s[i].second = cpuburst[i];
	}
	while (1) {
		//최소 cpu burst 찾기
		int min = 987654321;
		int min_idx = 0;
		for (int i = 0; i < a; i++) {
			if (cla[i] != 2) continue;
			if (cpuburst[i] == 0) continue;
			if (s[i].second < min) {
				min = s[i].second;
				min_idx = s[i].first;
			}
		}
		for (int i = 0; i < cpuburst[min_idx]; i++) { cout << pid[min_idx] << " "; }
		cpuburst[min_idx] = 0;
		cout << endl;
		
		int rest = 0;
		for (int i = 0; i < a; i++) {
			if (cla[i] != 2) continue;
			rest += cpuburst[i];
		}
		if (rest == 0) break;
	}
	pthread_exit(0);
}
