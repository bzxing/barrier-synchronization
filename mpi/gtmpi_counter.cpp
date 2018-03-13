
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

// zxing7: Also save the dynamic allocation of the status array that's completely not used.

void gtmpi_init(int num_threads)
{
    s_world_size = boost::numeric_cast<unsigned>(num_threads);
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
        MPI_Recv(nullptr, 0, MPI_INT, rank - 1, kDefaultTag, MPI_COMM_WORLD, nullptr);
    }
    // If not last node, arrive at next node
    if (rank != s_world_size - 1)
    {
        MPI_Send(nullptr, 0, MPI_INT, rank + 1, kDefaultTag, MPI_COMM_WORLD);
    }

    // If not first node, wake up previous node
    if (rank != 0)
    {
        MPI_Send(nullptr, 0, MPI_INT, rank - 1, kDefaultTag, MPI_COMM_WORLD);
    }

    // If not last node, wait for being waken up by next node
    if (rank != s_world_size - 1)
    {
        MPI_Recv(nullptr, 0, MPI_INT, rank + 1, kDefaultTag, MPI_COMM_WORLD, nullptr);
    }

    // Although on only 8 logical cores on my own machine, the absolute difference
    // in elapsed time vs the original implementation is tiny, but the time growth
    // from 2-core to 8-core is noticeably less than the original.
    //
    // Following the trend, this O(n) solution should be more scalable on massively
    // parallel system. The reason being less usage of the
    // network bandwidth.
    //
    // a potential weakness of this design might be the latency, which scales linearly
    // with the number of nodes but with quite big constant (at least need to
    // go around each node twice for barrier completion, serially!!) However this should not be
    // much worse than the original counter implementation, which will also need to send N
    // and receive N messages per node. The receiving part might be able to use some parallelism,
    // in the original, but not with my new implementation. I doubt there's much performance loss.
}

void gtmpi_finalize()
{
    s_world_size = kUnsignedInvalid;
}

