#include <limits>

#include <mpi.h>

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>


constexpr unsigned char kUnsignedCharInvalid = std::numeric_limits<unsigned char>::max();
constexpr unsigned kUnsignedInvalid = std::numeric_limits<unsigned>::max();
constexpr int kIntInvalid = std::numeric_limits<int>::min();

inline unsigned get_rank()
{
	int rank = kIntInvalid;
	int result = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	BOOST_ASSERT(result == MPI_SUCCESS);

	return boost::numeric_cast<unsigned>(rank);
}

inline unsigned get_world_size()
{
	int world_size = kIntInvalid;
	int result = MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	BOOST_ASSERT(result == MPI_SUCCESS);

	return boost::numeric_cast<unsigned>(world_size);
}
