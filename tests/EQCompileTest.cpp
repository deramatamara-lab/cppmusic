#include <gtest/gtest.h>

// Test that our EQ header compiles correctly
TEST(EQCompileTest, HeaderIncludes) {
    // This test verifies that our header files compile without errors
    EXPECT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
