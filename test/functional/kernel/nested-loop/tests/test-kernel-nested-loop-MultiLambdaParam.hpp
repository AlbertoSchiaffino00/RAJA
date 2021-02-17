//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef __TEST_KERNEL_NESTED_LOOP_MULTI_LAMBDA_PARAM_HPP__
#define __TEST_KERNEL_NESTED_LOOP_MULTI_LAMBDA_PARAM_HPP__

using MultiLambdaParamSupportedLoopTypeList = camp::list<
  DEPTH_3
  >;

template <typename WORKING_RES, typename EXEC_POLICY>
typename std::enable_if<is_not_null_exec_pol<EXEC_POLICY>::value>::type
KernelNestedLoopTest(){

  constexpr static int N = 1000;
  constexpr static int DIM = 2;

  camp::resources::Resource host_res{camp::resources::Host()};
  camp::resources::Resource work_res{WORKING_RES::get_default()};

  // Allocate Tests Data
  double* work_arrA = work_res.allocate<double>(N*N);
  double* work_arrB = work_res.allocate<double>(N*N);
  double* work_arrC = work_res.allocate<double>(N*N);

  double* test_arrA = host_res.allocate<double>(N*N);
  double* test_arrB = host_res.allocate<double>(N*N);
  double* test_arrC = host_res.allocate<double>(N*N);

  double* check_arrC = host_res.allocate<double>(N*N);

  // Initialize RAJA Views
  RAJA::View< double, RAJA::Layout<DIM> > test_viewA(test_arrA, N, N);
  RAJA::View< double, RAJA::Layout<DIM> > test_viewB(test_arrB, N, N);
  RAJA::View< double, RAJA::Layout<DIM> > test_viewC(test_arrC, N, N);

  RAJA::View< double, RAJA::Layout<DIM> > work_viewA(work_arrA, N, N);
  RAJA::View< double, RAJA::Layout<DIM> > work_viewB(work_arrB, N, N);
  RAJA::View< double, RAJA::Layout<DIM> > work_viewC(work_arrC, N, N);

  // Initialize Data
  for (int row = 0; row < N; ++row) {
    for (int col = 0; col < N; ++col) {
      test_viewA(row, col) = row;
      test_viewB(row, col) = col;
      test_viewB(row, col) = 0;
    }
  }

  work_res.memcpy(work_arrA, test_arrA, sizeof(double) * RAJA::stripIndexType(N*N));
  work_res.memcpy(work_arrB, test_arrB, sizeof(double) * RAJA::stripIndexType(N*N));
  work_res.memcpy(work_arrC, test_arrC, sizeof(double) * RAJA::stripIndexType(N*N));

  // Calculate Test data
  for (int row = 0; row < N; ++row) {
    for (int col = 0; col < N; ++col) {

      double dot = 0.0;
      for (int k = 0; k < N; ++k) {
        dot += test_viewA(row, k) * test_viewB(k, col);
      }
      test_viewC(row, col) = dot;

    }
  }
  
  // Calculate Working data
  RAJA::kernel_param<EXEC_POLICY>(
    RAJA::make_tuple(RAJA::RangeSegment{0, N},
                     RAJA::RangeSegment{0, N},
                     RAJA::RangeSegment{0, N}),

    RAJA::tuple<double>{0.0},

    // lambda 0
    [=] (double& dot) {
       dot = 0.0;
    },

    // lambda 1
    [=] (int col, int row, int k, double& dot) {
       dot += work_viewA(row, k) * work_viewB(k, col);
    },

    // lambda 2
    [=] (int col, int row, double& dot) {
       work_viewC(row, col) = dot;
    }

  );

  work_res.memcpy(check_arrC, work_arrC, sizeof(double) * RAJA::stripIndexType(N*N));

  RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment{0, N*N}, [=] (RAJA::Index_type i) {
    ASSERT_TRUE( std::abs(test_arrC[i] - check_arrC[i]) < 10e-8 );
  });

  work_res.deallocate(work_arrA);
  work_res.deallocate(work_arrB);
  work_res.deallocate(work_arrC);
  host_res.deallocate(test_arrA);
  host_res.deallocate(test_arrB);
  host_res.deallocate(test_arrC);
  host_res.deallocate(check_arrC);
}

//
//
// Defining the Kernel Loop structure for MultiLambdaParam Nested Loop Tests.
//
//
template<typename POLICY_TYPE, typename POLICY_DATA>
struct MultiLambdaParamNestedLoopExec { using type = NULL_T; };

template<typename POLICY_DATA>
struct MultiLambdaParamNestedLoopExec<DEPTH_3, POLICY_DATA> {
  using type = 
    RAJA::KernelPolicy<
      RAJA::statement::For<1, typename camp::at<POLICY_DATA, camp::num<0>>::type,
        RAJA::statement::For<0, typename camp::at<POLICY_DATA, camp::num<1>>::type,
          RAJA::statement::Lambda<0, RAJA::Params<0>>,  // dot = 0.0
          RAJA::statement::For<2, typename camp::at<POLICY_DATA, camp::num<2>>::type,
            RAJA::statement::Lambda<1> // inner loop: dot += ...
          >,
          RAJA::statement::Lambda<2, RAJA::Segs<0, 1>, RAJA::Params<0>>   // set C(row, col) = dot
        >
      >
    >;
};

//
//
// Setup the Nested Loop Multi Lambda g-tests.
//
//
TYPED_TEST_SUITE_P(KernelNestedLoopMultiLambdaParamTest);
template <typename T>
class KernelNestedLoopMultiLambdaParamTest : public ::testing::Test {};

TYPED_TEST_P(KernelNestedLoopMultiLambdaParamTest, NestedLoopMultiLambdaParamKernel) {
  using WORKING_RES = typename camp::at<TypeParam, camp::num<0>>::type;
  using EXEC_POL_DATA = typename camp::at<TypeParam, camp::num<1>>::type;

  // Attain the loop depth type from execpol data.
  using LOOP_TYPE = typename EXEC_POL_DATA::LoopType;

  // Get List of loop exec policies.
  using LOOP_POLS = typename EXEC_POL_DATA::type;

  // Build proper basic kernel exec policy type.
  using EXEC_POLICY = typename MultiLambdaParamNestedLoopExec<LOOP_TYPE, LOOP_POLS>::type;

  // For double nested loop tests the third arg is ignored.
  KernelNestedLoopTest<WORKING_RES, EXEC_POLICY>();
}

REGISTER_TYPED_TEST_SUITE_P(KernelNestedLoopMultiLambdaParamTest,
                            NestedLoopMultiLambdaParamKernel);

#endif  // __TEST_KERNEL_NESTED_LOOP_MULTI_LAMBDA_PARAM_HPP__
