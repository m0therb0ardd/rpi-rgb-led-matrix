#include <gtest/gtest.h>
#include "../src/genetic_colors.h"

TEST(FitnessTest, ZeroDistance) {
    EXPECT_EQ(GeneticColors::calcFitness(0xFF00FF, 0xFF00FF), 0);
}

TEST(FitnessTest, SimpleDistance) {
    EXPECT_EQ(GeneticColors::calcFitness(0x000000, 0x010101), 3);
}
