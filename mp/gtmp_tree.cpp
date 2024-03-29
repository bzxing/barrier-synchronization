#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <atomic>

extern "C" {
  #include "gtmp.h"
}

/*

    From the MCS Paper: A software combining tree barrier with optimized wakeup

    type node = record
        k : integer //fan in of this node
	count : integer // initialized to k
	locksense : Boolean // initially false
	parent : ^node // pointer to parent node; nil if root

	shared nodes : array [0..P-1] of node
	    //each element of nodes allocated in a different memory module or cache line

	processor private sense : Boolean := true
	processor private mynode : ^node // my group's leaf in the combining tree

	procedure combining_barrier
	    combining_barrier_aux (mynode) // join the barrier
	    sense := not sense             // for next barrier


	procedure combining_barrier_aux (nodepointer : ^node)
	    with nodepointer^ do
	        if fetch_and_decrement (&count) = 1 // last one to reach this node
		    if parent != nil
		        combining_barrier_aux (parent)
		    count := k // prepare for next barrier
		    locksense := not locksense // release waiting processors
		repeat until locksense = sense
*/

// zxing7: Add extra alignment requirement to make a node occupy entire cache line
struct alignas(LEVEL1_DCACHE_LINESIZE) node_t {
  int k;
  std::atomic<int> count;
  std::atomic<int> locksense;
  struct node_t* parent;
} ;

static int num_leaves;
static node_t* nodes;

void gtmp_barrier_aux(node_t* node, int sense);

node_t* _gtmp_get_node(int i){
  return &nodes[i];
}

void gtmp_init(int num_threads){
  int i, v, num_nodes;
  node_t* curnode;

  /*Setting constants */
  v = 1;
  while( v < num_threads)
    v *= 2;

  num_nodes = v - 1;
  num_leaves = v/2;

  /* Setting up the tree */
  nodes = (node_t*) malloc(num_nodes * sizeof(node_t));

  for(i = 0; i < num_nodes; i++){
    curnode = _gtmp_get_node(i);
    curnode->k = i < num_threads - 1 ? 2 : 1;
    curnode->count = curnode->k;
    curnode->locksense = 0;
    curnode->parent = _gtmp_get_node((i-1)/2);
  }

  curnode = _gtmp_get_node(0);
  curnode->parent = NULL;
}

void gtmp_barrier(){
  node_t* mynode;
  int sense;

  mynode = _gtmp_get_node(num_leaves - 1 + (omp_get_thread_num() % num_leaves));

  /*
     Rather than correct the sense variable after the call to
     the auxilliary method, we set it correctly before.
   */
  sense = !mynode->locksense;

  gtmp_barrier_aux(mynode, sense);
}

void gtmp_barrier_aux(node_t* node, int sense){

  int test = node->count.fetch_sub(1); // zxing7: Use atomic RMW instruction instead of traditional mutex.
                                       // Most performance gain comes from here
                                       // Overall, the time taken for 2^22 barrier crossings reduced
                                       // from 9-10 seconds to 7-8 seconds, as measured by my own
                                       // test case in main.cpp

  if( 1 == test )
  {
    if(node->parent != NULL)
      gtmp_barrier_aux(node->parent, sense);
    node->count = node->k;
    node->locksense = sense; // zxing7: makes more sense to use already-inverted local variable instead of taking the shared node data then burn a cycle to invert it,
                             // Performance gain should be minor (not a hotspot), but peace of mind hey, guarantees no race condition.
  }
  else // zxing7: Adding else clause mostly for clarity not for performance
  {
    while (node->locksense != sense);
  }

}

void gtmp_finalize(){
  free(nodes);
}

