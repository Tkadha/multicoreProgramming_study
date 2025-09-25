#include <iostream>
#include <Thread>

volatile bool done = false;
std::atomic<int> * bound = nullptr;
void ThreadFunc1()
{
	for (int j = 0; j <= 25000000; ++j) *bound = -(1 + *bound);
	done = true;
}
void ThreadFunc2()
{
	int error_cnt{};
	while (!done) {
		int v = *bound;
		if ((v != 0) && (v != -1)) 
			error_cnt++;
	}
	std::cout << "error count: " << error_cnt << std::endl;
}

int main()
{
	int num[32]{ 0, };
	long long addr = reinterpret_cast<long long> (&num[31]);
	addr = addr - (addr % 64);
	addr = addr - 2;
	bound = reinterpret_cast<std::atomic<int>*>(addr);

	std::thread th1(ThreadFunc2); // 관측먼저
	std::thread th2(ThreadFunc1);
	th1.join();
	th2.join();

}