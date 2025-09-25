#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <queue>

using namespace std::chrono;

volatile int sum = 0;

void no_lock_thread(const int thread_count)
{
	for (auto i = 0; i < 5'000'000 / thread_count; ++i) sum = sum + 2;
}

std::mutex mu;

void mutex_thread(const int thread_count)
{
	for (auto i = 0; i < 5'000'000 / thread_count; ++i)
	{
		mu.lock();
		sum = sum + 2;
		mu.unlock();
	}
}

volatile bool v_flag[8] = { false, false, false , false , false , false , false , false };
volatile int v_label[8] = { 0,0,0,0,0,0,0,0 };

void volatile_bakery_lock(const int thread_count, const int thread_num)
{
	v_flag[thread_num] = true;
	int max_label = 0;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	for (int i = 0; i < thread_count; ++i)
		if (max_label < v_label[i])
			max_label = v_label[i];

	v_label[thread_num] = max_label + 1;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	for (int k = 0; k < thread_count; ++k) {
		if (k == thread_num) continue;
		while (v_flag[k] && ((v_label[k] < v_label[thread_num]) || (v_label[k] == v_label[thread_num] && k < thread_num))) {}
	}
}

void volatile_bakery_unlock(const int thread_num)
{
	v_flag[thread_num] = false;
}

void volatile_bakery_thread(const int thread_count, const int thread_num)
{
	for (auto i = 0; i < 5'000'000 / thread_count; ++i)
	{
		volatile_bakery_lock(thread_count, thread_num);
		sum = sum + 2;
		volatile_bakery_unlock(thread_num);
	}
}





std::atomic<bool> a_flag[8] = { false, false, false , false , false , false , false , false };
std::atomic<int> a_label[8] = { 0,0,0,0,0,0,0,0 };
std::atomic<int> a_sum = 0;

void atomic_bakery_lock(const int thread_count, const int thread_num)
{
	a_flag[thread_num] = true;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	int max_label = 0;
	for (int i = 0; i < thread_count; ++i)
		if (max_label < a_label[i])
			max_label = a_label[i];
	a_label[thread_num] = max_label + 1;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	for (int k = 0; k < thread_count; ++k) {
		if (k == thread_num) continue;
		while (a_flag[k] && ((a_label[k] < a_label[thread_num]) || (a_label[k] == a_label[thread_num] && k < thread_num))) {}
	}
}

void atomic_bakery_unlock(const int thread_num)
{
	std::atomic_thread_fence(std::memory_order_seq_cst);
	a_flag[thread_num] = false;
}

void atomic_bakery_thread(const int thread_count, const int thread_num)
{
	for (auto i = 0; i < 5'000'000 / thread_count; ++i)
	{
		atomic_bakery_lock(thread_count, thread_num);
		a_sum += 2;
		atomic_bakery_unlock(thread_num);
	}
}


int main()
{
	/*for (int i = 1; i <= 8; i *= 2) {
		std::vector<std::thread> threads;
		sum = 0;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j) {
			threads.emplace_back(no_lock_thread, i);
		}
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << "no lock " << i << " thread - result_sum: " << sum << ", time: " << duration_cast<milliseconds>(end_t - start_t).count() << " ms" << std::endl;
	}*/
	/*for (int i = 1; i <= 8; i *= 2) {
		std::vector<std::thread> threads;
		sum = 0;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j) {
			threads.emplace_back(mutex_thread, i);
		}
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << "mutex " << i << " thread - result_sum: " << sum << ", time: " << duration_cast<milliseconds>(end_t - start_t).count() << " ms" << std::endl;
	}*/

	/*for (int i = 1; i <= 8; i *= 2) {
		std::vector<std::thread> threads;
		sum = 0;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j) {
			threads.emplace_back(volatile_bakery_thread, i, j);
		}
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << "volatile bakery " << i << " thread - result_sum: " << sum << ", time: " << duration_cast<milliseconds>(end_t - start_t).count() << " ms" << std::endl;
		for (int k = 0; k < 8; ++k) {
			v_flag[k] = false;
			v_label[k] = 0;
		}
	}*/

	/*for (int i = 1; i <= 8; i *= 2) {
		std::vector<std::thread> threads;
		a_sum = 0;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j) {
			threads.emplace_back(atomic_bakery_thread, i, j);
		}
		for (auto& th : threads) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << "atomic bakery " << i << " thread - result_sum: " << a_sum << ", time: " << duration_cast<milliseconds>(end_t - start_t).count() << " ms" << std::endl;
		for (int k = 0; k < 8; ++k) {
			a_flag[k] = false;
			a_label[k] = 0;
		}
	}*/

}