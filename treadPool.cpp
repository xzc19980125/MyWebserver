#include<iostream>
#include<vector>
#include<functional>
#include<future>
#include"treadPool.h"
#include<random>

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000,1000);
auto rnd = std::bind(dist,mt);

void simulate_hard_computation()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(2000+rnd()));
}

void multiply(const int a,const int b)
{
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << "*" << b << "=" << res << std::endl;
}

int main(void)
{
	ThreadPool pool(4);

	for(int i=0;i<8;i++)
	{
		pool.submit(multiply,i,i+1);
	}
	return 0;
}
