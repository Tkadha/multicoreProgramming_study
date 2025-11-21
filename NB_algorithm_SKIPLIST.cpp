#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <numeric>
#include <set>

const int MAX_THREADS = 16;

class NODE {
public:
	int value;
	NODE* volatile next;
	std::mutex mtx;
	volatile bool removed;
	NODE(int x) : next(nullptr), value(x), removed(false) {}
	void lock() { mtx.lock(); }
	void unlock() { mtx.unlock(); }
};

class DUMMY_MTX {
public:
	void lock() {}
	void unlock() {}
};

#include <queue>


int num_threads = 0;
thread_local int thread_id = 0;

constexpr int MAX_LEVEL = 9;

class SKNODE {
public:
	int value;
	SKNODE* volatile next[MAX_LEVEL + 1];
	int top_level;
	volatile bool marked;
	volatile bool fully_linked;
	std::recursive_mutex mtx;
	SKNODE(int x, int top) : value(x), top_level(top),
		marked(false), fully_linked(false)
	{
		for (auto& p : next) p = nullptr;
	}
	SKNODE() : value(-1), top_level(0),
		marked(false), fully_linked(false)
	{
		for (auto& p : next) p = nullptr;
	}
};

class C_SKLIST {
private:
	SKNODE* head, * tail;
	DUMMY_MTX mtx;
public:
	C_SKLIST() {
		head = new SKNODE(std::numeric_limits<int>::min(), MAX_LEVEL);
		tail = new SKNODE(std::numeric_limits<int>::max(), MAX_LEVEL);
		for (auto& p : head->next) p = tail;
	}

	~C_SKLIST()
	{
		clear();
		delete head;
		delete tail;
	}

	void clear()
	{
		SKNODE* curr = head->next[0];
		while (curr != tail) {
			SKNODE* temp = curr;
			curr = curr->next[0];
			delete temp;
		}
		for (auto& p : head->next) p = tail;
	}

	void find(SKNODE* prevs[], SKNODE* currs[], int x)
	{
		auto prev = head;
		for (int level = MAX_LEVEL; level >= 0; --level) {
			auto curr = prev->next[level];
			while (curr->value < x) {
				prev = curr;
				curr = curr->next[level];
			}
			prevs[level] = prev;
			currs[level] = curr;
		}
	}

	bool add(int x)
	{
		SKNODE* prevs[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		mtx.lock();
		find(prevs, currs, x);

		if (currs[0]->value == x) {
			mtx.unlock();
			return false;
		}
		else {
			int node_level = 0;
			for (node_level = 0; node_level < MAX_LEVEL; ++node_level)
				if (rand() % 2 == 0) break;
			auto newNode = new SKNODE(x, node_level);

			for (int level = 0; level <= node_level; ++level) {
				newNode->next[level] = currs[level];
				prevs[level]->next[level] = newNode;
			}
			mtx.unlock();
			return true;
		}
	}

	bool remove(int x)
	{
		SKNODE* prevs[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		mtx.lock();
		find(prevs, currs, x);

		if (currs[0]->value != x) {
			mtx.unlock();
			return false;
		}
		else {
			for (int level = 0; level <= currs[0]->top_level; ++level)
				prevs[level]->next[level] = currs[0]->next[level];
			mtx.unlock();
			delete currs[0];
			return true;
		}
	}

	bool contains(int x)
	{
		SKNODE* prevs[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		mtx.lock();
		find(prevs, currs, x);
		bool res = (currs[0]->value == x);
		mtx.unlock();
		return res;
	}

	void print20()
	{
		auto curr = head->next[0];
		for (int i = 0; i < 20 && curr != tail; ++i) {
			std::cout << curr->value << ", ";
			curr = curr->next[0];
		}
		std::cout << std::endl;
	}
};

class Z_SKLIST {
private:
	SKNODE* head, * tail;
public:
	Z_SKLIST() {
		head = new SKNODE(std::numeric_limits<int>::min(), MAX_LEVEL);
		tail = new SKNODE(std::numeric_limits<int>::max(), MAX_LEVEL);
		for (auto& p : head->next) p = tail;
		head->fully_linked = tail->fully_linked = true;
	}

	~Z_SKLIST()
	{
		clear();
		delete head;
		delete tail;
	}

	void clear()
	{
		SKNODE* curr = head->next[0];
		while (curr != tail) {
			SKNODE* temp = curr;
			curr = curr->next[0];
			delete temp;
		}
		for (auto& p : head->next) p = tail;
	}

	int find(SKNODE* prevs[], SKNODE* currs[], int x)
	{
		int max_level_found = -1;
		auto prev = head;
		for (int level = MAX_LEVEL; level >= 0; --level) {
			auto curr = prev->next[level];
			while (curr->value < x) {
				prev = curr;
				curr = curr->next[level];
			}
			if (max_level_found == -1 && curr->value == x)
				max_level_found = level;
			prevs[level] = prev;
			currs[level] = curr;
		}
		return max_level_found;
	}

	bool add(int x)
	{
		SKNODE* prevs[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		int node_level = 0;
		for (node_level = 0; node_level < MAX_LEVEL; ++node_level)
			if (rand() % 2 == 0) break;

		while (true) {
			int found_level = find(prevs, currs, x);

			if (found_level != -1) {
				if (false == currs[found_level]->marked) {
					while (false == currs[found_level]->fully_linked) {}
					return false;
				}
				continue;
			}


			int highest_locked = -1;
			bool valid = true;
			for (int level = 0; valid && level <= node_level; ++level) {
				prevs[level]->mtx.lock();
				highest_locked = level;

				valid = (!prevs[level]->marked && !currs[level]->marked && prevs[level]->next[level] == currs[level]);
			}

			if (false == valid) {
				for (int level = 0; level <= highest_locked; ++level)
					prevs[level]->mtx.unlock();
				continue;
			}
			auto newNode = new SKNODE{ x, node_level };

			for (int level = 0; level <= node_level; ++level) {
				newNode->next[level] = currs[level];
			}
			for (int level = 0; level <= node_level; ++level) {
				prevs[level]->next[level] = newNode;
			}

			for (int level = 0; level <= highest_locked; ++level)
				prevs[level]->mtx.unlock();

			newNode->fully_linked = true;
			return true;
		}
	}

	bool remove(int x)
	{
		SKNODE* prevs[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];

		SKNODE* victim = nullptr;
		bool is_marked = false;   // victim을 이미 mark했는지 여부
		int top_level = -1;

		while (true) {
			int f_level = find(prevs, currs, x);

			if (!is_marked) {
				if (f_level == -1) return false;

				victim = currs[f_level];

				if (victim->marked)            return false;
				if (!victim->fully_linked)     return false;
				if (victim->top_level != f_level) return false;

				victim->mtx.lock();
				if (victim->marked) {
					victim->mtx.unlock();
					return false;
				}

				victim->marked = true;
				top_level = victim->top_level;
				is_marked = true;
			}
			else {
				if (f_level == -1) {
					victim->mtx.unlock();
					return false;
				}
			}

			bool valid = true;
			int highest_locked = -1;
			for (int i = 0; i <= top_level; ++i) {
				prevs[i]->mtx.lock();
				highest_locked = i;

				if (prevs[i]->marked || prevs[i]->next[i] != victim) {
					valid = false;
					break;
				}
			}

			if (!valid) {
				for (int i = 0; i <= highest_locked; ++i)
					prevs[i]->mtx.unlock();
				continue;
			}

			for (int i = top_level; i >= 0; --i) {
				prevs[i]->next[i] = victim->next[i];
			}

			for (int i = 0; i <= highest_locked; ++i)
				prevs[i]->mtx.unlock();

			victim->mtx.unlock();
			// delete victim;

			return true;
		}
	}


	bool contains(int x)
	{
		SKNODE* prevs[MAX_LEVEL + 1];
		SKNODE* currs[MAX_LEVEL + 1];
		int f_level = find(prevs, currs, x);
		return (f_level != -1)
			&& (currs[f_level]->fully_linked == true)
			&& (currs[f_level]->marked == false);
	}

	void print20()
	{
		auto curr = head->next[0];
		for (int i = 0; i < 20 && curr != tail; ++i) {
			std::cout << curr->value << ", ";
			curr = curr->next[0];
		}
		std::cout << std::endl;
	}
};

class LFSKNODE;
class AMRSK { // atomic markable reference
	volatile long long ptr_and_mark;
public:
	AMRSK(LFSKNODE* ptr = nullptr, bool mark = false) {
		long long val = reinterpret_cast<long long> (ptr);
		if (true == mark) val |= 1;

		ptr_and_mark = val;
	}
	~AMRSK() {}


	LFSKNODE* get_ptr() {
		long long val = ptr_and_mark;
		return reinterpret_cast<LFSKNODE*>(val & 0xFFFF'FFFF'FFFF'FFFE);
	}
	bool get_mark() {
		return (1 == (ptr_and_mark & 1));
	}

	LFSKNODE* get_ptr_and_mark(bool* mark) {
		long long val = ptr_and_mark;
		*mark = (1 == (val & 1));
		return reinterpret_cast<LFSKNODE*>(val & 0xFFFF'FFFF'FFFF'FFFE);
	}

	bool attempt_mark(LFSKNODE* expected_ptr, bool new_mark) {
		return CAS(expected_ptr, expected_ptr, false, new_mark);
	}

	bool CAS(LFSKNODE* expected_ptr, LFSKNODE* new_ptr,
		bool expectedMark, bool newMark) {

		long long oldval = reinterpret_cast<long long>(expected_ptr);
		if (expectedMark) oldval |= 1;

		long long newval = reinterpret_cast<long long>(new_ptr);
		if (newMark) newval |= 1;

		return std::atomic_compare_exchange_strong(
			reinterpret_cast<volatile std::atomic<long long>*>(&ptr_and_mark),
			&oldval, newval);
	}
};

class LFSKNODE {
public:
	int value;
	AMRSK next[MAX_LEVEL + 1];
	int top_level;
	LFSKNODE(int x, int top) : value(x), top_level(top)
	{
		for (auto& p : next) p = nullptr;
	}
	LFSKNODE() : value(-1), top_level(0)
	{
		for (auto& p : next) p = nullptr;
	}
};


class LF_SKLIST {
private:
	LFSKNODE* head, * tail;
public:
	LF_SKLIST() {
		head = new LFSKNODE(std::numeric_limits<int>::min(), MAX_LEVEL);
		tail = new LFSKNODE(std::numeric_limits<int>::max(), MAX_LEVEL);
		for (auto& p : head->next) p = tail;
	}

	~LF_SKLIST()
	{
		clear();
		delete head;
		delete tail;
	}

	void clear()
	{
		LFSKNODE* curr = head->next[0].get_ptr();
		while (curr != tail) {
			LFSKNODE* temp = curr;
			curr = curr->next[0].get_ptr();
			delete temp;
		}
		for (auto& p : head->next) p = tail;
	}

	bool find(LFSKNODE* prevs[], LFSKNODE* currs[], int x)
	{
		retry:
		auto prev = head;
		for (int level = MAX_LEVEL; level >= 0; --level) {
			auto curr = prev->next[level].get_ptr();
			while (true) {
				bool removed;
				auto succ = curr->next[level].get_ptr_and_mark(&removed);
				while (removed) {
					if (!prev->next[level].CAS(curr, succ, false, false))
						goto retry;
					curr = succ;
					succ = curr->next[level].get_ptr_and_mark(&removed);
				}
				if (curr->value < x) {
					prev = curr;
					curr = succ;
				}
				else break;
			}
			prevs[level] = prev;
			currs[level] = curr;
		}
		return currs[0]->value == x;
	}

	bool add(int x)
	{
		LFSKNODE* prevs[MAX_LEVEL + 1];
		LFSKNODE* currs[MAX_LEVEL + 1];

		while (true) {
			if (true == find(prevs, currs, x)) return false;

			int node_level = 0;
			for (node_level = 0; node_level < MAX_LEVEL; ++node_level)
				if (rand() % 2 == 0) break;
			
			auto newnode = new LFSKNODE{ x,node_level };
			for (int level = 0;level <= node_level;++level) {
				newnode->next[level] = currs[level];
			}
			auto pred = prevs[0];
			auto curr = currs[0];
			if (false == pred->next[0].CAS(curr, newnode, false, false)) {
				delete newnode;
				continue;
			}

			for (int level = 1;level <= node_level;++level) {
				while (true) {
					pred = prevs[level];
					curr = currs[level];
					if (pred->next[level].CAS(curr, newnode, false, false)) break;
					find(prevs, currs, x);
				}
			}
			return true;
		}
	}

	bool remove(int x)
	{
		LFSKNODE* prevs[MAX_LEVEL + 1];
		LFSKNODE* currs[MAX_LEVEL + 1];


		if (false == find(prevs, currs, x))return false;
		auto victim = currs[0];
		int top_level = victim->top_level;

		for (int level = top_level;level >= 1;--level) {
			bool removed{ false };
			auto succ = victim->next[level].get_ptr_and_mark(&removed);
			while (!removed) {
				victim->next[level].CAS(succ, succ, false, true);
				succ = victim->next[level].get_ptr_and_mark(&removed);
			}
		}
		bool removed = false;
		auto succ = victim->next[0].get_ptr_and_mark(&removed);
		if (removed) {
			find(prevs, currs, x);
			return false;
		}
		while (true) {
			bool i_marked_it = victim->next[0].CAS(succ, succ, false, true);
			succ = victim->next[0].get_ptr_and_mark(&removed);
			if (i_marked_it) {
				find(prevs, currs, x);
				return true;
			}
			else if (removed) {
				find(prevs, currs, x);
				return false;
			}
		}
	}

	bool contains(int x)
	{
		LFSKNODE* prev = head;
		LFSKNODE* curr = nullptr;
		for (int i = MAX_LEVEL;i >= 0;--i) {
			curr = prev->next[i].get_ptr();
			while (true) {
				bool removed;
				auto succ = curr->next[i].get_ptr_and_mark(&removed);
				while (removed) {
					curr = succ;
					succ = curr->next[i].get_ptr_and_mark(&removed);
				}
				if (curr->value < x) {
					prev = curr;
					curr = succ;
				}
				else break;
			}
		}
		return curr->value == x;
	}

	void print20()
	{
		auto curr = head->next[0].get_ptr();
		for (int i = 0; i < 20 && curr != tail; ++i) {
			std::cout << curr->value << ", ";
			curr = curr->next[0].get_ptr();
		}
		std::cout << std::endl;
	}
};


LF_SKLIST set;

const int LOOP = 400'0000;
const int RANGE = 1000;

#include <array>

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

std::array<std::vector<HISTORY>, MAX_THREADS> history;

void check_history(int num_threads)
{
	std::array <int, RANGE> survive = {};
	std::cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		std::cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (false == op.o_value) continue;
			if (op.op == 3) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			std::cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			std::cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (set.contains(i)) {
				std::cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == set.contains(i)) {
				std::cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	std::cout << " OK\n";
}

void benchmark_check(int num_threads, int th_id)
{
	thread_id = th_id;
	for (int i = 0; i < LOOP / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % RANGE;
			history[th_id].emplace_back(0, v, set.add(v));
			break;
		}
		case 1: {
			int v = rand() % RANGE;
			history[th_id].emplace_back(1, v, set.remove(v));
			break;
		}
		case 2: {
			int v = rand() % RANGE;
			history[th_id].emplace_back(2, v, set.contains(v));
			break;
		}
		}
	}
}
void benchmark(const int num_threads, int th_id)
{
	thread_id = th_id;
	for (int i = 0; i < LOOP / num_threads; ++i) {
		int value = rand() % RANGE;
		int op = rand() % 3;
		if (op == 0) set.add(value);
		else if (op == 1) set.remove(value);
		else set.contains(value);
	}
}

int main()
{
	using namespace std::chrono;
	// Consistency check
	std::cout << "Consistency Check\n";
	for (num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		set.clear();
		std::vector<std::thread> threads;
		for (int i = 0; i < MAX_THREADS; ++i)
			history[i].clear();
		auto start = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(benchmark_check, num_threads, i);
		for (auto& th : threads)
			th.join();
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << "Threads: " << num_threads
			<< ", Duration: " << duration.count() << " ms.\n";
		std::cout << "Set: "; set.print20();
		check_history(num_threads);
		//set.recycle();
	}
	std::cout << "\nBenchmarking\n";
	for (num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		set.clear();
		std::vector<std::thread> threads;
		auto start = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(benchmark, num_threads, i);
		for (auto& th : threads)
			th.join();
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << "Threads: " << num_threads
			<< ", Duration: " << duration.count() << " ms.\n";
		std::cout << "Set: "; set.print20();
		//set.recycle();
	}
}