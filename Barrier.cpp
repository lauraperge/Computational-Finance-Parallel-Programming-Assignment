#include <mutex>
#include <thread>
#include <iostream>
#include <condition_variable>
#include <vector>
#include <random>
#include <algorithm>
#include <deque>
#include <cmath>

using namespace std;

//Barrier class - it is a reusable barrier, i.e. can be called several times withing the code
template <class LOCKABLE>
class Barrier {
	int wait_count;
	int N_runs;
	int const target_wait_count;
	std::mutex mtx;
	std::condition_variable cond_var;
public:
	Barrier(const int& threads_to_wait_for)
		: target_wait_count(threads_to_wait_for)
	{
		wait_count = 0;
		N_runs = 0;
	}

	void wait() { 
		std::unique_lock<std::mutex> lk(mtx); //when wait called it lock the mutex
		unsigned int const current_wait_cycle = N_runs; 
		//this is good for counting how many times the Barrier has run and also to be checked at spurious wake
		++wait_count; //wait count increased
		if (wait_count != target_wait_count) {//if we have not reached the target yet
			// wait condition must not depend on wait_count
			cond_var.wait(lk,
				[this, current_wait_cycle]() {
				return N_runs != current_wait_cycle;
			}); //we are waiting, also protected against spurious wake
		}
		else {//if we reached the target
			// increasing the second counter allows waiting threads to exit
			++N_runs; //counting the number of calls
			wait_count = 0; //wait count reset to 0, so it is reusable
			cond_var.notify_all(); //notify waiting threads
		}
	}

	int runs()
	{
		return N_runs; //returns how many times the Barrier has been called and executed
	}

}; //https://stackoverflow.com/questions/24205226/how-to-implement-a-re-usable-thread-barrier-with-stdatomic

//The following class incorporates all required functions to modify the vector to be summed.
//The class ensures that none of the modifications indicate a data race
class SumSeq
{
private:
	std::mutex mu0;
	std::mutex mu1;
	std::mutex mu2;
	vector<int> VectorToSum;
	deque<int> Indices;
	deque<int> Indices2;
	int VecLen_orig;
	condition_variable condv;
public:

	SumSeq(const vector<int>&v) : VectorToSum(v) //REFERENCE
	{
		for (size_t i = 0; i < (VectorToSum.size() / 2); i++)
		{
			Indices.push_back(i);
		} //we save the indices 0 to the half vector size
		VecLen_orig = VectorToSum.size(); //We save the original vector length
		Indices2 = Indices; //additional variable for later
	}

	int getOriginalLen() //get original vectorlength
	{
		return VecLen_orig;
	}

	int getentry(int index) //get entry of the given index
	{
		return VectorToSum[index];
	}

	void setentry(int index, int newval) //set entry of a given index
	{
		std::lock_guard<mutex> lck0(mu0); //locked to avoid data race
		VectorToSum[index] = newval; //add new value
	}

	vector<int> getVec() //get the vector
	{
		return VectorToSum;
	}

	int getsize() //get the most recent size of the vector
	{
		return VectorToSum.size();
	}

	int getindex() //get an available index
	{
		int ind = Indices.front();
		return ind;
	}

	void popindex() //clear the used index from the list
	{
		std::lock_guard<mutex> lck1(mu1); //lock until pop
		Indices.pop_front();
	}

	void ResizeVec() { //When 1 round has run the vector need to be halved in size
		VectorToSum.resize(VectorToSum.size() / 2);
		Indices = Indices2; //reset to intial index
		Indices.resize(VectorToSum.size() / 2); //only the ones that required for the next round
	}
};

//This function is running the required divide-and-concquer mechanism to sum up the vector entries
void SumVec(Barrier<class LOCKABLE>& barrier, SumSeq& sumseq, int& N_th, vector<mutex>& Mu, vector<int>& vect)
{
	while (sumseq.getsize()>N_th) //as long as the vector size is bigger than the number of threads it runs
	{

		for (int j = 0; j < (sumseq.getsize() / (2 * N_th)); j++)//Each thread returns here for an equal amount of rounds (that is sufficient to calculate sums on half of the indices) 
		{
			std::lock_guard<mutex> lckf(Mu[0]);//lock the mutex to set a new value for an entry
			sumseq.setentry(sumseq.getindex(), sumseq.getentry(sumseq.getindex()) + sumseq.getentry(sumseq.getindex() + sumseq.getsize() / 2));//setting entry
			sumseq.popindex(); //after the entry on a given index is set, we get rid of the index that has already been used
		}
		barrier.wait(); //waiting for all threads to join after the round

		std::lock_guard<mutex> lckf2(Mu[1]); //lock mutex to halve the vector, then it runs again on the indices on the half of the new vector.
		if (sumseq.getsize() != (sumseq.getOriginalLen() / pow(2, barrier.runs())))
		{
			sumseq.ResizeVec();
		}
	}
	std::lock_guard<mutex> lckf2(Mu[2]); //when there are as many entries elft as many threads we have
	if (vect.size() != N_th) {
		vect.clear();
		vect = sumseq.getVec(); //we reset the vector in the main thread to the one from the class (this has now only N_th entries)
	}
}

bool IsPowerOfTwo(int x) //to check whether the number of vector entries is the power of two
{
	return (x != 0) && ((x & (x - 1)) == 0);
}

int main() {

	int N = 1048576; //length of vector

	vector<int> vt(N); 
	srand(134);
	for (int i = 0; i < N; i++)
	{
		vt[i] = rand() % 100; //random integers to 100
	}

	//This is only to check the results
	int sum = 0;
	for (int i = 0; i < N; i++)
	{
		sum = sum + vt[i];
	}
	cout << "Real sum: " << sum << endl;
	//Check end.

	if (IsPowerOfTwo(N))//If it is a power of 2
	{
		if (vt.size() > 8) { //If the vector size is big enough so the multithreading makes sense
			int N_th = 8; //I use 8 threads
			vector<thread> threadvec(N_th); //vector for threads
			
			Barrier<class LOCKABLE> mainbarrier(N_th);  //the Barrier class with the count set to the number of threads
			SumSeq SumUpVec(vt); //This class gets the vector to be summed as input
			vector<mutex> MU1(3); //mutexes required within the sum function

			for (int i = 0; i < N_th; i++)//we call each thread
			{
				threadvec[i] = thread(SumVec, std::ref(mainbarrier),
					std::ref(SumUpVec), std::ref(N_th), std::ref(MU1), std::ref(vt)); //calling the SumVec function
			}
			for (size_t it = 0; it < threadvec.size(); it++)//waiting for all threads to join
			{
				threadvec[it].join();
			}

			//sum the remaining 8 entries
			int SumNew = 0;
			for (int j = 0; j < N_th; j++)
			{
				SumNew += vt[j];
			}
			cout << "The sum of the vector entries calculated by divide-and-conquer algorithm with threads: " << SumNew << endl;
		}
		else {

			cout << "Vector is small, no threads used, the sum is: " << sum << endl;
		}

	}
	else {
		cout << "The sum of the vector entries cannot be calculated by divide-and-conquer algorithm"
			<< " since the number of entries is not the power of two." << endl;
	}

	return 0;
}