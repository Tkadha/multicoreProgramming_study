#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <atomic>

volatile int sum{ 0 };
std::mutex mu;

const int MAX_THREADS = 16;
const int CACHE_LINE_SIZE_INT = 16;


volatile int arr_sum[MAX_THREADS] = { 0 };
volatile int bit_arr_sum[MAX_THREADS * CACHE_LINE_SIZE_INT] = { 0 };

struct NUM {
	alignas(64) volatile int sum;
};
NUM cache_arr_sum[MAX_THREADS] = { 0 };


void func(const int loop_count)
{
	volatile int local_sum = 0;
	for (int i = 0;i < loop_count;++i) local_sum = local_sum + 2;
	mu.lock();
	sum += local_sum;
	mu.unlock();
}

void arr_func(const int th_id, const int loop_count)
{
	for (int i = 0;i < loop_count; ++i) 
		arr_sum[th_id] = arr_sum[th_id] + 2;
}

void cache_arr_func(const int th_id, const int loop_count)
{
	for (int i = 0;i < loop_count; ++i)
		bit_arr_sum[th_id * CACHE_LINE_SIZE_INT] = bit_arr_sum[th_id * CACHE_LINE_SIZE_INT] + 2;
}

void alignas_arr_func(const int th_id, const int loop_count)
{
	for (int i = 0;i < loop_count; ++i)
		cache_arr_sum[th_id ].sum = cache_arr_sum[th_id].sum + 2;
}

int main()
{
	using namespace std::chrono;

	// single thread
	{
		auto start = high_resolution_clock::now();
		for (auto i = 0;i < 50'000'000;++i)	sum = sum + 2;
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << "Single Thread: " << duration.count() << "ms ";
		std::cout << "Sum = " << sum << std::endl;
	}

	/*for(int num_threads = 1;num_threads <= 16;num_threads*=2) {
		sum = 0;
		auto start = high_resolution_clock::now();
		std::vector<std::thread> threads;
		for (int i = 0;i < num_threads;++i) {
			threads.emplace_back(func, 50'000'000 / num_threads);
		}
		for (auto& t : threads) t.join();
		auto stop = high_resolution_clock::now();

		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << num_threads <<" Thread: " << duration.count() << "ms ";
		std::cout << "Sum = " << sum << std::endl;
	}*/

	/*for (int num_threads = 1;num_threads <= 16;num_threads *= 2) {
		sum = 0;
		auto start = high_resolution_clock::now();
		std::vector<std::thread> threads;
		for (int i = 0;i < num_threads;++i) {
			threads.emplace_back(arr_func, i , 50'000'000 / num_threads);
		}
		for (int i = 0;i < num_threads;++i) {
			threads[i].join();
			sum = sum + arr_sum[i];
			arr_sum[i]= 0;
		}
		auto stop = high_resolution_clock::now();

		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << num_threads << " Array Thread: " << duration.count() << "ms ";
		std::cout << "Sum = " << sum << std::endl;
	}*/

	/*for (int num_threads = 1;num_threads <= 16;num_threads *= 2) {
		sum = 0;
		auto start = high_resolution_clock::now();
		std::vector<std::thread> threads;
		for (int i = 0;i < num_threads;++i) {
			threads.emplace_back(cache_arr_func, i, 50'000'000 / num_threads);
		}
		for (int i = 0;i < num_threads;++i) {
			threads[i].join();
			sum = sum + bit_arr_sum[i * CACHE_LINE_SIZE_INT];
			bit_arr_sum[i * CACHE_LINE_SIZE_INT] = 0;
		}
		auto stop = high_resolution_clock::now();

		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << num_threads << " Cache Line Array Thread: " << duration.count() << "ms ";
		std::cout << "Sum = " << sum << std::endl;
	}*/

	for (int num_threads = 1;num_threads <= 16;num_threads *= 2) {
		sum = 0;
		auto start = high_resolution_clock::now();
		std::vector<std::thread> threads;
		for (int i = 0;i < num_threads;++i) {
			threads.emplace_back(alignas_arr_func, i, 50'000'000 / num_threads);
		}
		for (int i = 0;i < num_threads;++i) {
			threads[i].join();
			sum = sum + cache_arr_sum[i].sum;
			cache_arr_sum[i].sum = 0;
		}
		auto stop = high_resolution_clock::now();

		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << num_threads << " Alignas Cache Array Thread: " << duration.count() << "ms ";
		std::cout << "Sum = " << sum << std::endl;
	}
}
