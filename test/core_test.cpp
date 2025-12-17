#include <gtest/gtest.h>

// you can call DummyTest and DummyTestAssertions whatever you want
TEST(DummyTest, DummyTestAssertions) {
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}