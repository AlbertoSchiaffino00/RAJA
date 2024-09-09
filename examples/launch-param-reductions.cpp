//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-24, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <cstdlib>
#include <iostream>
#include <limits>

#include "RAJA/RAJA.hpp"

/*
 *  Reduction Example
 *
 *  This example illustrates use of the RAJA reduction types: min, max,
 *  sum, min-loc, and max-loc.
 *
 *  RAJA features shown:
 *    - `forall` loop iteration template method
 *    -  Index range segment
 *    -  Execution policies
 *    -  Reduction types
 *
 * If CUDA is enabled, CUDA unified memory is used.
 */

/*
  Specify the number of threads in a GPU thread block
*/
#if defined(RAJA_ENABLE_CUDA)
constexpr int CUDA_BLOCK_SIZE = 256;
#endif

#if defined(RAJA_ENABLE_HIP)
constexpr int HIP_BLOCK_SIZE = 256;
#endif

#if defined(RAJA_ENABLE_SYCL)
//LC testing hardware has a limit of 151
constexpr int SYCL_BLOCK_SIZE = 128;
#endif

int main(int RAJA_UNUSED_ARG(argc), char** RAJA_UNUSED_ARG(argv[]))
{

  std::cout << "\n\nRAJA reductions example...\n";

  // _reductions_array_init_start
//
// Define array length
//
  constexpr int N = 1000000;

//
// Use a resource to allocate memory
//
  RAJA::resources::Host host_res;
#if defined(RAJA_ENABLE_CUDA)
  RAJA::resources::Cuda device_res;
#endif
#if defined(RAJA_ENABLE_HIP)
  RAJA::resources::Hip device_res;
#endif
#if defined(RAJA_ENABLE_SYCL)
  RAJA::resources::Sycl device_res;
#endif


//
// Allocate array data and initialize data to alternating sequence of 1, -1.
//
  int* a = host_res.allocate<int>(N);

  for (int i = 0; i < N; ++i) {
    if ( i % 2 == 0 ) {
      a[i] = 1;
    } else {
      a[i] = -1;
    }
  }

//
// Set a[0] to a different value. Total sum should be 2.
//
  a[0] = 3;

//
// Set min and max loc values
//
  constexpr int minloc_ref = N / 2;
  a[minloc_ref] = -100;

  constexpr int maxloc_ref = N / 2 + 1;
  a[maxloc_ref] = 100;
  // _reductions_array_init_end

//
// Note: with this data initialization scheme, the following results will
//       be observed for all reduction kernels below:
//
//  - the sum will be zero
//  - the min will be -100
//  - the max will be 100
//  - the min loc will be N/2
//  - the max loc will be N/2 + 1
//
//

//
// Define index range for iterating over a elements in all examples
//
  // _reductions_range_start
  RAJA::TypedRangeSegment<int> arange(0, N);
  // _reductions_range_end

//
// Define ValLoc Type
//

  //using VALLOC_INT = RAJA::expt::ValLoc<int>;
  using REF_INT_SUM = RAJA::expt::ValOp<int, RAJA::operators::plus>;
  using REF_INT_MIN = RAJA::expt::ValOp<int, RAJA::operators::minimum>;
  using REF_INT_MAX = RAJA::expt::ValOp<int, RAJA::operators::maximum>;
  using REFLOC_INT_MIN = RAJA::expt::ValLocOp<int, RAJA::Index_type, RAJA::operators::minimum>;
  using REFLOC_INT_MAX = RAJA::expt::ValLocOp<int, RAJA::Index_type, RAJA::operators::maximum>;
//----------------------------------------------------------------------------//

  std::cout << "\n Running RAJA sequential reductions...\n";

  // _reductions_raja_seq_start
  using LAUNCH_POL1   = RAJA::LaunchPolicy<RAJA::seq_launch_t>;
  using LOOP_POL1     = RAJA::LoopPolicy<RAJA::seq_exec>;

  REF_INT_SUM seq_sum(0);
  REF_INT_MIN seq_min(std::numeric_limits<int>::max());
  REF_INT_MAX seq_max(std::numeric_limits<int>::min());
  REFLOC_INT_MIN seq_minloc(std::numeric_limits<int>::max(), -1);
  REFLOC_INT_MAX seq_maxloc(std::numeric_limits<int>::min(), -1);

  RAJA::launch<LAUNCH_POL1>
    (host_res, RAJA::LaunchParams(), "SeqReductionKernel",
    RAJA::expt::Reduce<>(&seq_sum),
    RAJA::expt::Reduce<>(&seq_min),
    RAJA::expt::Reduce<>(&seq_max),
    RAJA::expt::Reduce<>(&seq_minloc),
    RAJA::expt::Reduce<>(&seq_maxloc),
     [=] RAJA_HOST_DEVICE (RAJA::LaunchContext ctx,
                           REF_INT_SUM &_seq_sum,
                           REF_INT_MIN &_seq_min,
                           REF_INT_MAX &_seq_max,
                           REFLOC_INT_MIN &_seq_minloc,
                           REFLOC_INT_MAX &_seq_maxloc) {

      RAJA::loop<LOOP_POL1>(ctx, arange, [&] (int i) {

          _seq_sum += a[i];

          _seq_min.min(a[i]);
          _seq_max.max(a[i]);

          _seq_minloc.minloc(a[i], i);
          _seq_maxloc.maxloc(a[i], i);
          // Note : RAJA::expt::ValLoc<T> objects provide min() and max() methods
          //        that are equivalent to the assignments with RAJA_MIN and RAJA_MAX
          //        above.
        }
      );

    }
  );

  std::cout << "\tsum = " << seq_sum.get() << std::endl;
  std::cout << "\tmin = " << seq_min.get() << std::endl;
  std::cout << "\tmax = " << seq_max.get() << std::endl;
  std::cout << "\tmin, loc = " << seq_minloc.getVal() << " , "
                               << seq_minloc.getLoc() << std::endl;
  std::cout << "\tmax, loc = " << seq_maxloc.getVal() << " , "
                               << seq_maxloc.getLoc() << std::endl;
  // _reductions_raja_seq_end


//----------------------------------------------------------------------------//

#if defined(RAJA_ENABLE_OPENMP)
  std::cout << "\n Running RAJA OpenMP reductions...\n";

  // _reductions_raja_omppolicy_start
  using LAUNCH_POL2   = RAJA::LaunchPolicy<RAJA::omp_launch_t>;
  using LOOP_POL2     = RAJA::LoopPolicy<RAJA::omp_for_exec>;
  // _reductions_raja_omppolicy_end

  REF_INT_SUM omp_sum(0);
  REF_INT_MIN omp_min(std::numeric_limits<int>::max());
  REF_INT_MAX omp_max(std::numeric_limits<int>::min());
  REFLOC_INT_MIN omp_minloc(std::numeric_limits<int>::max(), -1);
  REFLOC_INT_MAX omp_maxloc(std::numeric_limits<int>::min(), -1);

  RAJA::launch<LAUNCH_POL2>
    (host_res, RAJA::LaunchParams(), "OmpReductionKernel",
    RAJA::expt::Reduce<>(&omp_sum),
    RAJA::expt::Reduce<>(&omp_min),
    RAJA::expt::Reduce<>(&omp_max),
    RAJA::expt::Reduce<>(&omp_minloc),
    RAJA::expt::Reduce<>(&omp_maxloc),
     [=] RAJA_HOST_DEVICE (RAJA::LaunchContext ctx,
                           REF_INT_SUM &_omp_sum,
                           REF_INT_MIN &_omp_min,
                           REF_INT_MAX &_omp_max,
                           REFLOC_INT_MIN &_omp_minloc,
                           REFLOC_INT_MAX &_omp_maxloc) {

      RAJA::loop<LOOP_POL2>(ctx, arange, [&] (int i) {

          _omp_sum += a[i];

          _omp_min.min(a[i]);
          _omp_max.max(a[i]);

          _omp_minloc.minloc(a[i], i);
          _omp_maxloc.maxloc(a[i], i);
        }
      );

    }
  );

  std::cout << "\tsum = " << omp_sum.get() << std::endl;
  std::cout << "\tmin = " << omp_min.get() << std::endl;
  std::cout << "\tmax = " << omp_max.get() << std::endl;
  std::cout << "\tmin, loc = " << omp_minloc.getVal() << " , "
                               << omp_minloc.getLoc() << std::endl;
  std::cout << "\tmax, loc = " << omp_maxloc.getVal() << " , "
                               << omp_maxloc.getLoc() << std::endl;

#endif

//----------------------------------------------------------------------------//

#if defined(RAJA_ENABLE_CUDA)
  std::cout << "\n Running RAJA CUDA reductions...\n";

  int* d_a = device_res.allocate<int>(N);
  device_res.memcpy(d_a, a, sizeof(int) * N);

  // _reductions_raja_cudapolicy_start
  using LAUNCH_POL3   = RAJA::LaunchPolicy<RAJA::cuda_launch_t<false /*async*/>>;
  using LOOP_POL3     = RAJA::LoopPolicy<RAJA::cuda_global_thread_x>;
  // _reductions_raja_cudapolicy_end

  const int NUMBER_OF_TEAMS = (N-1)/CUDA_BLOCK_SIZE + 1;

  REF_INT_SUM cuda_sum(0);
  REF_INT_MIN cuda_min(std::numeric_limits<int>::max());
  REF_INT_MAX cuda_max(std::numeric_limits<int>::min());
  REFLOC_INT_MIN cuda_minloc(std::numeric_limits<int>::max(), -1);
  REFLOC_INT_MAX cuda_maxloc(std::numeric_limits<int>::min(), -1);

  RAJA::launch<LAUNCH_POL3>
    (device_res, RAJA::LaunchParams(RAJA::Teams(NUMBER_OF_TEAMS), RAJA::Threads(CUDA_BLOCK_SIZE)),
     "CUDAReductionKernel",
    RAJA::expt::Reduce<>(&cuda_sum),
    RAJA::expt::Reduce<>(&cuda_min),
    RAJA::expt::Reduce<>(&cuda_max),
    RAJA::expt::Reduce<>(&cuda_minloc),
    RAJA::expt::Reduce<>(&cuda_maxloc),
     [=] RAJA_HOST_DEVICE (RAJA::LaunchContext ctx,
                           REF_INT_SUM &_cuda_sum,
                           REF_INT_MIN &_cuda_min,
                           REF_INT_MAX &_cuda_max,
                           REFLOC_INT_MIN &_cuda_minloc,
                           REFLOC_INT_MAX &_cuda_maxloc) {


      RAJA::loop<LOOP_POL3>(ctx, arange, [&] (int i) {

          _cuda_sum += d_a[i];

          _cuda_min.min(d_a[i]);
          _cuda_max.max(d_a[i]);

          _cuda_minloc.minloc(a[i], i);
          _cuda_maxloc.maxloc(a[i], i);

        }
      );


    }
  );

  std::cout << "\tsum = " << cuda_sum.get() << std::endl;
  std::cout << "\tmin = " << cuda_min.get() << std::endl;
  std::cout << "\tmax = " << cuda_max.get() << std::endl;
  std::cout << "\tmin, loc = " << cuda_minloc.getVal() << " , "
                               << cuda_minloc.getLoc() << std::endl;
  std::cout << "\tmax, loc = " << cuda_maxloc.getVal() << " , "
                               << cuda_maxloc.getLoc() << std::endl;

  device_res.deallocate(d_a);
#endif

//----------------------------------------------------------------------------//

#if defined(RAJA_ENABLE_HIP)
  std::cout << "\n Running RAJA HIP reductions...\n";

  int* d_a = device_res.allocate<int>(N);
  device_res.memcpy(d_a, a, sizeof(int) * N);

  // _reductions_raja_hippolicy_start
  using LAUNCH_POL3   = RAJA::LaunchPolicy<RAJA::hip_launch_t<false /*async*/>>;
  using LOOP_POL3     = RAJA::LoopPolicy<RAJA::hip_global_thread_x>;
  // _reductions_raja_hippolicy_end

  const int NUMBER_OF_TEAMS = (N-1)/HIP_BLOCK_SIZE + 1;

  REF_INT_SUM hip_sum(0);
  REF_INT_MIN hip_min(std::numeric_limits<int>::max());
  REF_INT_MAX hip_max(std::numeric_limits<int>::min());
  REFLOC_INT_MIN hip_minloc(std::numeric_limits<int>::max(), -1);
  REFLOC_INT_MAX hip_maxloc(std::numeric_limits<int>::min(), -1);

  RAJA::launch<LAUNCH_POL3>
    (device_res, RAJA::LaunchParams(RAJA::Teams(NUMBER_OF_TEAMS), RAJA::Threads(HIP_BLOCK_SIZE)),
     "HipReductionKernel",
    RAJA::expt::Reduce<>(&hip_sum),
    RAJA::expt::Reduce<>(&hip_min),
    RAJA::expt::Reduce<>(&hip_max),
    RAJA::expt::Reduce<>(&hip_minloc),
    RAJA::expt::Reduce<>(&hip_maxloc),
     [=] RAJA_HOST_DEVICE (RAJA::LaunchContext ctx,
                           REF_INT_SUM &_hip_sum,
                           REF_INT_MIN &_hip_min,
                           REF_INT_MAX &_hip_max,
                           REFLOC_INT_MIN &_hip_minloc,
                           REFLOC_INT_MAX &_hip_maxloc) {

      RAJA::loop<LOOP_POL3>(ctx, arange, [&] (int i) {

          _hip_sum += d_a[i];

          _hip_min.min(d_a[i]);
          _hip_max.max(d_a[i]);

          _hip_minloc.minloc(d_a[i], i);
          _hip_maxloc.maxloc(d_a[i], i);

        }
      );

    }
  );

  std::cout << "\tsum = " << hip_sum.get() << std::endl;
  std::cout << "\tmin = " << hip_min.get() << std::endl;
  std::cout << "\tmax = " << hip_max.get() << std::endl;
  std::cout << "\tmin, loc = " << hip_minloc.getVal() << " , "
                               << hip_minloc.getLoc() << std::endl;
  std::cout << "\tmax, loc = " << hip_maxloc.getVal() << " , "
                               << hip_maxloc.getLoc() << std::endl;

  device_res.deallocate(d_a);
#endif

//----------------------------------------------------------------------------//

#if defined(RAJA_ENABLE_SYCL)
  std::cout << "\n Running RAJA SYCL reductions...\n";

  int* d_a = device_res.allocate<int>(N);
  device_res.memcpy(d_a, a, sizeof(int) * N);

  // _reductions_raja_syclpolicy_start
  using LAUNCH_POL4   = RAJA::LaunchPolicy<RAJA::sycl_launch_t<false /*async*/>>;
  using LOOP_POL4     = RAJA::LoopPolicy<RAJA::sycl_global_item_2>;
  // _reductions_raja_syclpolicy_end

  const int NUMBER_OF_TEAMS = (N-1)/SYCL_BLOCK_SIZE + 1;

  REF_INT_SUM sycl_sum(0);
  REF_INT_MIN sycl_min(std::numeric_limits<int>::max());
  REF_INT_MAX sycl_max(std::numeric_limits<int>::min());
  REFLOC_INT_MIN sycl_minloc(std::numeric_limits<int>::max(), -1);
  REFLOC_INT_MAX sycl_maxloc(std::numeric_limits<int>::min(), -1);

  RAJA::launch<LAUNCH_POL4>
    (device_res, RAJA::LaunchParams(RAJA::Teams(NUMBER_OF_TEAMS), RAJA::Threads(SYCL_BLOCK_SIZE)),
     "SyclReductionKernel",
    RAJA::expt::Reduce<>(&sycl_sum),
    RAJA::expt::Reduce<>(&sycl_min),
    RAJA::expt::Reduce<>(&sycl_max),
    RAJA::expt::Reduce<>(&sycl_minloc),
    RAJA::expt::Reduce<>(&sycl_maxloc),
     [=] RAJA_HOST_DEVICE (RAJA::LaunchContext ctx,
                           REF_INT_SUM &_sycl_sum,
                           REF_INT_MIN &_sycl_min,
                           REF_INT_MAX &_sycl_max,
                           REFLOC_INT_MIN &_sycl_minloc,
                           REFLOC_INT_MAX &_sycl_maxloc) {

      RAJA::loop<LOOP_POL4>(ctx, arange, [&] (int i) {

          _sycl_sum += d_a[i];

          _sycl_min.min(d_a[i]);
          _sycl_max.max(d_a[i]);

          _sycl_minloc.minloc(d_a[i], i);
          _sycl_maxloc.maxloc(d_a[i], i);

        }
      );

    }
  );

  std::cout << "\tsum = " << sycl_sum.get() << std::endl;
  std::cout << "\tmin = " << sycl_min.get() << std::endl;
  std::cout << "\tmax = " << sycl_max.get() << std::endl;
  std::cout << "\tmin, loc = " << sycl_minloc.getVal() << " , "
                               << sycl_minloc.getLoc() << std::endl;
  std::cout << "\tmax, loc = " << sycl_maxloc.getVal() << " , "
                               << sycl_maxloc.getLoc() << std::endl;

  device_res.deallocate(d_a);
#endif

//----------------------------------------------------------------------------//

//
// Clean up.
//
  host_res.deallocate(a);

  std::cout << "\n DONE!...\n";

  return 0;
}
