
#include <mpi.h>

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <limits>

extern "C" {
	#include "gtmpi.h"
}

/*
    From the MCS Paper: A scalable, distributed tournament barrier with only local spinning

    type round_t = record
        role : (winner, loser, bye, champion, dropout)
	opponent : ^Boolean
	flag : Boolean
    shared rounds : array [0..P-1][0..LogP] of round_t
        // row vpid of rounds is allocated in shared memory
	// locally accessible to processor vpid

    processor private sense : Boolean := true
    processor private vpid : integer // a unique virtual processor index

    //initially
    //    rounds[i][k].flag = false for all i,k
    //rounds[i][k].role =
    //    winner if k > 0, i mod 2^k = 0, i + 2^(k-1) < P , and 2^k < P
    //    bye if k > 0, i mode 2^k = 0, and i + 2^(k-1) >= P
    //    loser if k > 0 and i mode 2^k = 2^(k-1)
    //    champion if k > 0, i = 0, and 2^k >= P
    //    dropout if k = 0
    //    unused otherwise; value immaterial
    //rounds[i][k].opponent points to
    //    round[i-2^(k-1)][k].flag if rounds[i][k].role = loser
    //    round[i+2^(k-1)][k].flag if rounds[i][k].role = winner or champion
    //    unused otherwise; value immaterial
    procedure tournament_barrier
        round : integer := 1
	loop   //arrival
	    case rounds[vpid][round].role of
	        loser:
	            rounds[vpid][round].opponent^ :=  sense
		    repeat until rounds[vpid][round].flag = sense
		    exit loop
   	        winner:
	            repeat until rounds[vpid][round].flag = sense
		bye:  //do nothing
		champion:
	            repeat until rounds[vpid][round].flag = sense
		    rounds[vpid][round].opponent^ := sense
		    exit loop
		dropout: // impossible
	    round := round + 1
	loop  // wakeup
	    round := round - 1
	    case rounds[vpid][round].role of
	        loser: // impossible
		winner:
		    rounds[vpid[round].opponent^ := sense
		bye: // do nothing
		champion: // impossible
		dropout:
		    exit loop
	sense := not sense
*/

constexpr unsigned char kUnsignedCharInvalid = std::numeric_limits<unsigned char>::max();
constexpr unsigned kUnsignedInvalid = std::numeric_limits<unsigned>::max();
constexpr int kIntInvalid = std::numeric_limits<int>::min();

class TournamentBarrier
{
public:
	struct InitPlaceholder {};

	TournamentBarrier() = default;

	TournamentBarrier(int num_threads)
	{
		BOOST_ASSERT(num_threads > 0);
		m_world_size = boost::numeric_cast<unsigned>(num_threads);
	}

	void barrier()
	{
		m_rank = get_rank();
		BOOST_ASSERT(m_world_size == get_world_size());

		unsigned round_opponent_distance = 1;

		// Loop toward championship
		//
		// When round_opponent_distance >= m_world_size, it means #0 does not
		// have opponent, hence competition stops
		bool is_winner = false;
		while ( round_opponent_distance < m_world_size )
		{
			is_winner = (m_rank % (round_opponent_distance * 2) == 0);
			if (!is_winner)
			{
				BOOST_ASSERT(m_rank >= round_opponent_distance);
				unsigned opponent = m_rank - round_opponent_distance;
				arrival_notify_winner(opponent);
				wakeup_wait_for_winner(opponent);
				break;
			}

			// This node is winner!
			unsigned opponent = m_rank + round_opponent_distance;

			// If opponent is out of range, current node automatically
			// advances into next round.
			//
			// Else, wait on loser to notify
			if (opponent < m_world_size)
			{
				arrival_wait_for_loser(opponent);
			}

			round_opponent_distance *= 2;
		}

		// Up until this point, this node has either lost to someone and been waken up by the winner,
		// or won the championship.

		// Loop to wakeup everyone lost to me
		//
		// Keep halving round_opponent_distance until it goes to 0 (it should be 1 during the final round)
		while (round_opponent_distance > 0)
		{
			unsigned loser = m_rank + round_opponent_distance;

			if (loser < m_world_size)
			{
				wakeup_loser(loser);
			}

			round_opponent_distance /= 2;
		}
	}

private:

	constexpr static unsigned char kMsgLoserArrival = 1;
	constexpr static unsigned char kMsgWinnerWakeup = 2;

	void arrival_notify_winner(unsigned opponent)
	{
		BOOST_ASSERT(opponent < m_world_size);
		unsigned char c = kMsgLoserArrival;
		int result = MPI_Send(&c, 1, MPI_UNSIGNED_CHAR, boost::numeric_cast<int>(opponent), 0, MPI_COMM_WORLD);
		BOOST_ASSERT(result == MPI_SUCCESS);
	}

	void arrival_wait_for_loser(unsigned opponent)
	{
		BOOST_ASSERT(opponent < m_world_size);
		unsigned char c = kUnsignedCharInvalid;
		int result = MPI_Recv(&c, 1, MPI_UNSIGNED_CHAR, boost::numeric_cast<int>(opponent), 0, MPI_COMM_WORLD, nullptr);
		BOOST_ASSERT(result == MPI_SUCCESS);
		BOOST_ASSERT(c == kMsgLoserArrival);
	}

	void wakeup_loser(unsigned opponent)
	{
		BOOST_ASSERT(opponent < m_world_size);
		unsigned char c = kMsgWinnerWakeup;
		int result = MPI_Send(&c, 1, MPI_UNSIGNED_CHAR, boost::numeric_cast<int>(opponent), 0, MPI_COMM_WORLD);
		BOOST_ASSERT(result == MPI_SUCCESS);
	}

	void wakeup_wait_for_winner(unsigned opponent)
	{
		BOOST_ASSERT(opponent < m_world_size);
		unsigned char c = kUnsignedCharInvalid;
		int result = MPI_Recv(&c, 1, MPI_UNSIGNED_CHAR, boost::numeric_cast<int>(opponent), 0, MPI_COMM_WORLD, nullptr);
		BOOST_ASSERT(result == MPI_SUCCESS);
		BOOST_ASSERT(c == kMsgWinnerWakeup);
	}


	static unsigned get_rank()
	{
		int rank = kIntInvalid;
		int result = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		BOOST_ASSERT(result == MPI_SUCCESS);

		return boost::numeric_cast<unsigned>(rank);
	}

	static unsigned get_world_size()
	{
		int world_size = kIntInvalid;
		int result = MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		BOOST_ASSERT(result == MPI_SUCCESS);

		return boost::numeric_cast<unsigned>(world_size);
	}

	unsigned m_world_size = kUnsignedInvalid;
	unsigned m_rank = kUnsignedInvalid;
};

static TournamentBarrier s_tournament_barrier;

void gtmpi_init(int num_threads)
{
	s_tournament_barrier = TournamentBarrier(num_threads);
}

void gtmpi_barrier()
{
	s_tournament_barrier.barrier();
}

void gtmpi_finalize()
{

}
