
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <vector>

#include "my_utils.h"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>


extern "C" {
    #include "gtmpi.h"
}

/*
        From the MCS Paper: A sense-reversing centralized barrier

        shared count : integer := P
        shared sense : Boolean := true
        processor private local_sense : Boolean := true

        procedure central_barrier
                local_sense := not local_sense // each processor toggles its own sense
        if fetch_and_decrement (&count) = 1
                count := P
                sense := local_sense // last processor toggles global sense
                else
                     repeat until sense = local_sense
*/


static unsigned s_world_size = kUnsignedInvalid;
//static std::vector<MPI_Request> s_reqs;


// zxing7: Also save the dynamic allocation of the status array that's completely not used.

void gtmpi_init(int num_threads)
{
    s_world_size = boost::numeric_cast<unsigned>(num_threads);
    //s_reqs.resize(s_world_size);
}

void gtmpi_barrier()
{
    unsigned rank = get_rank();
    BOOST_ASSERT(rank < s_world_size);

    constexpr int kDefaultTag = 0;

    // zxing7:
    // Instead every node sending to every other node which is O(n^2),
    // we can make it O(n), as follow:

    // If not first node, wait for previous node to arrive
    if (rank != 0)
    {
        MPI_Recv(nullptr, 0, MPI_UNSIGNED_CHAR, rank - 1, kDefaultTag, MPI_COMM_WORLD, nullptr);
    }
    // If not last node, arrive at next node
    if (rank != s_world_size - 1)
    {
        MPI_Send(nullptr, 0, MPI_UNSIGNED_CHAR, rank + 1, kDefaultTag, MPI_COMM_WORLD);
    }

    // If not first node, wake up previous node
    if (rank != 0)
    {
        MPI_Send(nullptr, 0, MPI_UNSIGNED_CHAR, rank - 1, kDefaultTag, MPI_COMM_WORLD);
    }

    // If not last node, wait for being waken up by next node
    if (rank != s_world_size - 1)
    {
        MPI_Recv(nullptr, 0, MPI_UNSIGNED_CHAR, rank + 1, kDefaultTag, MPI_COMM_WORLD, nullptr);
    }


    // if (rank == 0)
    // {
    //     for (unsigned i = 1; i < s_world_size; ++i)
    //     {
    //         int result = MPI_Recv(nullptr, 0, MPI_UNSIGNED_CHAR,
    //             boost::numeric_cast<int>(i), kDefaultTag, MPI_COMM_WORLD, nullptr);
    //         BOOST_ASSERT(result == MPI_SUCCESS);
    //     }
    //     // Wake everyone up
    //     for (unsigned i = 1; i < s_world_size; ++i)
    //     {
    //         int result = MPI_Send(nullptr, 0, MPI_UNSIGNED_CHAR,
    //             boost::numeric_cast<int>(i), kDefaultTag, MPI_COMM_WORLD);
    //         BOOST_ASSERT(result == MPI_SUCCESS);
    //     }
    // }
    // else
    // {
    //     MPI_Send(nullptr, 0, MPI_UNSIGNED_CHAR, 0, kDefaultTag, MPI_COMM_WORLD);
    //     MPI_Recv(nullptr, 0, MPI_UNSIGNED_CHAR, 0, kDefaultTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    // }
}

void gtmpi_finalize()
{

}

