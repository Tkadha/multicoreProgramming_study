#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <numeric>

using namespace std::chrono;
const int MAX_THREADS = 32;

class NODE {
public:
	int value;
	NODE* next;
	std::mutex mu;
	NODE() :next(nullptr) {}
	NODE(int x) :next(nullptr), value(x) {}
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
		std::cout <<std::endl;
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



F_SET set;
void benchmark(const int num_thread)
{
	const int loop_count = 4'000'000 / num_thread;
	const int range = 1000;
	for (int i = 0;i < loop_count;++i) {
		int value = rand() % range;
		int op = rand() % 3;
		if (op == 0) set.add(value);
		else if (op == 1) set.remove(value);
		else set.contains(value);
	}
}

int main()
{
	for (int i = MAX_THREADS; i >= 1; i /= 2) {
		set.clear();
		std::vector<std::thread> threads;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j) {
			threads.emplace_back(benchmark, i);
		}
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << "Thread " << i << ", time: " << duration_cast<milliseconds>(end_t - start_t).count() << " ms" << std::endl;
		std::cout << "Set: ";
		set.print20();
	}
}