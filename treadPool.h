#include<thread>
#include<condition_variable>
#include<mutex>
#include<vector>
#include<queue>
#include<future>

class ThreadPool
{
private:
	bool m_stop;//停止线程标识符
	std::vector<std::thread>m_thread;//线程数组
	std::queue<std::function<void()>>tasks;//任务队列
	std::mutex m_mutex;//互斥锁
	std::condition_variable m_cv;//条件变量
public:
	explicit ThreadPool(size_t threadNumber):m_stop(false)//线程池构造函数，将停止标识符初始化为false
	{
		for(size_t i = 0;i < threadNumber;++i)
		{
			m_thread.emplace_back(//在线程数组中进行尾插，使用一个lambda表达式来构建一个线程
				[this](){
					for(;;)
					{
						std::function<void()>task;//创建一个task任务对象
						{
							std::unique_lock<std::mutex>lk(m_mutex);//抢锁
							m_cv.wait(lk,[this](){return m_stop||!tasks.empty();});//等待条件变量成立，同时释放锁
							if(m_stop&&tasks.empty()) return;//当m_stop成立且任务队列为空时，返回
							task = std::move(tasks.front());//取出vector中第一个任务
							tasks.pop();//将该任务弹出
						}
						task();//执行该任务
					}
				}
					);
		}
	}
	//禁止拷贝构造和移动构造
	ThreadPool(const ThreadPool &) = delete;
	ThreadPool(ThreadPool &&) = delete;
	ThreadPool & operator=(const ThreadPool &) = delete;
	ThreadPool & operator=(ThreadPool &&) = delete;

	~ThreadPool()
	{
		{
			std::unique_lock<std::mutex>lk(m_mutex);//抢锁
			m_stop=true;//将m_stop标识符置为true
		}
		m_cv.notify_all();//使用条件变量通知所有线程，当tasks线程为空时，线程函数返回结束
		for(auto& threads:m_thread)//遍历线程池
		{
			threads.join();//回收线程资源
		}
	}

	template<typename F,typename... Args>
	auto submit(F&& f,Args&&... args)->std::future<decltype(f(args...))>//创建一个提交任务到任务队列的函数，返回值为future对象
	{
		//首先将函数f和参数包arg进行完美转发，使用bind来进行包装，其返回的为一个可调用对象
		//使用packaged_task将可调用对象包装成一个异步任务类，该任务类可以与一个future类进行关联
		//使用一个make_shared来创建一个智能指针，进而自动为对象分配内存，当对象的引用计数为0时，智能指针将会自动释放内存
		//最后的结果存入一个taskPtr对象中
		auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
				std::bind(std::forward<F>(f),std::forward<Args>(args)...)
				);
		{
			std::unique_lock<std::mutex>lk(m_mutex);//抢锁
			if(m_stop) throw std::runtime_error("submit on stopped ThreadPool");//若m_stop位置1，则抛出异常
			tasks.emplace([taskPtr](){(*taskPtr)();});//调用taskPtr中的函数，并将该函数添加到任务队列中
		}
		m_cv.notify_one();//通知一个线程来取任务
		return taskPtr->get_future();//返回一个future对象
	}

};
