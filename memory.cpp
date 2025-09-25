#include <iostream>
#include <thread>

const int LOOP_COUNT = 50'000'000;

volatile int x, y;
int tracex[LOOP_COUNT], tracey[LOOP_COUNT];

void trace_x_thread() {
	for (int i = 0;i < LOOP_COUNT;++i) {
		x = i;
		tracex[i] = y;
	}
}
void trace_y_thread() {
	for (int i = 0;i < LOOP_COUNT;++i) {
		y = i;
		tracey[i] = x;
	}
}


int main()
{
	std::thread th1(trace_x_thread);
	std::thread th2(trace_y_thread);
	th1.join();
	th2.join();

	int cnt{};
	for (int i = 0;i < LOOP_COUNT-1;++i) {
		if(tracex[i] == tracex[i+1])
			if (tracey[tracex[i]] == tracey[tracex[i + 1]]){
				if (tracey[tracex[i]] != i) continue;
					cnt++;

	}
	std::cout << "Total Memory Inconsistency: " << cnt << std::endl;

}