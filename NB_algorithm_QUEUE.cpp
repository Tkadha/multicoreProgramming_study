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

class CQUEUE {
	NODE* head, * tail;
	std::mutex	mu;

public:
	CQUEUE()
	{
		head = tail = new NODE{ 0 };
	}
	~CQUEUE() {
		clear();
		delete head;
	}

	void clear()
	{
		NODE* curr = head->next;
		while (curr != tail) {
			auto ptr = curr->next;
			delete curr;
			curr = ptr;
		}
		delete tail;
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
	NODE* head, * tail;
public:
	LFQUEUE()
	{
		head = tail = new NODE{ 0 };
	}
	~LFQUEUE() {
		clear();
		delete head;
	}

	void clear()
	{
		NODE* curr = head->next;
		while (curr != tail) {
			auto ptr = curr->next;
			delete curr;
			curr = ptr;
		}
		delete tail;
		tail = head;
		head->next = nullptr;
	}

	bool CAS(NODE** addr, NODE* expected, NODE* new_value) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<NODE*>*>(addr),
			&expected, new_value);
	}

	void Enq(int x)
	{
		NODE* n = new NODE(x);
		while (true) {
			NODE* old_tail = tail;
			NODE* old_next = old_tail->next;
			if (old_tail != tail) continue;
			if (old_next != nullptr) {
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


constexpr int MAX_THREADS = 16;
constexpr int NUM_TEST = 4'000'000;

CQUEUE g_queue;

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

