#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <numeric>
#include <set>

using namespace std::chrono;
const int MAX_THREADS = 16;

class NODE {
public:
	int value;
	NODE* next;
	std::mutex mu;
	bool removed;
	NODE() :next(nullptr), removed(false) {}
	NODE(int x) :next(nullptr), value(x), removed(false) {}
	void lock() { mu.lock(); }
	void unlock() { mu.unlock(); }
};

class DUMMY_MUTEX {
public:
	void lock() {}
	void unlock() {}
};

class C_SET {
private:
	NODE* head, * tail;
	std::mutex mu;
	//DUMMY_MUTEX mu;
public:
	C_SET() {
		head = new NODE(std::numeric_limits<int>::min());
		tail = new NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~C_SET() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		NODE* curr = head->next;
		while (curr != tail) {
			NODE* temp = curr;
			curr = curr->next;
			delete temp;
		}
		head->next = tail;
	}

	bool add(int x) {
		NODE* prev = head;
		mu.lock();
		NODE* curr = prev->next;

		while (curr->value < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->value == x) {
			mu.unlock();
			return false;
		}
		else {
			NODE* newnode = new NODE(x);
			newnode->next = curr;
			prev->next = newnode;
			mu.unlock();
			return true;
		}

	}
	bool remove(int x) {
		NODE* prev = head;
		mu.lock();
		NODE* curr = prev->next;

		while (curr->value < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->value == x) {

			NODE* temp = curr;
			prev->next = curr->next;
			mu.unlock();
			delete temp;
			return true;
		}
		else {
			mu.unlock();
			return false;
		}
	}
	bool contains(int x) {
		NODE* prev = head;
		mu.lock();
		while (prev->value < x) {
			prev = prev->next;
		}
		if (prev->value == x) {
			mu.unlock();
			return true;
		}
		else {
			mu.unlock();
			return false;
		}
	}
	void print20() {
		auto curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			std::cout << curr->value << ", ";
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};

class F_SET {
private:
	NODE* head, * tail;
public:
	F_SET() {
		head = new NODE(std::numeric_limits<int>::min());
		tail = new NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~F_SET() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		NODE* curr = head->next;
		while (curr != tail) {
			NODE* temp = curr;
			curr = curr->next;
			delete temp;
		}
		head->next = tail;
	}

	bool add(int x) {
		NODE* prev = head;
		prev->lock();
		NODE* curr = prev->next;
		curr->lock();

		while (curr->value < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->value == x) {
			prev->unlock(); curr->unlock();
			return false;
		}
		else {
			NODE* newnode = new NODE(x);
			newnode->next = curr;
			prev->next = newnode;
			prev->unlock(); curr->unlock();
			return true;
		}

	}
	bool remove(int x) {
		NODE* prev = head;
		prev->lock();
		NODE* curr = prev->next;
		curr->lock();

		while (curr->value < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->value == x) {

			NODE* temp = curr;
			prev->next = curr->next;
			prev->unlock(); curr->unlock();
			delete temp;
			return true;
		}
		else {
			prev->unlock(); curr->unlock();
			return false;
		}
	}
	bool contains(int x) {
		NODE* prev = head;
		prev->lock();
		while (prev->value < x) {
			prev->unlock();
			prev = prev->next;
			prev->lock();
		}
		if (prev->value == x) {
			prev->unlock();
			return true;
		}
		else {
			prev->unlock();
			return false;
		}
	}
	void print20() {
		auto curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			std::cout << curr->value << ", ";
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};

class O_SET {
private:
	NODE* head, * tail;
public:
	O_SET() {
		head = new NODE(std::numeric_limits<int>::min());
		tail = new NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~O_SET() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		NODE* curr = head->next;
		while (curr != tail) {
			NODE* temp = curr;
			curr = curr->next;
			delete temp;
		}
		head->next = tail;
	}

	bool validate(int x, NODE* p, NODE* c) {
		NODE* prev = head;
		NODE* curr = prev->next;
		while (curr->value < x) {
			prev = curr;
			curr = curr->next;
		}
		return ((prev == p) && (curr == c));
	}

	bool add(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {
				prev->unlock(); curr->unlock();
				return false;
			}
			else {
				NODE* newnode = new NODE(x);
				newnode->next = curr;
				prev->next = newnode;
				prev->unlock(); curr->unlock();
				return true;
			}
		}

	}
	bool remove(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}

			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {

				NODE* temp = curr;
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				//delete temp;
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool contains(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;
			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {
				prev->unlock(); curr->unlock();
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	void print20() {
		auto curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			std::cout << curr->value << ", ";
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};

class L_SET {
private:
	NODE* head, * tail;
public:
	L_SET() {
		head = new NODE(std::numeric_limits<int>::min());
		tail = new NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~L_SET() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		NODE* curr = head->next;
		while (curr != tail) {
			NODE* temp = curr;
			curr = curr->next;
			delete temp;
		}
		head->next = tail;
	}

	bool validate(int x, NODE* p, NODE* c) {
		return (!p->removed && !c->removed && p->next == c);
	}

	bool add(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {
				prev->unlock(); curr->unlock();
				return false;
			}
			else {
				NODE* newnode = new NODE(x);
				newnode->next = curr;
				prev->next = newnode;
				prev->unlock(); curr->unlock();
				return true;
			}
		}

	}
	bool remove(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}

			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {

				curr->removed = true;
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				//delete temp;
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool contains(int x) {
		while (true) {
			NODE* prev = head;
			while (prev->value < x) {
				prev = prev->next;
			}
			return prev->value == x && !prev->removed;
		}
	}
	void print20() {
		auto curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			if (!curr->removed) {
				std::cout << curr->value << ", ";
			}
			else --i;
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};

#include <queue>
class L_SET_FL {
private:
	NODE* head, * tail;
	std::queue<NODE*> free_list;
	std::mutex fl_mu;
public:
	void my_delete(NODE* node) {
		std::lock_guard<std::mutex> lg(fl_mu);
		free_list.push(node);
	}
	void recycle() {
		while (!free_list.empty()) {
			auto node = free_list.front();
			free_list.pop();
			delete node;
		}
	}
	L_SET_FL() {
		head = new NODE(std::numeric_limits<int>::min());
		tail = new NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~L_SET_FL() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		NODE* curr = head->next;
		while (curr != tail) {
			NODE* temp = curr;
			curr = curr->next;
			delete temp;
		}
		head->next = tail;
	}

	bool validate(int x, NODE* p, NODE* c) {
		return (!p->removed && !c->removed && p->next == c);
	}

	bool add(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {
				prev->unlock(); curr->unlock();
				return false;
			}
			else {
				NODE* newnode = new NODE(x);
				newnode->next = curr;
				prev->next = newnode;
				prev->unlock(); curr->unlock();
				return true;
			}
		}

	}
	bool remove(int x) {
		while (true) {
			NODE* prev = head;
			NODE* curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}

			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {

				curr->removed = true;
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				my_delete(curr);
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool contains(int x) {
		while (true) {
			NODE* prev = head;
			while (prev->value < x) {
				prev = prev->next;
			}
			return prev->value == x && !prev->removed;
		}
	}
	void print20() {
		auto curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			if (!curr->removed) {
				std::cout << curr->value << ", ";
			}
			else --i;
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};

/*
class NODE_SP {
public:
	int value;
	std::shared_ptr<NODE_SP> next;
	std::mutex mu;
	volatile bool removed;
	NODE_SP() :next(nullptr), removed(false) {}
	NODE_SP(int x) :next(nullptr), value(x), removed(false) {}
	void lock() { mu.lock(); }
	void unlock() { mu.unlock(); }
};

class L_SET_SP {
private:
	std::shared_ptr<NODE_SP> head, tail;
public:
	L_SET_SP() {
		head = std::make_shared <NODE_SP>(std::numeric_limits<int>::min());
		tail = std::make_shared <NODE_SP>(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~L_SET_SP() {
		clear();
	}
	void clear() {
		head->next = tail;
	}

	bool validate(const std::shared_ptr<NODE_SP>& p, const std::shared_ptr<NODE_SP>& c) {
		return (!p->removed && !c->removed && std::atomic_load(&p->next) == c);
	}

	bool add(int x) {
		while (true) {
			std::shared_ptr<NODE_SP> prev = head;
			std::shared_ptr<NODE_SP> curr = std::atomic_load(&prev->next);

			while (curr->value < x) {
				prev = curr;
				curr = std::atomic_load(&curr->next);
			}
			prev->lock(); curr->lock();
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {
				prev->unlock(); curr->unlock();
				return false;
			}
			else {
				std::shared_ptr<NODE_SP> newnode = std::make_shared <NODE_SP>(x);
				newnode->next = curr;
				std::atomic_exchange(&prev->next, newnode);
				prev->unlock(); curr->unlock();
				return true;
			}
		}

	}
	bool remove(int x) {
		while (true) {
			std::shared_ptr<NODE_SP> prev = head;
			std::shared_ptr<NODE_SP> curr = std::atomic_load(&prev->next);

			while (curr->value < x) {
				prev = curr;
				curr = std::atomic_load(&curr->next);
			}

			prev->lock(); curr->lock();
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {

				curr->removed = true;
				std::atomic_exchange(&prev->next, std::atomic_load(&curr->next));
				prev->unlock(); curr->unlock();
				//delete temp;
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool contains(int x) {
		while (true) {
			std::shared_ptr<NODE_SP> prev = head;
			while (prev->value < x) {
				prev = std::atomic_load(&prev->next);
			}
			return prev->value == x && !prev->removed;
		}
	}
	void print20() {
		auto curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			if (!curr->removed) {
				std::cout << curr->value << ", ";
			}
			else --i;
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};
*/

#include <atomic>
class NODE_ASP {
public:
	int value;
	std::atomic<std::shared_ptr<NODE_ASP>> next;
	std::mutex mu;
	volatile bool removed;
	NODE_ASP() :next(nullptr), removed(false) {}
	NODE_ASP(int x) :next(nullptr), value(x), removed(false) {}
	void lock() { mu.lock(); }
	void unlock() { mu.unlock(); }
};

class L_SET_ASP {
private:
	std::shared_ptr<NODE_ASP> head, tail;
public:
	L_SET_ASP() {
		head = std::make_shared <NODE_ASP>(std::numeric_limits<int>::min());
		tail = std::make_shared <NODE_ASP>(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~L_SET_ASP() {
		clear();
	}
	void clear() {
		head->next = tail;
	}

	bool validate(const std::shared_ptr<NODE_ASP>& p, const std::shared_ptr<NODE_ASP>& c) {
		return (!p->removed && !c->removed && p->next.load() == c);
	}

	bool add(int x) {
		while (true) {
			std::shared_ptr<NODE_ASP> prev = head;
			std::shared_ptr<NODE_ASP> curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {
				prev->unlock(); curr->unlock();
				return false;
			}
			else {
				std::shared_ptr<NODE_ASP> newnode = std::make_shared <NODE_ASP>(x);
				newnode->next = curr;
				prev->next = newnode;
				prev->unlock(); curr->unlock();
				return true;
			}
		}

	}
	bool remove(int x) {
		while (true) {
			std::shared_ptr<NODE_ASP> prev = head;
			std::shared_ptr<NODE_ASP> curr = prev->next;

			while (curr->value < x) {
				prev = curr;
				curr = curr->next;
			}

			prev->lock(); curr->lock();
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->value == x) {

				curr->removed = true;
				prev->next = curr->next.load();
				prev->unlock(); curr->unlock();
				//delete temp;
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool contains(int x) {
		while (true) {
			std::shared_ptr<NODE_ASP> prev = head;
			while (prev->value < x) {
				prev = prev->next;
			}
			return prev->value == x && !prev->removed;
		}
	}
	void print20() {
		std::shared_ptr<NODE_ASP> curr = head->next;
		for (int i = 0;i < 20 && curr != tail;++i) {
			if (!curr->removed) {
				std::cout << curr->value << ", ";
			}
			else --i;
			curr = curr->next;
		}
		std::cout << std::endl;
	}
};


class LF_NODE;

class AMR { // atomic markable reference
	volatile long long ptr_and_mark;
public:
	AMR(LF_NODE* ptr = nullptr, bool mark = false) {
		long long val = reinterpret_cast<long long> (ptr);
		if (true == mark) val |= 1;

		ptr_and_mark = val;
	}
	~AMR() {}


	LF_NODE* get_ptr() {
		long long val = ptr_and_mark;
		return reinterpret_cast<LF_NODE*>(val & 0xFFFF'FFFF'FFFF'FFFE);
	}
	bool get_mark() {
		return (1 == (ptr_and_mark & 1));
	}

	LF_NODE* get_ptr_and_mark(bool* mark) {
		long long val = ptr_and_mark;
		*mark = (1 == (val & 1));
		return reinterpret_cast<LF_NODE*>(val & 0xFFFF'FFFF'FFFF'FFFE);
	}

	bool attempt_mark(LF_NODE* expected_ptr, bool new_mark) {
		return CAS(expected_ptr, expected_ptr, false, new_mark);
	}

	bool CAS(LF_NODE* expected_ptr, LF_NODE* new_ptr,
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

class LF_NODE {
public:
	int value;
	AMR next;
	int epoch;
	LF_NODE(int x) :value(x) {}
};

class LF_SET {
private:
	LF_NODE* head, * tail;
public:
	LF_SET() {
		head = new LF_NODE(std::numeric_limits<int>::min());
		tail = new LF_NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~LF_SET() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		LF_NODE* curr = head->next.get_ptr();
		while (curr != tail) {
			LF_NODE* temp = curr;
			curr = curr->next.get_ptr();
			delete temp;
		}
		head->next = tail;
	}

	void find(LF_NODE*& prev, LF_NODE*& curr, int x)
	{
		while (true)
		{
			retry:
			prev = head;
			curr = prev->next.get_ptr();
			while (true) {
				bool curr_mark;
				auto succ = curr->next.get_ptr_and_mark(&curr_mark);
				while (true == curr_mark) {
					if (false == prev->next.CAS(curr, succ, false, false)) goto retry;
					curr = succ;
					succ = curr->next.get_ptr_and_mark(&curr_mark);
				}
				if (curr->value >= x) return;
				prev = curr;
				curr = succ;
			}
		}
	}

	bool add(int x) {
		while (true) {
			LF_NODE* prev, *curr;
			find(prev, curr, x);

			if (curr->value == x) {
				return false;
			}
			else {
				auto newnode = new LF_NODE(x);
				newnode->next = curr;
				if (true == prev->next.CAS(curr, newnode, false, false)) return true;
			}
		}

	}
	bool remove(int x) {
		while (true) {
			LF_NODE* prev, * curr;
			find(prev, curr, x);
			
			if (curr->value == x) {
				auto succ = curr->next.get_ptr();
				if (false == curr->next.attempt_mark(succ, true)) continue;
				prev->next.CAS(curr, succ, false, false);
				return true;
			}
			else {
				return false;
			}
		}
	}
	bool contains(int x) {
		LF_NODE* curr = head;
		while (curr->value < x) {
			curr = curr->next.get_ptr();
		}
		return (curr->next.get_mark() == false) && (curr->value == x);
	}
	void print20() {
		auto p = head->next.get_ptr();

		for (int i = 0; i < 20; ++i) {
			if (tail == p) break;
			std::cout << p->value << ", ";
			p = p->next.get_ptr();
		}
		std::cout << std::endl;
	}
};

std::atomic_int g_ebr_counter{};

int num_thread{};
thread_local int thread_id{};

class EBR {
private:
	std::queue<LF_NODE*> free_list[MAX_THREADS];
	std::atomic_int epoch_counter;
	struct THREAD_COUNTER {
		alignas(64) std::atomic_int local_epoch;
	};
	THREAD_COUNTER thread_counter[MAX_THREADS];
public:

	EBR() = default;
	~EBR() {
		recycle();
	}
	void recycle() {
		for (int i = 0;i < MAX_THREADS;++i) {
			while (false == free_list[i].empty()) {
				auto node = free_list[i].front();
				free_list[i].pop();
				delete node;
			}
			thread_counter[i].local_epoch = 0;
			epoch_counter = 0;
		}

	}
	void delete_node(LF_NODE* node) {
		node->epoch = epoch_counter;
		free_list[thread_id].push(node);
	}
	LF_NODE* new_node(int x) {
		if (false == free_list[thread_id].empty()) {
			auto node = free_list[thread_id].front();
			bool can_reuse = true;
			for (int i = 0;i < num_thread;++i) {
				if (i == thread_id) continue;
				if (thread_counter[i].local_epoch <= node->epoch) {
					can_reuse = false;
					break;
				}
			}
			if (true == can_reuse) {
				free_list[thread_id].pop();
				node->value = x;
				node->next = nullptr;
				return node;
			}
		}
		return new LF_NODE(x);
	}
	void increase_ebr_counter() {
		thread_counter[thread_id].local_epoch.store(++epoch_counter);
	}
	void max_th_ebr_counter() {
		thread_counter[thread_id].local_epoch.store(std::numeric_limits<int>::max());
	}
};

class LF_SET_EBR {
private:
	EBR ebr;
	LF_NODE* head, * tail;
public:
	LF_SET_EBR() {
		head = new LF_NODE(std::numeric_limits<int>::min());
		tail = new LF_NODE(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~LF_SET_EBR() {
		clear();
		delete head;
		delete tail;
	}
	void clear() {
		LF_NODE* curr = head->next.get_ptr();
		while (curr != tail) {
			LF_NODE* temp = curr;
			curr = curr->next.get_ptr();
			delete temp;
		}
		head->next = tail;
		ebr.recycle();
	}

	void find(LF_NODE*& prev, LF_NODE*& curr, int x)
	{
		while (true)
		{
		retry:
			prev = head;
			curr = prev->next.get_ptr();
			while (true) {
				bool curr_mark;
				auto succ = curr->next.get_ptr_and_mark(&curr_mark);
				while (true == curr_mark) {
					if (false == prev->next.CAS(curr, succ, false, false)) goto retry;
					ebr.delete_node(curr);
					curr = succ;
					succ = curr->next.get_ptr_and_mark(&curr_mark);
				}
				if (curr->value >= x) return;
				prev = curr;
				curr = succ;
			}
		}
	}

	bool add(int x) {
		ebr.increase_ebr_counter();
		while (true) {
			LF_NODE* prev, * curr;
			find(prev, curr, x);

			if (curr->value == x) {
				ebr.max_th_ebr_counter();
				return false;
			}
			else {
				auto newnode = ebr.new_node(x);
				newnode->next = curr;
				if (true == prev->next.CAS(curr, newnode, false, false)) {
					ebr.max_th_ebr_counter();
					return true;
				}
				else ebr.delete_node(newnode);
			}
		}

	}
	bool remove(int x) {
		ebr.increase_ebr_counter();
		while (true) {
			LF_NODE* prev, * curr;
			find(prev, curr, x);

			if (curr->value == x) {
				auto succ = curr->next.get_ptr();
				if (false == curr->next.attempt_mark(succ, true)) continue;
				if (true == prev->next.CAS(curr, succ, false, false)) {
					ebr.delete_node(curr);
				}
				ebr.max_th_ebr_counter();
				return true;
			}
			else {
				ebr.max_th_ebr_counter();
				return false;
			}
		}
	}
	bool contains(int x) {
		ebr.increase_ebr_counter();
		LF_NODE* curr = head->next.get_ptr();
		while (curr->value < x) {
			curr = curr->next.get_ptr();
		}
		ebr.max_th_ebr_counter();
		return (curr->next.get_mark() == false) && (curr->value == x);
	}
	void print20() {
		auto p = head->next.get_ptr();

		for (int i = 0; i < 20; ++i) {
			if (tail == p) break;
			std::cout << p->value << ", ";
			p = p->next.get_ptr();
		}
		std::cout << std::endl;
	}
};

enum INVO_OP { ADD = 0, REMOVE, CONTAINS };
class INVOCATION {
public:
	INVO_OP op;
	int value;
	INVOCATION(INVO_OP o, int v) : op(o), value(v) {}
};

typedef bool RESPONSE;

// single thread api
class SEQ_SET {
	std::set<int> m_set;
public:
	RESPONSE apply(INVOCATION inv) {
		switch (inv.op) {
		case ADD:
			return m_set.insert(inv.value).second;
		case REMOVE:
			return (m_set.erase(inv.value) > 0);
		case CONTAINS:
			return (m_set.find(inv.value) != m_set.end());
		default:
			return false;
		}
	}
	void clear() { m_set.clear(); }
	void print20() {
		int cnt{};
		for (auto& n : m_set) {
			std::cout << n << ", ";
			if (++cnt >= 20) break;
		}
		std::cout << '\n';
	}
};


class LNODE;

class CONSENSUS {
	LNODE* value{ nullptr };
public:
	LNODE* decide(LNODE* v)
	{
		CAS(&value, nullptr, v);
		return value;
	}
	void CAS(LNODE** addr, LNODE* expected, LNODE* update)
	{
		std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<LNODE*>*>(addr),
			&expected, update);
	}
	void clear()
	{
		value = nullptr;
	}
};

class LNODE {
public:
	INVOCATION m_inv;
	int	m_seq;
	LNODE* m_next;
	CONSENSUS decide_next;
	LNODE(INVOCATION inv) : m_inv(inv), m_seq(0), m_next(nullptr) {}
};

class LFU_SET {
	LNODE* head[MAX_THREADS];
	LNODE* tail;
public:
	LFU_SET() {
		tail = new LNODE(INVOCATION(CONTAINS, 0)); // dummy
		for (int i = 0; i < MAX_THREADS; ++i) {
			head[i] = tail;
		}
	}

	~LFU_SET()
	{
		while (nullptr != tail) {
			LNODE* temp = tail;
			tail = tail->m_next;
			delete temp;
		}
	}

	LNODE* max_head()
	{
		LNODE* max_node = head[0];
		for (int i = 1; i < num_thread; ++i) {
			if (max_node->m_seq < head[i]->m_seq)
				max_node = head[i];
		}
		return max_node;
	}

	RESPONSE apply(INVOCATION inv)
	{
		int i = thread_id;
		auto prefer = new LNODE(inv);
		while (prefer->m_seq == 0) {
			LNODE* before = max_head();
			LNODE* after = before->decide_next.decide(prefer);
			before->m_next = after;
			after->m_seq = before->m_seq + 1;
			head[i] = after;
		}

		SEQ_SET seq_set;
		LNODE* curr = tail->m_next;
		while (curr != prefer) {
			seq_set.apply(curr->m_inv);
			curr = curr->m_next;
		}
		return seq_set.apply(inv);
	};

	void clear()
	{
		for (int i = 0; i < num_thread; ++i) {
			head[i] = tail;
		}
		LNODE* curr = tail->m_next;
		while (nullptr != curr) {
			LNODE* temp = curr;
			curr = curr->m_next;
			delete temp;
		}
		tail->m_next = nullptr;
		tail->decide_next.clear();
	}

	void print20()
	{
		SEQ_SET seq_set;
		LNODE* curr = tail->m_next;
		while (nullptr != curr) {
			seq_set.apply(curr->m_inv);
			curr = curr->m_next;
		}
		seq_set.print20();
	}
};

class WFU_SET {
	LNODE* announce[MAX_THREADS];
	LNODE* head[MAX_THREADS];
	LNODE* tail;
public:
	WFU_SET() {
		tail = new LNODE(INVOCATION(CONTAINS, 0)); // dummy
		tail->m_seq = 1;
		for (int i = 0; i < MAX_THREADS; ++i) {
			head[i] = tail;
			announce[i] = tail;
		}
	}

	~WFU_SET()
	{
		while (nullptr != tail) {
			LNODE* temp = tail;
			tail = tail->m_next;
			delete temp;
		}
	}

	LNODE* max_head()
	{
		LNODE* max_node = head[0];
		for (int i = 1; i < num_thread; ++i) {
			if (max_node->m_seq < head[i]->m_seq)
				max_node = head[i];
		}
		return max_node;
	}

	RESPONSE apply(INVOCATION inv)
	{
		int i = thread_id;
		announce[i] = new LNODE(inv);
		head[i] = max_head();
		while (announce[i]->m_seq == 0) {
			LNODE* before = head[i];
			LNODE* help = announce[((before->m_seq + 1) % num_thread)];
			LNODE* prefer;
			if (help->m_seq == 0) prefer = help;
			else prefer = announce[i];
			LNODE* after = before->decide_next.decide(prefer);
			before->m_next = after;
			after->m_seq = before->m_seq + 1;
			head[i] = after;
		}

		SEQ_SET seq_set;
		LNODE* curr = tail->m_next;
		while (curr != announce[i]) {
			seq_set.apply(curr->m_inv);
			curr = curr->m_next;
		}
		head[i] = announce[i];
		return seq_set.apply(inv);
	};

	void clear()
	{
		for (int i = 0; i < num_thread; ++i) {
			head[i] = tail;
			announce[i] = tail;
		}
		LNODE* curr = tail->m_next;
		while (nullptr != curr) {
			LNODE* temp = curr;
			curr = curr->m_next;
			delete temp;
		}
		tail->m_seq = 1;
		tail->m_next = nullptr;
		tail->decide_next.clear();
	}

	void print20()
	{
		SEQ_SET seq_set;
		LNODE* curr = tail->m_next;
		while (nullptr != curr) {
			seq_set.apply(curr->m_inv);
			curr = curr->m_next;
		}
		seq_set.print20();
	}
};

// benchmarking ¿ë
class STD_SET {
private:
	WFU_SET m_set;
	std::mutex mu;
public:
	STD_SET() = default;
	~STD_SET() = default;
	void clear() {
		m_set.clear();
	}

	bool add(int x) {
		mu.lock();
		auto res = m_set.apply({ ADD, x });
		mu.unlock();
		return res;

	}
	bool remove(int x) {
		mu.lock();
		auto res = m_set.apply({ REMOVE, x });
		mu.unlock();
		return res;
	}
	bool contains(int x) {
		mu.lock();
		auto res = m_set.apply({ CONTAINS, x });
		mu.unlock();
		return res;
	}
	void print20() {
		m_set.print20();
	}
};




STD_SET set;


const int LOOP = 1'0000;
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

	for (num_thread = 1; num_thread <= MAX_THREADS; num_thread *= 2) {
		set.clear();
		std::vector<std::thread> threads;
		for (int i = 0; i < MAX_THREADS; ++i)
			history[i].clear();
		auto start = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i)
			threads.emplace_back(benchmark_check, num_thread, i);
		for (auto& th : threads)
			th.join();
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << "Threads: " << num_thread
			<< ", Duration: " << duration.count() << " ms.\n";
		std::cout << "Set: "; set.print20();
		check_history(num_thread);
	}
	std::cout << "\n\nBenchMarking Check\n";

	for (num_thread = 1; num_thread <= MAX_THREADS; num_thread *= 2) {
		set.clear();
		std::vector<std::thread> threads;
		auto start = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i)
			threads.emplace_back(benchmark, num_thread, i);
		for (auto& th : threads)
			th.join();
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << "Threads: " << num_thread
			<< ", Duration: " << duration.count() << " ms.\n";
		std::cout << "Set: "; set.print20();
	}
}