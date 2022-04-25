
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
        engine.setFrameCountLimit(10);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        //engine.enablePresentation(800, (int)(800 / 1.77f), "Vulkan Simple App");
        engine.setFramesInFlight(2);
        engine.setThreadModeSingle();

        // engine initialization
        engine.init("Test");
    }
    Log("Test end. (Should appear after destructor log)\n");
    LogfileScanner log;
    EXPECT_GT(log.searchForLine("Engine destructor"), -1);
    EXPECT_GT(log.searchForLine("Test end."), -1);
    //Log(" found in logfile: " << log.searchForLine("Engine destructor") << endl);
    EXPECT_TRUE(log.assertLineBefore("Engine destructor", "Test end."));
}

TEST(Engine, Headless) {
    {
        ShadedPathEngine engine;
        ShaderState shaderState;
        engine.files.findAssetFolder("data");
        engine.setFrameCountLimit(10);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        engine.setFramesInFlight(2);
        engine.setThreadModeSingle();
        engine.init("Test");

        engine.shaders.simpleShader.init(engine, shaderState);
        engine.prepareDrawing();
        engine.drawFrame();
    }
}

TEST(Timer, Average) {
    ThemedTimer::getInstance()->create("AVG", 10);
    auto td = ThemedTimer::getInstance()->test_add("AVG", 3);
    EXPECT_EQ(1, td->calls);
    EXPECT_EQ(3, td->averageTimeBetweenMicroS);
    ThemedTimer::getInstance()->test_add("AVG", 5);
    EXPECT_EQ(4, td->averageTimeBetweenMicroS);
    ThemedTimer::getInstance()->test_add("AVG", 7);
    EXPECT_EQ(5, td->averageTimeBetweenMicroS);
}

TEST(Timer, FPS) {
    ThemedTimer::getInstance()->create("FPS", 10);
    auto td = ThemedTimer::getInstance()->test_add("FPS");
    // first call does not count for FPS:
    EXPECT_EQ(0, td->calls);
    EXPECT_EQ(0, td->averageTimeBetweenMicroS);
    // sleep half a second
    this_thread::sleep_for(chrono::milliseconds(500));
    ThemedTimer::getInstance()->test_add("FPS");
    double fps = ThemedTimer::getInstance()->getFPS("FPS");
    // 2 time points .5s apart should be around 2 fps
    EXPECT_NEAR(2.0f, fps, 0.1f);

    this_thread::sleep_for(chrono::milliseconds(1500));
    ThemedTimer::getInstance()->test_add("FPS");
    fps = ThemedTimer::getInstance()->getFPS("FPS");
    // 3rd time point again 500ms apart
    EXPECT_NEAR(1.0f, fps, 0.1f);

    this_thread::sleep_for(chrono::milliseconds(100));
    ThemedTimer::getInstance()->test_add("FPS");
    this_thread::sleep_for(chrono::milliseconds(100));
    ThemedTimer::getInstance()->test_add("FPS");
    fps = ThemedTimer::getInstance()->getFPS("FPS");
    EXPECT_NEAR(1.82f, fps, 0.1f);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // enable single tests
    //::testing::GTEST_FLAG(filter) = "Environment.GLFW";
    //::testing::GTEST_FLAG(filter) = "Engine.Headless";
    // all test but excluded one:
    //::testing::GTEST_FLAG(filter) = "-Engine.Headless";
    return RUN_ALL_TESTS();
}