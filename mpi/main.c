#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>
#include <sys/utsname.h>


#include "gtmpi.h"

// This is only to provide an example how to link C object to GTMP library
int main1(int argc, char ** argv)
{
	MPI_Init(&argc, &argv);

	int num_processes = 1;
	MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

	gtmpi_init(num_processes);

	int my_id;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

	struct utsname ugnm;
	uname(&ugnm);

	{
		printf("Start thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);

		const unsigned kMaxIter = (1u << 22);
		for (unsigned i = 0; i < kMaxIter; ++i)
		{
			gtmpi_barrier();
		}

		printf("Finish thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);
	}

	gtmpi_finalize();

	MPI_Finalize();


	return 0;
}

