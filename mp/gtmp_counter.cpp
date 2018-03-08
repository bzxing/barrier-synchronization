#include <atomic>
#include <memory>
#include <type_traits>

#include <omp.h>

#include <boost/assert.hpp>

#include "gtmp.h"


class CounterBarrier
{
public:

    CounterBarrier(int num_threads = 1) :
        m_num_threads(num_threads),
        m_count(num_threads),
        m_sense(false)
    {
        BOOST_ASSERT(num_threads > 0);
    }

    void barrier()
    {
        thread_local bool local_sense = false;
        local_sense = !local_sense;

        int prev = m_count.fetch_sub(1);

        if (prev == 1)
        {
            m_count.store(m_num_threads);
            m_sense.store(local_sense);
        }
        else
        {
            while (local_sense != m_sense.load());
        }
    }

private:
    int m_num_threads;
    std::atomic<int> m_count;
    std::atomic<bool> m_sense;

};


static CounterBarrier s_instance;

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


void gtmp_init(int num_threads)
{
    // Hacky...
    s_instance.~CounterBarrier();
    new(&s_instance) CounterBarrier(num_threads);
}

void gtmp_barrier()
{
    s_instance.barrier();
}

void gtmp_finalize()
{

}
