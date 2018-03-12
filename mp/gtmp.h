#ifndef GTMP_H
#define GTMP_H

// Prototypes for C functions. Must include with extern "C"
void gtmp_init(int num_threads);
void gtmp_barrier();
void gtmp_finalize();

#endif
