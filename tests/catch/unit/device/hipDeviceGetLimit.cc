/*
Copyright (c) 2021-Present Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANNTY OF ANY KIND, EXPRESS OR
IMPLIED, INNCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANNY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER INN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR INN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
 * Conformance test for checking functionality of
 * hipError_t hipDeviceGetLimit(size_t* pValue, enum hipLimit_t limit);
 */
#include <hip_test_common.hh>
/**
 * hipDeviceGetLimit tests
 * Scenario1: Validates if pValue = nullptr returns hip error code.
 * Scenario2: Validates if *pValue > 0 is returned for limit = hipLimitMallocHeapSize.
 * Scenario3: Validates if error code is returned for limit = Invalid Flag = 0xff.
 */
TEST_CASE("Unit_hipDeviceGetLimit_NegTst") {
  size_t Value = 0;
  // Scenario1
  SECTION("NULL check") {
    REQUIRE_FALSE(hipDeviceGetLimit(nullptr, hipLimitMallocHeapSize)
                  == hipSuccess);
  }
  // Scenario3
  SECTION("Invalid Input Flag") {
    REQUIRE_FALSE(hipDeviceGetLimit(&Value, static_cast<hipLimit_t>(0xff)) ==
                  hipSuccess);
  }
}

TEST_CASE("Unit_hipDeviceGetLimit_CheckValidityOfOutputVal") {
  size_t Value = 0;
  // Scenario2
  REQUIRE(hipDeviceGetLimit(&Value, hipLimitMallocHeapSize) ==
          hipSuccess);
  REQUIRE_FALSE(Value <= 0);
}
