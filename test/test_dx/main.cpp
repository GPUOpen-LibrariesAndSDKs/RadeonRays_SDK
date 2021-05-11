#include "algos_test.h"
#include "basic_test.h"
#include "gtest/gtest.h"
#include "hlbvh_test.h"
#include "tiny_obj_loader.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}