#include "gtest/gtest.h"
#include "gmock/gmock.h"

TEST(some_suite, some_test)
{
    ASSERT_EQ(1, 1);
    // ASSERT_EQ(1, 2); // for failure test
}

TEST(some_suite, some_test1)
{
    ASSERT_EQ(1,1);
}

TEST(some_suite2, some_test2)
{
    ASSERT_EQ(1,1);
}
