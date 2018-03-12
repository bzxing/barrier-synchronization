#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <sys/sysinfo.h>

#include "omp.h"
#include "gtmp.h"

inline bool is_power_of_2(unsigned x)
{
	return ((x != 0) && ((x & (~x + 1)) == x));
}


double get_time_diff(const struct timespec * begin, const struct timespec * end)
{
	long ns_diff;
	long nanoseconds_in_second = 1000000000;
	bool borrow = false;

	if (end->tv_nsec >= begin->tv_nsec)
	{
		ns_diff = end->tv_nsec - begin->tv_nsec;
	}
	else
	{
		borrow = true;
		long sum = end->tv_nsec + nanoseconds_in_second;
		assert(sum >= begin->tv_nsec);
		ns_diff = sum - begin->tv_nsec;
	}

	long seconds;
	assert(end->tv_sec >= begin->tv_sec);
	seconds = end->tv_sec - begin->tv_sec;

	if (borrow)
	{
		assert(seconds > 0);
		--seconds;
	}

	return (double)seconds + ( (double)ns_diff / (double)nanoseconds_in_second );
}

int main_c(int argc, char ** argv)
{
	int num_threads = get_nprocs();

	if (argc >= 2)
	{
		num_threads = atoi(argv[1]);
		assert(num_threads >= 1);
	}

	printf("Will be running on %d threads.\n", num_threads);

	// Disable dynamic threading
	omp_set_dynamic(0);
	assert(omp_get_dynamic() == 0);

	omp_set_num_threads(num_threads);

	gtmp_init(num_threads);

	{
		int * workspace = calloc((size_t)num_threads, sizeof(int));

		unsigned kMaxIters = 1 << 22;

		struct timespec start_time;
		int time_result_1 = clock_gettime(CLOCK_MONOTONIC, &start_time);
		assert(time_result_1 == 0);
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
					assert(workspace[thread_id] == workspace[thread_id + 1]);
				}
			} // End paralle section

			if (is_power_of_2(i))
			{
				printf(".");
				fflush(stdout);
			}
		}
		struct timespec end_time;
		int time_result_2 = clock_gettime(CLOCK_MONOTONIC, &end_time);
		assert(time_result_2 == 0);

		double time_diff = get_time_diff(&start_time, &end_time);

		printf("\n");
		fflush(stdout);

		free(workspace);

		printf("Elapsed time: %f seconds\n", time_diff);
	}

	gtmp_finalize();

	return 0;
}
