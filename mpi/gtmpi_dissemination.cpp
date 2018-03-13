

#include <thread>

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <mpi.h>

#include "my_utils.h"

extern "C" {
#include "gtmpi.h"
}

/*
    From the MCS Paper: The scalable, distributed dissemination barrier with only local spinning.

    type flags = record
        myflags : array [0..1] of array [0..LogP - 1] of Boolean
	partnerflags : array [0..1] of array [0..LogP - 1] of ^Boolean

    processor private parity : integer := 0
    processor private sense : Boolean := true
    processor private localflags : ^flags

    shared allnodes : array [0..P-1] of flags
        //allnodes[i] is allocated in shared memory
	//locally accessible to processor i

    //on processor i, localflags points to allnodes[i]
    //initially allnodes[i].myflags[r][k] is false for all i, r, k
    //if j = (i+2^k) mod P, then for r = 0 , 1:
    //    allnodes[i].partnerflags[r][k] points to allnodes[j].myflags[r][k]

    procedure dissemination_barrier
        for instance : integer :0 to LogP-1
	    localflags^.partnerflags[parity][instance]^ := sense
	    repeat until localflags^.myflags[parity][instance] = sense
	if parity = 1
	    sense := not sense
	parity := 1 - parity
*/

class DisseminationBarrier
{
public:
    DisseminationBarrier() = default;

    DisseminationBarrier(unsigned world_size) :
        m_world_size(world_size)
    {
        BOOST_ASSERT(world_size > 0);
    }

    void barrier()
    {
        BOOST_ASSERT( m_world_size == get_world_size() );

        m_rank = get_rank();
        BOOST_ASSERT(m_rank < m_world_size);

        unsigned distance = 1;
        constexpr int kDefaultTag = 0;

        while (distance < m_world_size)
        {
            // Notify next, blocking
            {
                unsigned next_rank = (m_rank + distance) % m_world_size;
                int result = MPI_Send(nullptr, 0, MPI_UNSIGNED_CHAR,
                    boost::numeric_cast<int>(next_rank),
                    kDefaultTag, MPI_COMM_WORLD);
                BOOST_ASSERT(result == MPI_SUCCESS);
            }

            // Wait on prev, blocking
            {
                unsigned prev_rank = (m_rank + m_world_size - distance) % m_world_size;
                int result = MPI_Recv(nullptr, 0, MPI_UNSIGNED_CHAR,
                    boost::numeric_cast<int>(prev_rank),
                    kDefaultTag, MPI_COMM_WORLD, nullptr);
                BOOST_ASSERT(result == MPI_SUCCESS);
            };


            distance *= 2;
        }
    }

private:


    unsigned m_world_size = kUnsignedInvalid;
    unsigned m_rank = kUnsignedInvalid;
};

static DisseminationBarrier s_barrier;


void gtmpi_init(int num_threads)
{
    s_barrier = DisseminationBarrier( boost::numeric_cast<unsigned>(num_threads) );
}

void gtmpi_barrier()
{
    s_barrier.barrier();
}

void gtmpi_finalize()
{

}
