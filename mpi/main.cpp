#include <string>
#include <ios>
#include <iostream>
#include <fstream>

#include <mpi.h>
#include <sys/utsname.h>

#include <boost/assert.hpp>

#include "profiler.h"

extern "C" {
	#include "gtmpi.h"
}

inline std::string get_filename_for_node(int rank)
{
	return "workspace_" + std::to_string(rank) + ".txt";
}

int main(int argc, char ** argv)
{
	MPI_Init(&argc, &argv);

	int num_processes = 1;
	MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

	int rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	gtmpi_init(num_processes);

	struct utsname ugnm;
	uname(&ugnm);

	std::string node_string = "Barrier Loop on Node #" + std::to_string(rank) + " " + ugnm.nodename;

	{
		Profiler p(node_string);

		std::string filename(get_filename_for_node(rank));
		std::fstream fs(filename, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc );


		std::fstream next_fs;
		if (rank < num_processes - 1)
		{
			std::string next_filename(get_filename_for_node(rank + 1));
			next_fs = std::fstream(filename, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc );
		}

		const unsigned kMaxIter = (1u << 22);
		for (unsigned i = 0; i < kMaxIter; ++i)
		{
			// Write file
			fs.seekg( static_cast<std::fstream::pos_type>(0) );
			fs << i;
			gtmpi_barrier();

			// Check if equal to neighbour
			if (rank < num_processes - 1)
			{
				next_fs.seekg( static_cast<std::fstream::pos_type>(0) );
				unsigned j = 0;
				next_fs >> j;
				BOOST_ASSERT_MSG(i == j, "My value doesn't equal to my neighbour after the barrier!!");
			}

			// Check file
			gtmpi_barrier();
		}
	}

	gtmpi_finalize();

	MPI_Finalize();


	return 0;
}

