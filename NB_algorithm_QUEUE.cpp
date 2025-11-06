#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>


struct NODE {
	int value;
	NODE* next;
	NODE() : value(-1) { next = nullptr; }
	NODE(int x) : value(x), next(nullptr) {}
};

class DUMMY_MUTEX {
public:
	void lock() {}
	void unlock() {}
};

class CQUEUE {
	NODE* head, * tail;
	std::mutex	mu;

public:
	CQUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	~CQUEUE() {
		clear();
		delete head;
	}

	void clear()
	{
		NODE* curr = head->next;
		while (curr != nullptr) {
			NODE* ptr = curr->next;
			delete curr;
			curr = ptr;
		}
		tail = head;
		head->next = nullptr;
	}

	void Enq(int x)
	{
		NODE* n = new NODE(x);
		mu.lock();
		tail->next = n;
		tail = n;
		mu.unlock();
	}

	int Deq()
	{
		mu.lock();
		if (head->next == nullptr) {
			mu.unlock();
			return -1;
		}
		int value = head->next->value;
		NODE* old = head;
		head = head->next;
		mu.unlock();
		delete old;
		return value;
	}

	void print20()
	{
		auto p = head->next;

		for (int i = 0; i < 20; ++i) {
			if (nullptr == p) break;
			std::cout << p->value << ", ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};

class LFQUEUE {
	NODE* volatile head, * volatile tail;
public:
	LFQUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	~LFQUEUE() {
		clear();
		delete head;
	}

	void clear()
	{
		NODE* curr = head->next;
		while (curr != nullptr) {
			auto ptr = curr->next;
			delete curr;
			curr = ptr;
		}
		tail = head;
		head->next = nullptr;
	}

	bool CAS(NODE* volatile * addr, NODE* expected, NODE* new_value) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<volatile std::atomic<NODE*>*>(addr),
			&expected, new_value);
	}

	void Enq(int x)
	{
		NODE* n = new NODE(x);
		while (true) {
			NODE* old_tail = tail;
			NODE* old_next = old_tail->next;
			if (old_tail != tail) continue;
			if (old_next == nullptr) {
				if (true == CAS(&old_tail->next, nullptr, n)) {
					CAS(&tail, old_tail, n);
					return;
				}
			}
			else CAS(&tail, old_tail, old_next);
		}
	}

	int Deq()
	{
		while (true) {
			NODE* old_head = head;
			NODE* old_next = old_head->next;
			NODE* old_tail = tail;
			if (old_head != head) continue;
			if (old_next != nullptr) return -1;
			if (old_tail == old_head) {
				CAS(&tail, old_tail, old_next);
				continue;
			}

			int value = old_next->value;
			if (true == CAS(&head, old_head, old_next)) {
				delete old_head;
				return value;
			}
		}
	}

	void print20()
	{
		auto p = head->next;

		for (int i = 0; i < 20; ++i) {
			if (nullptr == p) break;
			std::cout << p->value << ", ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};


class STNODE;
class STPTR {
public:
	std::atomic_llong raw;
	void set_ptr(STNODE* p) {
		raw = reinterpret_cast<long long>(p) << 32;
	}
	STNODE* get_ptr() {
		return reinterpret_cast<STNODE*>(raw >> 32);
	}
	STNODE* get_ptr(int* stamp) {
		long long cur_raw = raw;
		*stamp = static_cast<int>(cur_raw & 0xFFFF'FFFF);
		return reinterpret_cast<STNODE*>(cur_raw >> 32);
	}
	bool CAS(STNODE* old_val, STNODE* new_val, int old_stamp, int new_stamp) {
		long long old_raw = (reinterpret_cast<long long>(old_val) << 32) | old_stamp;
		long long new_raw = (reinterpret_cast<long long>(new_val) << 32) | new_stamp;
		return std::atomic_compare_exchange_strong(
			&raw, &old_raw, new_raw);
	}
};

class STNODE {
public:
	int value;
	STPTR next;
	STNODE(int x) : value(x) {}
};

class LFSTQUEUE {
	STPTR head, tail;
public:
	LFSTQUEUE()
	{
		head.set_ptr(new STNODE{ -1 });
		tail.set_ptr(head.get_ptr());
	}
	~LFSTQUEUE() {
		clear();
		delete head.get_ptr();
	}

	void clear()
	{
		STNODE* curr = head.get_ptr()->next.get_ptr();
		while (curr != nullptr) {
			STNODE* ptr = curr->next.get_ptr();
			delete curr;
			curr = ptr;
		}
		tail.set_ptr(head.get_ptr());
		head.get_ptr()->next.set_ptr(nullptr);
	}

	/*bool CAS(STPTR addr, STNODE* expected, STNODE* new_value, int old_stamp, int new_stamp) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<volatile std::atomic<STNODE*>*>(addr),
			&expected, new_value);
	}*/

	void Enq(int x)
	{
		STNODE* n = new STNODE(x);
		while (true) {
			int tail_stamp = 0;
			STNODE* old_tail = tail.get_ptr(&tail_stamp);
			int next_stamp = 0;
			STNODE* old_next = old_tail->next.get_ptr(&next_stamp);
			if (old_tail != tail.get_ptr()) continue;
			if (old_next == nullptr) {
				if (true == old_tail->next.CAS(nullptr, n, next_stamp, next_stamp + 1)) {
					tail.CAS(old_tail, n, tail_stamp, tail_stamp + 1);
					return;
				}
			}
			else tail.CAS(old_tail, old_next, tail_stamp, tail_stamp + 1);
		}
	}

	int Deq()
	{
		while (true) {
			int head_stamp{};
			STNODE* old_head = head.get_ptr(&head_stamp);
			int next_stamp{};
			STNODE* old_next = old_head->next.get_ptr(&next_stamp);
			int tail_stamp{};
			STNODE* old_tail = tail.get_ptr(&tail_stamp);
			if (old_head != head.get_ptr()) continue;
			if (old_next != nullptr) return -1;
			if (old_tail == old_head) {
				tail.CAS(old_tail, old_next, tail_stamp, tail_stamp + 1);
				continue;
			}

			int value = old_next->value;
			if (true == head.CAS(old_head, old_next,head_stamp,head_stamp+1)) {
				delete old_head;
				return value;
			}
		}
	}

	void print20()
	{
		STNODE* p = head.get_ptr()->next.get_ptr();

		for (int i = 0; i < 20; ++i) {
			if (nullptr == p) break;
			std::cout << p->value << ", ";
			p = p->next.get_ptr();
		}
		std::cout << std::endl;
	}
};


constexpr int MAX_THREADS = 16;
constexpr int NUM_TEST = 10'000'000;

LFSTQUEUE g_queue;

void benchmark(const int num_thread)
{
	int key = 0;
	const int num_loop = NUM_TEST / num_thread;

	for (int i = 0; i < num_loop; i++) {
		if ((i < 32) || (rand() % 2 == 0))
			g_queue.Enq(key++);
		else
			g_queue.Deq();

	}
}

int main()
{
	using namespace std::chrono;

	{
		for (int i = 1; i <= MAX_THREADS; i = i * 2) {
			std::vector <std::thread> threads;
			g_queue.clear();
			auto start_t = system_clock::now();
			for (int j = 0; j < i; ++j)
				threads.emplace_back(benchmark, i);
			for (auto& th : threads)
				th.join();
			auto end_t = system_clock::now();
			auto exec_t = end_t - start_t;
			auto exec_ms = duration_cast<milliseconds>(exec_t).count();

			std::cout << i << " Threads, Exec time = " << exec_ms << "ms.  \n";
			std::cout << "QUEUE = ";
			g_queue.print20();
			std::cout << std::endl;
		}
	}
}