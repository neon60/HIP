/*
Copyright (c) 2021 - present Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <hip_test_common.hh>
#include <hip/hip_runtime.h>

#include <atomic>
#include <thread>

__host__ __device__ void fence_system() {
#ifdef __HIP_DEVICE_COMPILE__
  __threadfence_system();
#else
  std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

__host__ __device__ void round_robin(const int id, const int num_dev, const int num_iter,
                                     volatile int* data, volatile int* flag) {
  for (int i = 0; i < num_iter; i++) {
    while (*flag % num_dev != id) fence_system();  // invalid the cache for read

    (*data)++;
    fence_system();  // make sure the store to data is sequenced before the store to flag
    (*flag)++;
    fence_system();  // invalid the cache to flush out flag
  }
}

__global__ void gpu_round_robin(const int id, const int num_dev, const int num_iter,
                                volatile int* data, volatile int* flag) {
  round_robin(id, num_dev, num_iter, data, flag);
}

TEST_CASE("Unit_threadfence_system") {
  int num_gpus = 0;
  HIP_CHECK(hipGetDeviceCount(&num_gpus));
  REQUIRE(num_gpus > 0);

  volatile int* data;
  if (hipHostMalloc(&data, sizeof(int), hipHostMallocCoherent) != hipSuccess) {
    SUCCEED("Memory allocation failed. Skip test. Is SVM atomic supported?");
  }

  constexpr int init_data = 1000;
  *data = init_data;

  volatile int* flag;
  if (hipHostMalloc(&flag, sizeof(int), hipHostMallocCoherent) != hipSuccess) {
    SUCCEED("Memory allocation failed. Skip test. Is SVM atomic supported?");
  }
  *flag = 0;

  // number of rounds per device
  constexpr int num_iter = 1000;

  // one CPU thread + 1 kernel/GPU
  const int num_dev = num_gpus + 1;

  int next_id = 0;
  std::vector<std::thread> threads;

  // create a CPU thread for the round_robin
  threads.push_back(std::thread(round_robin, next_id++, num_dev, num_iter, data, flag));

  // run one thread per GPU
  dim3 dim_block(1, 1, 1);
  dim3 dim_grid(1, 1, 1);

  // launch one kernel per device for the round robin
  for (; next_id < num_dev; ++next_id) {
    threads.push_back(std::thread([=]() {
      HIP_CHECK(hipSetDevice(next_id - 1));
      hipLaunchKernelGGL(gpu_round_robin, dim_grid, dim_block, 0, 0x0, next_id, num_dev, num_iter,
                         data, flag);
      HIP_CHECK(hipDeviceSynchronize());
    }));
  }

  for (auto& t : threads) {
    t.join();
  }

  int expected_data = init_data + num_dev * num_iter;
  int expected_flag = num_dev * num_iter;

  bool passed = *data == expected_data && *flag == expected_flag;

  HIP_CHECK(hipHostFree((void*)data));
  HIP_CHECK(hipHostFree((void*)flag));

  REQUIRE(passed == true);
}
