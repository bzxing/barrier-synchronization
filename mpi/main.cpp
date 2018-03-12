#include <string>
#include <iostream>

#include <mpi.h>
#include <sys/utsname.h>

#include "profiler.h"

extern "C" {
	#include "gtmpi.h"
}

int main(int argc, char ** argv)
{
	MPI_Init(&argc, &argv);

	int num_processes = 1;
	MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

	gtmpi_init(num_processes);

	int my_id;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

	struct utsname ugnm;
	uname(&ugnm);

	std::string node_string = "Barrier Loop on Node #" + std::to_string(my_id) + " " + ugnm.nodename;

	{
		Profiler p(node_string);

		const unsigned kMaxIter = (1u << 22);
		for (unsigned i = 0; i < kMaxIter; ++i)
		{
			gtmpi_barrier();
		}
	}

	gtmpi_finalize();

	MPI_Finalize();


	return 0;
}

