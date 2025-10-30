#include "gtest/gtest.h"
#include "gmock/gmock.h"


using ::testing::HasSubstr;
using ::testing::ThrowsMessage;
using ::testing::AllOf;


namespace {
    TEST(FooTest, Check) {
        EXPECT_EQ(1, 1);
    }
}
