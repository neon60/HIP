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

/*
This file verifies the following scenarios of hipMemcpy2DFromArray API
1. Negative Scenarios
2. Extent Validation Scenarios
3. hipMemcpy2DFromArray Basic Scenario
4. Pinned Memory scenarios on same and peer GPU
5. Device Context change scenario where memory is allocated in
   one GPU and API is triggered from peer GPU.
*/

#include <hip_test_common.hh>
#include <hip_test_checkers.hh>

static constexpr auto NUM_W{10};
static constexpr auto NUM_H{10};
/*
 * This testcase verifies device to host copy for hipMemcpy2DFromArray API
 * INPUT:  Copying Host variable hData(Initialized with value Phi(1.618))
 *         --> A_d device variable
 * OUTPUT: For validating the result,Copying A_d device variable
 *         --> A_h host variable
 *         and verifying A_h with Phi
 */
TEST_CASE("Unit_hipMemcpy2DFromArray_Basic") {
  HIP_CHECK(hipSetDevice(0));
  hipArray *A_d{nullptr};
  size_t width{sizeof(float)*NUM_W};
  float *A_h{nullptr}, *hData{nullptr};
  // Initialization of variables
  HipTest::initArrays<float>(nullptr, nullptr, nullptr,
                             &A_h, &hData, nullptr,
                             width*NUM_H, false);
  hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
  HIP_CHECK(hipMallocArray(&A_d, &desc, NUM_W, NUM_H, hipArrayDefault));
  HipTest::setDefaultData<float>(width*NUM_H, A_h, hData, nullptr);

  HIP_CHECK(hipMemcpy2DToArray(A_d, 0, 0, hData, width,
                             width, NUM_H,
                             hipMemcpyHostToDevice));

  HIP_CHECK(hipMemcpy2DFromArray(A_h, width, A_d,
                               0, 0, width, NUM_H,
                               hipMemcpyDeviceToHost));
  REQUIRE(HipTest::checkArray(A_h, hData, NUM_W, NUM_H) == true);

  // Cleaning the memory
  HIP_CHECK(hipFreeArray(A_d));
  HipTest::freeArrays<float>(nullptr, nullptr, nullptr,
                             A_h, hData, nullptr, false);
}

/*
 * This testcase verifies the extent validation scenarios
 * of hipMemcpy2DFromArray API
 */
TEST_CASE("Unit_hipMemcpy2DFromArray_ExtentValidation") {
  HIP_CHECK(hipSetDevice(0));
  hipArray *A_d{nullptr};
  size_t width{sizeof(float)*NUM_W};
  float *A_h{nullptr}, *hData{nullptr};
  // Initialization of variables
  HipTest::initArrays<float>(nullptr, nullptr, nullptr,
                             &A_h, &hData, nullptr,
                             width*NUM_H, false);
  hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
  HIP_CHECK(hipMallocArray(&A_d, &desc, NUM_W, NUM_H, hipArrayDefault));

  SECTION("Destination width is 0") {
    REQUIRE(hipMemcpy2DFromArray(A_h, 0, A_d,
                                 0, 0, NUM_W*sizeof(float),
                                 NUM_H, hipMemcpyDeviceToHost) != hipSuccess);
  }
  // hipMemcpy2DFromArray API would return success for width and height as 0
  // and does not perform any copy
  // Validating the result with the initialized value
  // 1.Initializing A_d with Pi value
  // 2.copying A_d-->hData variable
  //   with height 0(copy will not be performed)
  // 3 validating hData<-->A_h which will not be equal as copy is not done.
  SECTION("Height is 0") {
    HIP_CHECK(hipMemcpy2DToArray(A_d, 0, 0,
                               A_h, width, width,
                               NUM_H, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy2DFromArray(hData, width, A_d,
                                   0, 0, NUM_W*sizeof(float),
                                   0, hipMemcpyDeviceToHost));
    REQUIRE(HipTest::checkArray(hData, A_h, NUM_W, NUM_H) != true);
  }
  // hipMemcpy2DFromArray API would return success for width and height as 0
  // and does not perform any copy
  // Validating the result with the initialized value
  // 1.Initializing A_d with Pi value
  // 2.copying A_d-->hData variable
  //   with width 0(copy will not be performed)
  // 3 validating hData<-->A_h which will not be equal as copy is not done.

  SECTION("Width is 0") {
    HIP_CHECK(hipMemcpy2DToArray(A_d, 0, 0,
                               A_h, width, width,
                               NUM_H, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy2DFromArray(hData, width, A_d,
                                   0, 0, 0,
                                   NUM_H, hipMemcpyDeviceToHost));
    REQUIRE(HipTest::checkArray(hData, A_h, NUM_W, NUM_H) != true);
  }

  // Cleaning the memory
  HIP_CHECK(hipFreeArray(A_d));
  HipTest::freeArrays<float>(nullptr, nullptr, nullptr,
                             A_h, hData, nullptr, false);
}
/*
 * This Scenario Verifies hipMemcpy2DFromArray API by copying the
 * data from pinned host memory to device on same GPU
 * INPUT:  Copying Host variable PinnMem(Initialized with value "10" )
 *         --> A_d device variable
 * OUTPUT: For validating the result,Copying A_d device variable
 *         --> A_h host variable
 *         and verifying A_h with PinnedMem[0](i.e., 10)
 */
TEST_CASE("Unit_hipMemcpy2DFromArray_PinnedMemSameGPU") {
  HIP_CHECK(hipSetDevice(0));
  hipArray *A_d{nullptr};
  constexpr auto def_val{10};
  size_t width{sizeof(float)*NUM_W};
  float *A_h{nullptr}, *PinnMem{nullptr};

  // Initialization of variables
  HipTest::initArrays<float>(nullptr, nullptr, nullptr,
                             &A_h, nullptr, nullptr,
                             width*NUM_H, false);
  HIP_CHECK(hipHostMalloc(reinterpret_cast<void**>(&PinnMem), width * NUM_H));
  hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
  HIP_CHECK(hipMallocArray(&A_d, &desc, NUM_W, NUM_H, hipArrayDefault));
  HipTest::setDefaultData<float>(width*NUM_H, A_h, nullptr, nullptr);
  for (int i = 0; i < NUM_W*NUM_H; i++) {
    PinnMem[i] = def_val + i;
  }
  HIP_CHECK(hipMemcpy2DToArray(A_d, 0, 0, PinnMem,
                               width, width, NUM_H,
                               hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy2DFromArray(A_h, width, A_d,
                                 0, 0, width, NUM_H,
                                 hipMemcpyDeviceToHost));
  REQUIRE(HipTest::checkArray(A_h, PinnMem, NUM_W, NUM_H) == true);

  // Cleaning the memory
  HIP_CHECK(hipFreeArray(A_d));
  HIP_CHECK(hipHostFree(PinnMem));
  HipTest::freeArrays<float>(nullptr, nullptr, nullptr,
                             A_h, nullptr, nullptr, false);
}
/*
 * This Scenario Verifies hipMemcpy2DFromArray API by copying the
 * data from pinned host memory to device from Peer GPU.
 * Device Memory is allocated in GPU 0 and the API is trigerred from GPU1
 * INPUT:  Intializa A_d with A_h
 *         Copy A_d->E_h which is a pinned host memory
 * OUTPUT: For validating the result,Copying A_d device variable
 *         --> E_h host variable
 *         and verifying A_h with E_h
 */
TEST_CASE("Unit_hipMemcpy2DFromArray_multiDevicePinnedMemPeerGpu") {
  int numDevices = 0;
  constexpr auto def_val{10};
  HIP_CHECK(hipGetDeviceCount(&numDevices));
  if (numDevices > 1) {
    int canAccessPeer = 0;
    hipDeviceCanAccessPeer(&canAccessPeer, 0, 1);
    if (canAccessPeer) {
      HIP_CHECK(hipSetDevice(0));
      hipArray *A_d{nullptr};
      size_t width{sizeof(float)*NUM_W};
      float *A_h{nullptr}, *E_h{nullptr};

      // Initialization of variables
      HipTest::initArrays<float>(nullptr, nullptr, nullptr,
                                 &A_h, nullptr, nullptr,
                                 width*NUM_H, false);
      hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
      HIP_CHECK(hipMallocArray(&A_d, &desc, NUM_W, NUM_H, hipArrayDefault));
      HipTest::setDefaultData<float>(width*NUM_H, A_h, nullptr, nullptr);
      HIP_CHECK(hipMemcpy2DToArray(A_d, 0, 0, A_h,
                                   width, width, NUM_H,
                                   hipMemcpyHostToDevice));
      HIP_CHECK(hipHostMalloc(reinterpret_cast<void**>(&E_h), width * NUM_H));
      for (int i = 0; i < NUM_W*NUM_H; i++) {
        E_h[i] = def_val + i;
      }
      HIP_CHECK(hipSetDevice(1));
      HIP_CHECK(hipMemcpy2DFromArray(E_h, width, A_d,
                                     0, 0, width, NUM_H,
                                     hipMemcpyDeviceToHost));
      REQUIRE(HipTest::checkArray(A_h, E_h, NUM_W, NUM_H) == true);

      // Cleaning the memory
      HIP_CHECK(hipFreeArray(A_d));
      HIP_CHECK(hipHostFree(E_h));
      HipTest::freeArrays<float>(nullptr, nullptr, nullptr,
          A_h, nullptr, nullptr, false);
    } else {
      SUCCEED("Device Does not have P2P capability");
    }
  } else {
    SUCCEED("Number of devices are < 2");
  }
}

/*
 * This scenario verifies the hipMemcpy2DFromArray API in case of device
 * context change.
 * Memory is allocated in GPU-0 and the API is triggered from GPU-1
 * INPUT:  Copying Host variable hData(Initial value Phi)
 *         --> A_d device variable
 *         whose memory is allocated in GPU 0
 * OUTPUT: For validating the result,Copying A_d device variable
 *         --> A_h host variable
 *         and verifying A_h with Phi
 * */
TEST_CASE("Unit_hipMemcpy2DFromArray_multiDeviceContextChange") {
  int numDevices = 0;
  HIP_CHECK(hipGetDeviceCount(&numDevices));
  if (numDevices > 1) {
    int canAccessPeer = 0;
    hipDeviceCanAccessPeer(&canAccessPeer, 0, 1);
    if (canAccessPeer) {
      HIP_CHECK(hipSetDevice(0));
      hipArray *A_d{nullptr};
      size_t width{sizeof(float)*NUM_W};
      float *A_h{nullptr}, *hData{nullptr};

      // Initialization of variables
      HipTest::initArrays<float>(nullptr, nullptr, nullptr,
          &A_h, &hData, nullptr,
          width*NUM_H, false);
      hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
      HIP_CHECK(hipMallocArray(&A_d, &desc, NUM_W, NUM_H, hipArrayDefault));
      HipTest::setDefaultData<float>(width*NUM_H, A_h, hData, nullptr);

      HIP_CHECK(hipSetDevice(1));
      HIP_CHECK(hipMemcpy2DToArray(A_d, 0, 0, hData, width,
                                   width, NUM_H,
                                   hipMemcpyHostToDevice));

      HIP_CHECK(hipMemcpy2DFromArray(A_h, width, A_d,
                                     0, 0, width, NUM_H,
                                     hipMemcpyDeviceToHost));
      REQUIRE(HipTest::checkArray(A_h, hData, NUM_W, NUM_H) == true);

      // Cleaning the memory
      HIP_CHECK(hipFreeArray(A_d));
      HipTest::freeArrays<float>(nullptr, nullptr, nullptr,
          A_h, hData, nullptr, false);
    } else {
      SUCCEED("Device Does not have P2P capability");
    }
  } else {
    SUCCEED("Number of devices are < 2");
  }
}
/* This testcase verifies the negative scenarios of
 * hipMemcpy2DFromArray API
 */
TEST_CASE("Unit_hipMemcpy2DFromArray_Negative") {
  HIP_CHECK(hipSetDevice(0));
  hipArray *A_d{nullptr};
  size_t width{sizeof(float)*NUM_W};
  float *A_h{nullptr}, *hData{nullptr};

  // Initialization of variables
  HipTest::initArrays<float>(nullptr, nullptr, nullptr,
                             &A_h, &hData, nullptr,
                             width*NUM_H, false);
  HipTest::setDefaultData<float>(width*NUM_H, A_h, hData, nullptr);
  hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
  HIP_CHECK(hipMallocArray(&A_d, &desc, NUM_W, NUM_H, hipArrayDefault));

  SECTION("Nullptr to destination") {
    REQUIRE(hipMemcpy2DFromArray(nullptr, width, A_d,
                                 0, 0, width, NUM_H,
                                 hipMemcpyDeviceToHost) != hipSuccess);
  }

  SECTION("Nullptr to source") {
    REQUIRE(hipMemcpy2DFromArray(A_h, width, nullptr,
                                 0, 0, width, NUM_H,
                                 hipMemcpyDeviceToHost) != hipSuccess);
  }

  SECTION("Passing offset more than 0") {
    REQUIRE(hipMemcpy2DFromArray(A_h, width, A_d, 1,
                                 1, width, NUM_H,
                                 hipMemcpyDeviceToHost) != hipSuccess);
  }

  SECTION("Passing array more than allocated") {
    REQUIRE(hipMemcpy2DFromArray(A_h, width, A_d, 0,
                                 0, width+2, NUM_H+2,
                                 hipMemcpyDeviceToHost) != hipSuccess);
  }

  // Cleaning of memory
  HIP_CHECK(hipFreeArray(A_d));
  HipTest::freeArrays<float>(nullptr, nullptr, nullptr,
                             A_h, hData, nullptr, false);
}

