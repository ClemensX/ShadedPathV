
#include "pch.h"
#include "test.h"

TEST(Environment, GLM) {
    glm::mat4 m1;
    glm::vec4 vec;
    auto test = m1 * vec;

    // m1 is not initialized - we expect arbitrary values:
    EXPECT_NE(m1[0].x, 1.0f);

    // initialize with Identity:
    glm::mat4 m2(1.0f);
    EXPECT_EQ(m2[0].x, 1.0f);

    // mutltiply vec with Identity and check for equality
    glm::vec4 v2(1.0f);
    auto vm = m2 * v2;
    EXPECT_EQ(v2.x, 1.0f);
    EXPECT_EQ(vm.x, 1.0f);

    // check matrix equality (don't do this for a matrix with arbitrary float values
    EXPECT_EQ(glm::mat4(1.f), m2);
    EXPECT_TRUE(true);
}

TEST(Environment, FLOAT_PRECISION) {
    std::cout << "Minimum value for int: " << std::numeric_limits<int>::min() << '\n';
    std::cout << "sizeof float: " << sizeof(float) << '\n';
    std::cout << "Minimum value for float: " << std::numeric_limits<float>::min() << '\n';
    std::cout << "sizeof double: " << sizeof(double) << '\n';
    std::cout << "Minimum value for double: " << std::numeric_limits<double>::min() << '\n';
    double d = 1.0;
    glm::mat4 m((float)d);
    std::cout << "sizeof matrix single element: " << sizeof(m[0].x) << '\n';

    // Vulkan is single precision, make sure of it:
    EXPECT_EQ(4, sizeof(float));
    EXPECT_EQ(8, sizeof(double));
    EXPECT_EQ(4, sizeof(m[0].x));
}

TEST(Environment, GLFW) {
    EXPECT_EQ(GLFW_TRUE, glfwInit());
    glfwTerminate();
}

TEST(Environment, Vulkan) {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " Vulkan extensions supported\n";
    // very rough validation: more than 10 extensions expected
    EXPECT_GT((int)extensionCount, 10);
}

TEST(Util, Logs) {
    // should log into /ShadedPathTest/spe_run.log
    LogFile("Hi, Shaded Path Engine\n");

    // log to debug console and append to spe_run.log (depending on LOGFILE defined or not):
    Log("Hi, Shaded Path Engine!\n");
}

TEST(Engine, Initialization) {
    {
        ShadedPathEngine engine;
    }
    Log("Test end. (Should appear after destructor log)\n");
    LogfileScanner log;
    EXPECT_GT(log.searchForLine("Engine destructor"), -1);
    EXPECT_GT(log.searchForLine("Test end."), -1);
    //Log(" found in logfile: " << log.searchForLine("Engine destructor") << endl);
    EXPECT_TRUE(log.assertLineBefore("Engine destructor", "Test end."));
}