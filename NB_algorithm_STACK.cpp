#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <set>
#include <unordered_set>
#include <immintrin.h>

const int MAX_THREADS = 16;
int num_threads{};

class NODE {
public:
	int value;
	NODE* volatile next;
	NODE(int v) : value(v), next(nullptr) {}
};

class DUMMY_MUTEX {
public:
	void lock() {}
	void unlock() {}
};

class C_STACK {
	NODE* top;
	std::mutex mu;
public:
	C_STACK() {
		top = nullptr;
	}

	~C_STACK() {
		clear();
	}

	void clear() {
		while (nullptr != top) pop();
	}

	void push(int x)
	{
		NODE* new_node = new NODE(x);
		mu.lock();
		new_node->next = top;
		top = new_node;
		mu.unlock();
	}

	int pop()
	{
		mu.lock();
		if (nullptr == top) {
			mu.unlock();
			return -2;
		}
		int res = top->value;
		auto temp = top;
		top = top->next;
		mu.unlock();
		delete temp;
		return res;
	}

	void print20()
	{
		NODE* curr = top;
		for (int i = 0; i < 20 && curr != nullptr; i++, curr = curr->next)
			std::cout << curr->value << ", ";
		std::cout << "\n";
	}
};

class LF_STACK {
	NODE* top;
public:
	LF_STACK() {
		top = nullptr;
	}

	~LF_STACK() {
		clear();
	}

	void clear() {
		while (nullptr != top) pop();
	}

	bool CAS(NODE* expected, NODE* new_value)
	{
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<NODE*>*>(&top),
			&expected,
			new_value);
	}

	void push(int x)
	{
		NODE* new_node = new NODE(x);
		while (true) {
			NODE* before_top = top;
			new_node -> next = before_top;
			if (true == CAS(before_top, new_node)) return;
		}
	}

	int pop()
	{
		while (true) {
			NODE* before_top = top;
			if (nullptr == before_top) {
				return -2;
			}
			NODE* next = before_top->next;
			int res = before_top->value;
			if (before_top != top) continue;
			if (true == CAS(before_top, next)) {
				return res;
			}
		}
	}

	void print20()
	{
		NODE* curr = top;
		for (int i = 0; i < 20 && curr != nullptr; i++, curr = curr->next)
			std::cout << curr->value << ", ";
		std::cout << "\n";
	}
};

class BACKOFF { 
	int minDelay, maxDelay;
	int limit;
public:
	BACKOFF(int min, int max)
		: minDelay(min), maxDelay(max), limit(min) {
		if (0 == limit) {
			std::cout << "Error limit == 0\n";
			exit(-1);
		}
	}
	void InterruptedException() { 
		// 문제점: 너무 김(micro sec은 3000clock)
		// 정확한 딜레이가 생성되지 않음
		// - Sleep_for는 OS 호출
		// - OS 호출은 몇 micro sec의 딜레이가 있음
		//	- sleep_for는 정밀하지 않음, OS의 스케줄러가 담당
		//	- sleep_for는 nano sec으로는 호출 안됨
		int delay = rand() % limit;
		limit += limit;
		if (limit > maxDelay) limit = maxDelay;
		//std::this_thread::sleep_for(std::chrono::microseconds(delay));
		for (int i = 0; i < delay; i++) _mm_pause();
	}
};


class LFBO_STACK {
	NODE* volatile top;
public:
	LFBO_STACK() {
		top = nullptr;
	}

	~LFBO_STACK() {
		clear();
	}

	void clear() {
		while (nullptr != top) pop();
	}

	bool CAS(NODE* expected, NODE* new_value)
	{
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<volatile std::atomic<NODE*>*>(&top),
			&expected,
			new_value);
	}

	void push(int x)
	{
		BACKOFF bo(1, num_threads);
		NODE* new_node = new NODE(x);
		while (true) {
			NODE* before_top = top;
			new_node->next = before_top;
			if (true == CAS(before_top, new_node)) return;
			bo.InterruptedException();
		}
	}

	int pop()
	{
		BACKOFF bo(1, num_threads);
		while (true) {
			NODE* before_top = top;
			if (nullptr == before_top) {
				return -2;
			}
			NODE* next = before_top->next;
			if (before_top != top) continue;
			int res = before_top->value;
			if (true == CAS(before_top, next)) return res;
			bo.InterruptedException();
		}
	}

	void print20()
	{
		NODE* curr = top;
		for (int i = 0; i < 20 && curr != nullptr; i++, curr = curr->next)
			std::cout << curr->value << ", ";
		std::cout << "\n";
	}
};


constexpr int ST_EMPTY = 0;
constexpr int ST_WAITING = 1;
constexpr int ST_BUSY = 2;
constexpr int TIME_OUT = 100;

class LockFreeExchanger {
	alignas(64) std::atomic_llong slot;
public:
	LockFreeExchanger() : slot(0) { }
	int exchange(int my_item, bool* busy) {
		*busy = false;
		for (int j = 0; j < TIME_OUT; ++j) {
			long long s = slot;
			int item = (int)(s & 0xFFFF'FFFF);
			int status = (int)((s >> 32) & 0x3);
			switch (status) {
			case ST_EMPTY: {
				long long new_s = ((long long)my_item & 0xFFFF'FFFF) | ((long long)ST_WAITING << 32);
				if (std::atomic_compare_exchange_strong(&slot, &s, new_s)) {
					int spins = 0;
					for (int i = 0;i < TIME_OUT;++i) {
						s = slot;
						status = (int)((s >> 32) & 0x3);
						if (status == ST_BUSY) {
							int their_item = (int(s & 0xFFFFFFFF));
							slot = 0;
							return their_item;
						}
					}
					if (std::atomic_compare_exchange_strong(&slot, &s, 0)) {
						return -2;	// TIME OUT
					}
					else { // BUSY
						s = slot;
						int their_item = (int)(s & 0xFFFF'FFFF);
						slot = 0;
						return their_item;
					}
				}
			}
			break;
			case ST_WAITING: {
				long long new_s = ((long long)my_item & 0xFFFFFFFF) | ((long long)ST_BUSY << 32);
				if (std::atomic_compare_exchange_strong(&slot,&s,new_s)) {
					int their_item = item;
					return their_item;
				}
			}
			break;
			case ST_BUSY: {
				*busy = true;
			}
			break;
			}
		}
		return -2;
	}
};

class EliminationArray {
	int range;
	LockFreeExchanger exchanger[MAX_THREADS / 2 - 1];
public:
	EliminationArray() { range = 1; }
	~EliminationArray() {}
	int Visit(int value) {
		int slot = rand() % range;
		bool busy;
		int ret = exchanger[slot].exchange(value, &busy);
		int old_range = range;
		if ((ret == -2) && (old_range > 1))  // TIME OUT
			range = old_range - 1;
		if ((true == busy) && (old_range <= num_threads / 2 - 1))
			range = old_range + 1; // MAX RANGE is # of thread / 2
		return ret;
	}
};

int exchange_cnt{};

class LFEL_STACK {
	NODE* top;
	EliminationArray el_arr;
public:
	LFEL_STACK() {
		top = nullptr;
	}

	~LFEL_STACK() {
		clear();
	}

	void clear() {
		while (nullptr != top) pop();
	}

	bool CAS(NODE* volatile* addr, NODE* expected, NODE* desired)
	{
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<volatile std::atomic<NODE*>*>(addr),
			&expected,
			desired);
	}

	void push(int x)
	{
		NODE* new_node = new NODE(x);
		while (true) {
			new_node->next = top;
			if (CAS(&top, new_node->next, new_node))
				return;
			int res = el_arr.Visit(x);
			if (res == -1) {
				exchange_cnt += 1;
				return;
			}
		}
	}

	int pop()
	{
		while (true) {
			NODE* curr_top = top;
			if (nullptr == curr_top) {
				return -2;
			}
			NODE* next_node = curr_top->next;
			if (CAS(&top, curr_top, next_node)) {
				int res = curr_top->value;
				//delete curr_top;
				return res;
			}
			int result = el_arr.Visit(-1);
			if (result >= 0) {
				return result;
			}

		}
	}

	void print20()
	{
		NODE* curr = top;
		for (int i = 0; i < 20 && curr != nullptr; i++, curr = curr->next)
			std::cout << curr->value << ", ";
		std::cout << "\n";
	}
};

LFEL_STACK my_stack;

struct HISTORY {
	std::vector <int> push_values, pop_values;
};
std::atomic_int stack_size;
thread_local int thread_id;
const int NUM_TEST = 10'000'000;

void benchmark(const int num_thread)
{
	int key = 0;
	const int loop_count = NUM_TEST / num_thread;
	for (auto i = 0; i < loop_count; ++i) {
		if ((rand() % 2 == 0) || (i < 1000))
			my_stack.push(key++);
		else
			my_stack.pop();
	}
}

void benchmark_test(const int th_id, const int num_threads, HISTORY& h)
{
	thread_id = th_id;
	int loop_count = NUM_TEST / num_threads;
	for (int i = 0; i < loop_count; i++) {
		if ((rand() % 2) || i < 128 / num_threads) {
			h.push_values.push_back(i);
			stack_size++;
			my_stack.push(i);
		}
		else {
			volatile int curr_size = stack_size--;
			int res = my_stack.pop();
			if (res == -2) {
				stack_size++;
				if ((curr_size > num_threads * 2) && (stack_size > num_threads)) {
					std::cout << "ERROR Non_Empty Stack Returned NULL\n";
					exit(-1);
				}
			}
			else h.pop_values.push_back(res);
		}
	}
}

void check_history(std::vector <HISTORY>& h)
{
	std::unordered_multiset <int> pushed, poped, in_stack;

	for (auto& v : h)
	{
		for (auto num : v.push_values) pushed.insert(num);
		for (auto num : v.pop_values) poped.insert(num);
		while (true) {
			int num = my_stack.pop();
			if (num == -2) break;
			poped.insert(num);
		}
	}
	for (auto num : pushed) {
		if (poped.count(num) < pushed.count(num)) {
			std::cout << "Pushed Number " << num << " does not exists in the STACK.\n";
			exit(-1);
		}
		if (poped.count(num) > pushed.count(num)) {
			std::cout << "Pushed Number " << num << " is poped more than " << poped.count(num) - pushed.count(num) << " times.\n";
			exit(-1);
		}
	}
	for (auto num : poped)
		if (pushed.count(num) == 0) {
			std::multiset <int> sorted;
			for (auto num : poped)
				sorted.insert(num);
			std::cout << "There were elements in the STACK no one pushed : ";
			int count = 20;
			for (auto num : sorted)
				std::cout << num << ", ";
			std::cout << std::endl;
			exit(-1);

		}
	std::cout << "NO ERROR detectd.\n";
}

int main()
{
	using namespace std::chrono;

	for (int n = 1; n <= MAX_THREADS; n = n * 2) {
		num_threads = n;
		my_stack.clear();
		std::vector<std::thread> tv;
		std::vector<HISTORY> history;
		history.resize(n);
		stack_size = 0;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark_test, i, n, std::ref(history[i]));
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << " Threads,  " << ms << "ms. ----";
		my_stack.print20();
		check_history(history);
	}

	for (int n = 1; n <= MAX_THREADS; n *= 2) {
		num_threads = n;
		exchange_cnt = 0;
		my_stack.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark, n);
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << " Threads,  " << ms << "ms. ----";
		my_stack.print20();
		std::cout << "교환 성공 횟수: " << exchange_cnt << '\n';
	}
}