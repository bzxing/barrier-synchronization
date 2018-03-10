
#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/assert.hpp>

#include <omp.h>

#include "profiler.h"
#include "gtmp.h"

class ArgParse
{
public:
	ArgParse(int argc, char ** argv)
	{
		if (argc >= 2)
		{
			std::string str(argv[1]);
			m_num_threads = std::stoi(str);
			BOOST_ASSERT(m_num_threads >= 1);
		}
		else
		{
			m_num_threads = std::thread::hardware_concurrency();
		}

		std::cout << "Number of threads is " + std::to_string(m_num_threads) + "\n";
	}

	int get_num_threads() const
	{
		return m_num_threads;
	}

private:
	int m_num_threads = 1;
};

class HeapInt
{
public:
	HeapInt() :
		m_ptr(std::make_unique<int>(0))
	{

	}

	HeapInt(int val) :
		m_ptr(std::make_unique<int>(0))
	{

	}

	HeapInt(HeapInt && other) = default;

	operator int()
	{
		return *m_ptr;
	}

	HeapInt & operator++()
	{
		++(*m_ptr);
		return *this;
	}

	friend bool operator==(const HeapInt & a, const HeapInt & b)
	{
		return *(a.m_ptr) == *(b.m_ptr);
	}

private:
	std::unique_ptr<int> m_ptr;
};

int main(int argc, char ** argv)
{
	// Get num threads
	const ArgParse args(argc, argv);
	const int num_threads = args.get_num_threads();

	// Disable dynamic threading
	omp_set_dynamic(0);
	BOOST_ASSERT(omp_get_dynamic() == 0);

	omp_set_num_threads(num_threads);


	gtmp_init(num_threads);

	{
		Profiler p("Parallel Section");
		std::vector<HeapInt> workspace(num_threads);

		constexpr int kMaxIters = 1 << 21;
		for (int i = 0; i < kMaxIters; ++i)
		{
			#pragma omp parallel
			{
				const int thread_id = omp_get_thread_num();
				++(workspace[thread_id]);

				gtmp_barrier();

				// After barrier, every thread's value should be equal to its neighbour.
				if (thread_id < (num_threads - 1))
				{
					BOOST_ASSERT(workspace[thread_id] == workspace[thread_id + 1]);
				}
			} // End paralle section
		}
	}

	gtmp_finalize();
}
