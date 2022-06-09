#include <gtest/gtest.h>

#include "autowired-cpp/hello_world.h"

TEST(HELLO_WORLD, hello_world) {
    EXPECT_EQ(HelloWorld(), "Hello World");
}
