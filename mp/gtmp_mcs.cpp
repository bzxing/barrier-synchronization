#include <string>
#include <algorithm>
#include <type_traits>
#include <iostream>
#include <limits>
#include <vector>
#include <utility>
#include <atomic>
#include <memory>

#include <boost/assert.hpp>
#include <boost/integer.hpp>
#include <boost/range/irange.hpp>
#include <boost/container/small_vector.hpp>

#include <omp.h>

#include "strong_int.h"
#include "strong_vec.h"
#include "gtmp.h"

/*
    From the MCS Paper: A scalable, distributed tree-based barrier with only local spinning.

    type treenode = record
        parentsense : Boolean
	parentpointer : ^Boolean
	childpointers : array [0..1] of ^Boolean
	havechild : array [0..3] of Boolean
	childnotready : array [0..3] of Boolean
	dummy : Boolean //pseudo-data

    shared nodes : array [0..P-1] of treenode
        // nodes[vpid] is allocated in shared memory
        // locally accessible to processor vpid
    processor private vpid : integer // a unique virtual processor index
    processor private sense : Boolean

    // on processor i, sense is initially true
    // in nodes[i]:
    //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
    //    parentpointer = &nodes[floor((i-1)/4].childnotready[(i-1) mod 4],
    //        or dummy if i = 0
    //    childpointers[0] = &nodes[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &nodes[2*i+2].parentsense, or &dummy if 2*i+2 >= P
    //    initially childnotready = havechild and parentsense = false

    procedure tree_barrier
        with nodes[vpid] do
	    repeat until childnotready = {false, false, false, false}
	    childnotready := havechild //prepare for next barrier
	    parentpointer^ := false //let parent know I'm ready
	    // if not root, wait until my parent signals wakeup
	    if vpid != 0
	        repeat until parentsense = sense
	    // signal children in wakeup tree
	    childpointers[0]^ := sense
	    childpointers[1]^ := sense
	    sense := not sense
*/

struct NodeIdTag {};
using NodeId = StrongInt<unsigned, NodeIdTag>;


template <unsigned ArriveK, unsigned WakeupK>
class GenericMcsTree
{
    static_assert(ArriveK > 0, "");
    static_assert(WakeupK > 0, "");

public:

    GenericMcsTree() = default;


    void init(int omp_num_threads)
    {
        *this = GenericMcsTree();

        BOOST_ASSERT(omp_num_threads > 0);

        NodeId num_nodes(omp_num_threads);
        m_num_nodes = num_nodes;

        m_nodes.reserve(num_nodes);

        for (NodeId inode(0); inode < num_nodes; ++inode)
        {
            m_nodes.emplace_back( get_num_children_to_arrive(inode) );
        }
    }

    void barrier(int omp_thread_num)
    {
        NodeId inode(omp_thread_num);

        check_node_id(inode);

        NodeId iparent = get_parent_id_to_arrive(inode);
        Node * parent = iparent.is_valid() ? &(m_nodes[iparent]) : nullptr;

        unsigned nth_arrival_child = which_arrival_child(inode);

        auto wakeup_range = get_children_range_to_wake_up(inode);

        m_nodes[inode].barrier(parent, nth_arrival_child, wakeup_range, get_num_children_to_arrive(inode) );
    }


private:

    NodeId get_num_nodes() const
    {
        return m_num_nodes;
    }


    class alignas(64) Node
    {
        using ArrivalWord = uint32_t;

        static constexpr const unsigned kMaxChildren = std::numeric_limits<ArrivalWord>::digits;

        static constexpr ArrivalWord get_initial_arrival_word(unsigned num_children_to_arrive)
        {
            if (num_children_to_arrive == kMaxChildren)
            {
                return 0;
            }

            BOOST_ASSERT(num_children_to_arrive < kMaxChildren);

            ArrivalWord word = 1;

            word = (word << num_children_to_arrive);

            --word;

            word = ~word;

            return (word);
        }

        static constexpr ArrivalWord get_all_arrived_word()
        {
            ArrivalWord all_arrived = 0;
            all_arrived = ~all_arrived;
            return all_arrived;
        }

        // Some simple tests
        static_assert( get_initial_arrival_word(kMaxChildren) == 0 , "If node has kMaxChildren children, initial arrival word should be all 0s" );
        static_assert( get_initial_arrival_word(0) == get_all_arrived_word() , "If node has 0 children, initial arrival word should be all 1s" );

    public:

        Node(unsigned num_children_to_arrive) :
            m_arrival_word( get_initial_arrival_word(num_children_to_arrive) ),
            m_lock_sense(false)
        {

        }

        // No copying allowed: Node objects must stay in the array as is.
        Node(const Node &) = delete;
        Node & operator=(const Node &) = delete;

        Node(Node && other) :
            m_arrival_word( other.m_arrival_word.load() ),
            m_lock_sense(other.m_lock_sense.load() )
        {

        }

        Node & operator=(Node && other)
        {
            m_arrival_word = ( other.m_arrival_word.load() );
            m_lock_sense = (other.m_lock_sense.load() );

            return *this;
        }


        template <class ChildrenRange>
        void barrier(Node * parent_to_arrive, unsigned nth_arrival_child, ChildrenRange wakeup_children_range, unsigned num_children_to_arrive)
        {
            // Step 0: Remember lock sense
            bool ori_lock_sense = m_lock_sense.load();

            // Step 1: wait until all arrived
            while ( m_arrival_word.load() != get_all_arrived_word() );

            m_arrival_word.store( get_initial_arrival_word(num_children_to_arrive) );


            if (parent_to_arrive)
            {
                // Step 2: signal parent
                parent_to_arrive->mark_arrive(nth_arrival_child);

                // Step 3: spin on lock sense reversal by parent
                while ( m_lock_sense.load() == ori_lock_sense );
            }
            else
            {
                // Step 2 and 3: is root, sense-reverse myself
                m_lock_sense.store(!ori_lock_sense);
            }



            // Step 4: spread lock sense to wakeup children
            for (Node & child : wakeup_children_range)
            {
                child.wakeup(!ori_lock_sense);
            }
        }

        void mark_arrive(unsigned nth_arrival_child)
        {
            ArrivalWord old_word, new_word;
            old_word = m_arrival_word.load();

            ArrivalWord mask = 1;
            mask <<= nth_arrival_child;

            do
            {
                new_word = old_word | mask;

            } while( !m_arrival_word.compare_exchange_weak(old_word, new_word) );
        }

        void wakeup(bool new_sense)
        {
            m_lock_sense.store(new_sense);
        }


    private:
        std::atomic<ArrivalWord> m_arrival_word;
        std::atomic<bool> m_lock_sense;
    };

    using NodeVec = StrongVec< boost::container::small_vector<Node, 32> , NodeId >;


    // Utilities to traverse up and down an array tree.
    // This class is unaware of the tree size, and
    // assumes the tree expands from root indefinitely.
    // You have to handle nodes with incomplete or no children.
    template <unsigned K>
    class NodeFinder
    {
        static_assert(K > 0, "");

    public:

        static NodeId get_parent_id(NodeId ichild)
        {
            unsigned raw = ichild.valid_base();

            if (raw == 0)
            {
                return NodeId();
            }

            return NodeId( (raw - 1u) / K );
        }

        static std::pair<NodeId, NodeId> get_children_id_range(NodeId iparent)
        {
            unsigned raw = iparent.valid_base();

            unsigned begin = raw * K + 1;
            unsigned end = begin + K;

            return std::make_pair( NodeId(begin), NodeId(end) );
        }

    private:

    };

    NodeId check_node_id(NodeId id) const
    {
        BOOST_ASSERT( id.is_valid() );
        BOOST_ASSERT( id >= NodeId(0) );
        BOOST_ASSERT( id < get_num_nodes() );

        return id;
    }

    NodeId check_node_id_casual(NodeId id) const
    {
        if ( id.is_valid() )
        {
            BOOST_ASSERT( id >= NodeId(0) );
            BOOST_ASSERT( id < get_num_nodes() );
        }

        return id;
    }

    NodeId check_node_id_end(NodeId id) const
    {
        BOOST_ASSERT( id.is_valid() );
        BOOST_ASSERT( id >= NodeId(0) );
        BOOST_ASSERT( id <= get_num_nodes() );

        return id;
    }


    // Helpers to go up and down the arrival or wakeup trees, that know
    // the tree size. (Handles less than K child list properly.)
    NodeId get_parent_id_to_arrive(NodeId ichild) const
    {
        BOOST_ASSERT(ichild < get_num_nodes());

        NodeFinder<ArriveK> node_finder;
        return check_node_id_casual(node_finder.get_parent_id(ichild));
    }

    template <unsigned K>
    auto get_children_id_range(NodeId iparent) const
    {
        BOOST_ASSERT(iparent < get_num_nodes());

        NodeFinder<K> node_finder;
        auto range = node_finder.get_children_id_range(iparent);
        BOOST_ASSERT(range.first < range.second);

        range.second = std::min(range.second, get_num_nodes());

        if (range.first >= range.second)
        {
            range.first = NodeId();
            range.second = NodeId();
        }
        else
        {
            range.first = check_node_id(range.first);
            range.second = check_node_id_end(range.second);
            BOOST_ASSERT(range.first < range.second);
        }

        return range;
    }

    auto get_children_id_range_to_wake_up(NodeId iparent) const
    {
        return get_children_id_range<WakeupK>(iparent);
    }

    auto get_children_range_to_wake_up(NodeId iparent)
    {
        return get_node_range( get_children_id_range_to_wake_up(iparent) );
    }

    auto get_children_id_range_to_arrive(NodeId iparent) const
    {
        return get_children_id_range<ArriveK>(iparent);
    }

    unsigned get_num_children_to_arrive(NodeId iparent) const
    {
        auto range = get_children_id_range_to_arrive(iparent);
        if (!range.first.is_valid() && !range.second.is_valid())
        {
            return 0;
        }
        BOOST_ASSERT(range.first.is_valid() && range.second.is_valid());
        BOOST_ASSERT(range.second > range.first);
        NodeId size = range.second - range.first;
        return boost::numeric_cast<unsigned>( size.valid_base() );
    }

    unsigned which_arrival_child(NodeId ichild) const
    {
        BOOST_ASSERT(ichild.is_valid());

        NodeId arrival_parent = get_parent_id_to_arrive(ichild);

        if (!arrival_parent.is_valid())
        {
            return std::numeric_limits<unsigned>::max();
        }

        auto child_range = get_children_id_range_to_arrive(arrival_parent);
        NodeId begin_child = (child_range.first);
        NodeId end_child = (child_range.second);

        BOOST_ASSERT(begin_child.is_valid());
        BOOST_ASSERT(end_child.is_valid());
        BOOST_ASSERT(ichild >= begin_child);
        BOOST_ASSERT(ichild < end_child);

        return ichild.valid_base() - begin_child.valid_base();
    }


    template <class NodeIdRange>
    auto get_node_range(NodeIdRange id_range)
    {
        NodeId ibegin = (id_range.first);
        NodeId iend = (id_range.second);

        if (!ibegin.is_valid() && !iend.is_valid())
        {
            return boost::make_iterator_range(m_nodes.end(), m_nodes.end());
        }

        BOOST_ASSERT(ibegin.is_valid());
        BOOST_ASSERT(iend.is_valid());
        BOOST_ASSERT(ibegin < get_num_nodes());
        BOOST_ASSERT(iend <= get_num_nodes());

        auto begin = m_nodes.begin() + ibegin.valid_base();
        auto end = m_nodes.begin() + iend.valid_base();

        return boost::make_iterator_range(begin, end);
    }

    NodeVec m_nodes;
    NodeId m_num_nodes; // Used to tell member functions the number of nodes during the construction of m_nodes
};

using McsTree = GenericMcsTree<4, 2>;

static McsTree s_instance;

void gtmp_init(int num_threads)
{
    s_instance.init(num_threads);
}

void gtmp_barrier()
{
    int thread_id = omp_get_thread_num();
    s_instance.barrier(thread_id);
}

void gtmp_finalize()
{
}
