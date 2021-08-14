//
//
//

#include <gtest/gtest.h>
#include "AutoVersion.h"

TEST(AutoVersionTest, ParsesUpstreamOnly)
{
    AutoVersion v("1");
    EXPECT_EQ(0, v.Epoch());
    EXPECT_STREQ("1", v.Upstream());
    EXPECT_STREQ("0", v.Revision());
}

TEST(AutoVersionTest, ParsesEpochs)
{
    AutoVersion v("1:2.0.1");
    EXPECT_EQ(1, v.Epoch());
    EXPECT_STREQ("2.0.1", v.Upstream());
    EXPECT_STREQ("0", v.Revision());
}

TEST(AutoVersionTest, ParsesRevisions)
{
    AutoVersion v("10:4.0.1~alpha-4-5");
    EXPECT_EQ(10, v.Epoch());
    EXPECT_STREQ("4.0.1~alpha-4", v.Upstream());
    EXPECT_STREQ("5", v.Revision());
}

TEST(AutoVersionTest, ComparesNumbers)
{
    EXPECT_LT(AutoVersion("1"), AutoVersion("2"));
    EXPECT_EQ(AutoVersion("10"), AutoVersion("10"));
    EXPECT_LT(AutoVersion("9"), AutoVersion("10"));
    EXPECT_GT(AutoVersion("10"), AutoVersion("9"));
}

TEST(AutoVersionTest, ComparesEpochs)
{
    EXPECT_GT(AutoVersion("2:1"), AutoVersion("1:2"));
    EXPECT_LT(AutoVersion("10"), AutoVersion("1:2"));
}

TEST(AutoVersionTest, ComparesAlphas)
{
    EXPECT_LT(AutoVersion("alpha"), AutoVersion("beta"));
    EXPECT_LT(AutoVersion("alpha1"), AutoVersion("alpha2"));
    EXPECT_GT(AutoVersion("alpha10"), AutoVersion("alpha2"));
}

TEST(AutoVersionTest, ComparesTildes)
{
    EXPECT_LT(AutoVersion("3.0~beta1"), AutoVersion("3.0"));
    EXPECT_GT(AutoVersion("3.0~beta"), AutoVersion("3.0~~prebeta"));
    EXPECT_LT(AutoVersion("3.0~beta4"), AutoVersion("3.0~rc1"));
}

TEST(AutoVersionTest, ComparesRevisions)
{
    EXPECT_LT(AutoVersion("3.0-2"), AutoVersion("3.0-10"));
}

assert mycmp('1', '2') == -1
assert mycmp('2', '1') == 1
assert mycmp('1', '1') == 0
assert mycmp('1.0', '1') == 0
assert mycmp('1', '1.000') == 0
assert mycmp('12.01', '12.1') == 0
assert mycmp('13.0.1', '13.00.02') == -1
assert mycmp('1.1.1.1', '1.1.1.1') == 0
assert mycmp('1.1.1.2', '1.1.1.1') == 1
assert mycmp('1.1.3', '1.1.3.000') == 0
assert mycmp('3.1.1.0', '3.1.2.10') == -1
assert mycmp('1.1', '1.10') == -1

