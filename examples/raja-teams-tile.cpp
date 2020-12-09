//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "RAJA/RAJA.hpp"
#include "camp/resource.hpp"


/*
 * RAJA Teams Example: Upper Triangular Pattern + Shared Memory
 *
 * Teams introduces hierarchal parallelism through the concept of
 * teams and threads.  Computation is executed in a pre-defined grid
 * composed of threads and grouped into teams. The teams model enables
 * developers to express parallelism through loops over teams, and inner loops
 * over threads. Team loops are executed in parallel and
 * threads within a team should be treated as sub-parallel regions.
 *
 * Team shared memory is allocated between team and thread loops.
 * Memory allocated within thread loops are thread private.
 * The example below demonstrates composing an upper triangular
 * loop pattern, and using shared memory.
 *
 */

/*
 * Define host/device launch policies
 */
using launch_policy = RAJA::expt::LaunchPolicy<
#if defined(RAJA_ENABLE_OPENMP)
    RAJA::expt::omp_launch_t
#else
    RAJA::expt::seq_launch_t
#endif
#if defined(RAJA_ENABLE_CUDA)
    ,
    RAJA::expt::cuda_launch_t<false>
#endif
#if defined(RAJA_ENABLE_HIP)
    ,
    RAJA::expt::hip_launch_t<false>
#endif
    >;

/*
 * Define team policies.
 * Up to 3 dimension are supported: x,y,z
 */
using teams_x = RAJA::expt::LoopPolicy<
#if defined(RAJA_ENABLE_OPENMP)
                                      RAJA::omp_parallel_for_exec
#else
                                       RAJA::loop_exec
#endif
#if defined(RAJA_ENABLE_CUDA)
                                       ,
                                       RAJA::cuda_block_x_direct
#endif
#if defined(RAJA_ENABLE_HIP)
                                       ,
                                       RAJA::hip_block_x_direct
#endif
                                       >;

using loop_t = RAJA::expt::LoopPolicy<RAJA::loop_exec
#if defined(RAJA_ENABLE_CUDA)
                                       ,
                                       RAJA::cuda_thread_x_loop
#endif
#if defined(RAJA_ENABLE_HIP)
                                       ,
                                       RAJA::hip_thread_x_loop
#endif
                                       >;
/*
 * Define thread policies.
 * Up to 3 dimension are supported: x,y,z
 */
using threads_x = RAJA::expt::LoopPolicy<RAJA::loop_exec
#if defined(RAJA_ENABLE_CUDA)
                                         ,
                                         RAJA::cuda_thread_x_loop
#endif
#if defined(RAJA_ENABLE_HIP)
                                         ,
                                         RAJA::hip_thread_x_loop
#endif
                                         >;


int main(int RAJA_UNUSED_ARG(argc), char **RAJA_UNUSED_ARG(argv[]))
{

  // Resource object for host
  camp::resources::Host host_res;

  // Resource objects for CUDA or HIP
#if defined(RAJA_ENABLE_CUDA)
  camp::resources::Cuda device_res;
#endif

#if defined(RAJA_ENABLE_HIP)
  camp::resources::Hip device_res;
#endif

  std::cout << "\n Running RAJA-Teams examples...\n";
  int num_of_backends = 1;
#if defined(RAJA_DEVICE_ACTIVE)
  num_of_backends++;
#endif

  // RAJA teams may switch between host and device policies at run time.
  // The loop below will execute through the available backends.

  for (int exec_place = 0; exec_place < num_of_backends; ++exec_place) {

    RAJA::expt::ExecPlace select_cpu_or_gpu = (RAJA::expt::ExecPlace)exec_place;

    // auto select_cpu_or_gpu = RAJA::HOST;
    // auto select_cpu_or_gpu = RAJA::DEVICE;

    // Allocate memory for either host or device
    int N_tri = 10;

    int* Ddat = nullptr;
    if (select_cpu_or_gpu == RAJA::expt::HOST) {
      Ddat = host_res.allocate<int>(N_tri * N_tri);
    }

#if defined(RAJA_DEVICE_ACTIVE)
    if (select_cpu_or_gpu == RAJA::expt::DEVICE) {
      Ddat = device_res.allocate<int>(N_tri * N_tri);
    }
#endif

    /*
     * RAJA::expt::launch just starts a "kernel" and doesn't provide any looping.
     *
     * The first argument determines which policy should be executed,
     *
     * The second argument is the number of teams+threads needed for each of the
     * policies.
     *
     * Third argument is the lambda.
     *
     * The lambda takes a "resource" object, which has the teams+threads
     * and is used to perform thread synchronizations within a team.
     */

    if (select_cpu_or_gpu == RAJA::expt::HOST){
      std::cout << "\n Running upper triangular pattern example on the host...\n";
    }else {
      std::cout << "\n Running upper triangular pattern example on the device...\n";
    }


    RAJA::View<int, RAJA::Layout<2>> D(Ddat, N_tri, N_tri);

    RAJA::expt::launch<launch_policy>(select_cpu_or_gpu,
       RAJA::expt::Resources(RAJA::expt::Teams(N_tri), RAJA::expt::Threads(N_tri)),
       [=] RAJA_HOST_DEVICE(RAJA::expt::LaunchContext ctx) {

        const int TILE_SZ = 4;
        RAJA::expt::tile_idx<teams_x>(ctx, TILE_SZ, RAJA::RangeSegment(0, N_tri), [&](RAJA::RangeSegment const &r_tile, int tile_id) {

             RAJA::expt::loop_idx<loop_t>(ctx, r_tile, [&](int global_id, int loc_id) {

                 printf("global_id %d loc_id %d tile_id %d \n", global_id, loc_id, tile_id);

             }); // loop r
         });  // tile r
       });  // outer lambda

    if (select_cpu_or_gpu == RAJA::expt::HOST) {
      host_res.deallocate(Ddat);
    }

#if defined(RAJA_DEVICE_ACTIVE)
    if (select_cpu_or_gpu == RAJA::expt::DEVICE) {
      device_res.deallocate(Ddat);
    }
#endif

  }  // Execution places loop


}  // Main
