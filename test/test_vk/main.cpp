#include "algos_test.h"
#include "basic_test.h"
#include "internal_resources_test.h"
#include "hlbvh_test.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "gtest/gtest.h"
#include "tiny_obj_loader.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}