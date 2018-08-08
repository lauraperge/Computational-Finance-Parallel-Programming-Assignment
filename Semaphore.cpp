#include <mutex>
#include <iostream>
#include <thread>
#include <condition_variable>
#include<vector>
#include<random>
using namespace std;

template <class LOCKABLE>
class Semaphore
{
public:

	Semaphore(const size_t count_) : count(count_)
	{}

	void notify() //want to give back a copy
	{
		unique_lock<mutex> lck(mtx); //wait until the librarian has time for us
		count++; // return copy
		cout << "One copy is given back by Visitor(id: " << this_thread::get_id() << "), (" << count << " copies available in the library)" << endl; //info
		cv.notify_one(); // 1 copy returned, can be taken by someone!
	}

	void wait() //want to borrow a copy
	{
		unique_lock<mutex> lck(mtx); //everyone is waiting until someone is in the borrowing process
									 //as long as the count is not positive, i.e. there is no copy in the library everyone waits for the notify
		cv.wait(lck, [this]() { return count > 0; });
		count--; //get in, take a copy
		cout << "One copy is borrowed by Visitor(id: " << this_thread::get_id() << "), (" << count << " copies left)" << endl; //info
	}

	void lock() { wait(); }
	void unlock() { notify(); }
private:

	mutex mtx;
	condition_variable cv;
	int count;
};


void ThreadFunc(Semaphore<class LOCKABLE>& Visitor) {
	Visitor.lock(); //lock semaphore - wait for copy
	this_thread::sleep_for(chrono::microseconds(rand() % 5)); //Reading for random time
	//this_thread::sleep_for(chrono::microseconds(5)); //Reading for given fix time
	Visitor.unlock(); //unlock semaphore - give back copy
};

int main() {
	size_t nocopies = 5; //5 copies
	Semaphore<class LOCKABLE> Counter(nocopies);
	const size_t N = 50; // It is more obvious how it works with less threads..
	vector<thread> vt(N);

	const int M = 1;
	int j = 0;
	//const int M = 1000; //if we want to check whether it crashes somewhere - CAREFUL: might run long
	while (j<M) // just for checking purposes
	{
		//Simulation start
		cout << "The library has " << nocopies << " copies of the book." << endl;
		for (size_t i = 0; i < N; i++)
		{
			vt[i] = thread(ThreadFunc, ref(Counter)); //calling threads
		}

		for (size_t i = 0; i < N; i++)
		{
			vt[i].join();
		}

		cout << "Simulation " << j << " has finished." << endl; //After everyone has read the book once.
		j++;
	}
	return 0;
}