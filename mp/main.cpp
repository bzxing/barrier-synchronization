
#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/assert.hpp>

#include <omp.h>

#include "profiler.h"

extern "C" {
  #include "gtmp.h"
}

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

class alignas(LEVEL1_DCACHE_LINESIZE) MyInt
{
public:
	MyInt(int val = 0) :
		m_val(val)
	{}

	MyInt(MyInt && other) = default;

	operator int()
	{
		return m_val;
	}

	MyInt & operator++()
	{
		++m_val;
		return *this;
	}

	friend bool operator==(const MyInt & a, const MyInt & b)
	{
		return a.m_val == b.m_val;
	}

private:
	int m_val;
};


inline bool is_power_of_2(unsigned x)
{
	return ((x != 0) && ((x & (~x + 1)) == x));
}

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
		std::vector<MyInt> workspace(num_threads);

		constexpr unsigned kMaxIters = 1 << 22;
		for (unsigned i = 0; i < kMaxIters; ++i)
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

			if (is_power_of_2(i))
			{
				std::cout << "." << std::flush;
			}
		}
		std::cout << std::endl;
	}


	gtmp_finalize();

	return 0;
}


