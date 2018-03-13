#include <string>
#include <ios>
#include <iostream>
#include <fstream>

#include <mpi.h>
#include <sys/utsname.h>

#include <boost/assert.hpp>

#include "profiler.h"
#include "my_utils.h"

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
	int result = MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
	BOOST_ASSERT(result == MPI_SUCCESS);

	int rank = 0;
	result = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	BOOST_ASSERT(result == MPI_SUCCESS);

	gtmpi_init(num_processes);

	struct utsname ugnm;
	uname(&ugnm);

	std::string node_string = "Barrier Loop on Node #" + std::to_string(rank) + " " + ugnm.nodename;

	{
		Profiler p(node_string);

		std::string filename(get_filename_for_node(rank));
		std::ofstream ofs(filename);

		std::ifstream ifs;
		if (rank < num_processes - 1)
		{
			std::string next_filename(get_filename_for_node(rank + 1));
			ifs = std::ifstream(next_filename);
		}

		const unsigned kMaxIter = (1u << 16);
		for (unsigned i = 0; i < kMaxIter; ++i)
		{
			// Write file
			ofs.seekp( static_cast<std::ofstream::pos_type>(0) );
			ofs << i << std::flush;
			gtmpi_barrier();

			// Check if equal to neighbour
			if (rank < num_processes - 1)
			{
				ifs.seekg( static_cast<std::ifstream::pos_type>(0) );
				unsigned j = kUnsignedInvalid;
				ifs >> j;

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

