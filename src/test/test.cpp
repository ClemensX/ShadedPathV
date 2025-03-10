
#include "mainheader.h"
#include "test.h"
#include <gtest/gtest.h>


using namespace std;
using namespace glm;

// we need to change working directory for all non-trivial engine tests.
// Especially for tests that check log files.
// each test runs in a sub folder with it's name, e.g. build\src\test\Debug\Logs"
class WorkingDirectoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        const ::testing::TestInfo* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string test_name = test_info->name();
        original_path = std::filesystem::current_path();
        test_directory = test_name;
        if (!std::filesystem::exists(test_directory)) {
            std::filesystem::create_directories(test_directory);
        }
        std::filesystem::current_path(test_directory);
    }

    void TearDown() override {
        std::filesystem::current_path(original_path);
    }

private:
    std::filesystem::path original_path;
    std::filesystem::path test_directory;
};

class UtilTest : public WorkingDirectoryTest {};
class EngineTest : public WorkingDirectoryTest {};
class EngineImageConsumer : public WorkingDirectoryTest {};

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

TEST_F(UtilTest, Logs) {
    // should log into /ShadedPathTest/spe_run.log
    LogFile("Hi, Shaded Path Engine\n");

    // log to debug console and append to spe_run.log (depending on LOGFILE defined or not):
    Log("Hi, Shaded Path Engine!\n");
}

TEST_F(EngineTest, Initialization) {
    {
        ShadedPathEngine engine;
        //engine.setFrameCountLimit(10);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        //engine.setFramesInFlight(2);
        engine.setSingleThreadMode(true);

        // engine initialization
        engine.initGlobal();
        engine.createImage("debugName");
        //engine.init("Test");
    }
    Log("Test end. (Should appear after destructor log)\n");
    LogfileScanner log;
    EXPECT_GT(log.searchForLine("Engine destructor"), -1);
    EXPECT_GT(log.searchForLine("Test end."), -1);
    //Log(" found in logfile: " << log.searchForLine("Engine destructor") << endl);
    EXPECT_TRUE(log.assertLineBefore("Engine destructor", "Test end."));
}

TEST_F(EngineTest, DirectImageManipulation) {
    {
        ShadedPathEngine engine;
        //engine.setFrameCountLimit(10);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        //engine.setFramesInFlight(2);
        engine.setSingleThreadMode(true);

        // engine initialization
        engine.initGlobal();
        auto gpui = engine.createImage("Test Image");
        DirectImage di(&engine);
        GPUImage directImage;
        engine.globalRendering.createDumpImage(directImage);
        di.openForCPUWriteAccess(gpui, &directImage);
        engine.util.writeRawImageTestData(directImage, 0);
        di.closeCPUWriteAccess(gpui, &directImage);
        di.dumpToFile(gpui);

        di.openForCPUWriteAccess(gpui, &directImage);
        engine.util.writeRawImageTestData(directImage, 1);
        di.closeCPUWriteAccess(gpui, &directImage);
        di.dumpToFile(gpui);

        engine.globalRendering.destroyImage(&directImage);
    }
    Log("Test end. (Should appear after destructor log)\n");
    LogfileScanner log;
    EXPECT_GT(log.searchForLine("Engine destructor"), -1);
    EXPECT_GT(log.searchForLine("Test end."), -1);
    //Log(" found in logfile: " << log.searchForLine("Engine destructor") << endl);
    EXPECT_TRUE(log.assertLineBefore("Engine destructor", "Test end."));
}

TEST_F(EngineTest, Headless) {
    {
        ShadedPathEngine my_engine;
        static ShadedPathEngine* engine = &my_engine;
        engine->initGlobal();
        class TestApp : public ShadedPathApplication
        {
        public:
            void prepareFrame(FrameResources* fi) override {
                Log("prepareFrame " << fi->frameNum << endl);
                if (fi->frameNum >= 10) {
                    shouldStopEngine = true;
                }
                lastFrameNum = fi->frameNum;
            };
            void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) override {
                Log("drawFrame " << fi->frameNum << endl);
                directImage.rendered = false;
                engine->util.writeRawImageTestData(directImage, 0);
                directImage.rendered = true;
                fi->renderedImage = &directImage;
            };
            void run(ContinuationInfo* cont) override {
                Log("TestApp started\n");
                Log(" run thread: ");
                engine->log_current_thread();
                di.setEngine(engine);
                gpui = engine->createImage("Test Image");
                engine->globalRendering.createDumpImage(directImage);
                di.openForCPUWriteAccess(gpui, &directImage);
                engine->shaders.initActiveShaders();

                engine->eventLoop();

                // cleanup
                di.closeCPUWriteAccess(gpui, &directImage);
                engine->globalRendering.destroyImage(&directImage);

            };
            bool shouldClose() override {
                return shouldStopEngine;
            }
            long lastFrameNum = 0;
        private:
            bool shouldStopEngine = false;
            DirectImage di;
            GPUImage* gpui = nullptr;
            GPUImage directImage;

        //    void drawFrame(ThreadResources& tr) override {
        //        engine->shaders.submitFrame(tr);
        //    };
        //    void handleInput(InputState& inputState) override {
        //    };
        };
        TestApp testApp;
        //ShaderState shaderState;
        //engine->files.findAssetFolder("data");
        //engine->setFrameCountLimit(10);
        //engine->setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        //engine->setFramesInFlight(2);
        //engine->setThreadModeSingle();
        engine->registerApp((ShadedPathApplication*)&testApp);
        engine->app->run();
        //engine->init("Test");
        //engine->textureStore.generateBRDFLUT();
        //engine->shaders.addShader(engine->shaders.simpleShader);
        //engine->shaders.initActiveShaders();

        //engine->prepareDrawing();
        //engine->drawFrame();
        EXPECT_TRUE(engine->app);
        EXPECT_EQ(10, testApp.lastFrameNum);
    }
    Log("Test end. (Should appear after destructor log)\n");
}

TEST_F(EngineTest, DumpTexture) {
    {
        //ShadedPathEngineManager man;
        //static ShadedPathEngine* engine = man.createEngine();
        //class TestApp : ShadedPathApplication
        //{
        //public:
        //    void drawFrame(ThreadResources& tr) override {
        //        engine->shaders.submitFrame(tr);
        //    };
        //    void handleInput(InputState& inputState) override {
        //    };
        //};
        //TestApp testApp;
        //ShaderState shaderState;
        //engine->files.findAssetFolder("data");
        //engine->setFrameCountLimit(10);
        //engine->setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        //engine->setFramesInFlight(2);
        //engine->setThreadModeSingle();
        //engine->registerApp((ShadedPathApplication*)&testApp);
        //engine->init("Test");
        //engine->textureStore.generateBRDFLUT();
        //engine->textureStore.loadTexture("height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT, TextureFlags::KEEP_DATA_BUFFER);
        //unsigned int texIndexHeightmap = engine->textureStore.getTexture("heightmap")->index;
        //engine->shaders.addShader(engine->shaders.simpleShader);
        //engine->shaders.initActiveShaders();

        //engine->prepareDrawing();
        //engine->drawFrame();
    }
    Log("Test end. (Should appear after destructor log)\n");
}

TEST_F(EngineTest, Alignment) {
    {
        //ShadedPathEngineManager man;
        //ShadedPathEngine* engine = man.createEngine();
        //engine->setFrameCountLimit(10);
        //engine->setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        ////engine->enablePresentation(800, (int)(800 / 1.77f), "Vulkan Simple App");
        //engine->setFramesInFlight(2);
        //engine->setThreadModeSingle();

        //// engine initialization
        //engine->init("Test");

        //// test alignment method:
        //EXPECT_EQ(64, engine->global.calcConstantBufferSize(1));
        //EXPECT_EQ(64, engine->global.calcConstantBufferSize(2));
        //EXPECT_EQ(256, engine->global.calcConstantBufferSize(255));
        //EXPECT_EQ(256, engine->global.calcConstantBufferSize(256));
        //EXPECT_EQ(320, engine->global.calcConstantBufferSize(257));
        //// 120*64 == 7680
        //EXPECT_EQ(7424, engine->global.calcConstantBufferSize(7423));
        //EXPECT_EQ(7680, engine->global.calcConstantBufferSize(7679));
        //EXPECT_EQ(7680, engine->global.calcConstantBufferSize(7680));
        //EXPECT_EQ(7744, engine->global.calcConstantBufferSize(7681));
        //EXPECT_EQ(7808, engine->global.calcConstantBufferSize(7781));
        //EXPECT_EQ(7936, engine->global.calcConstantBufferSize(7881));
    }
    Log("Test end. (Should appear after destructor log)\n");
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

TEST(Spatial, Heightmap) {
    int heightmap_points_side = 1024 + 1;
    int heightmap_segments_side = heightmap_points_side - 1;
    // create heigthmap with heightmap_points_side points on each side:
    ASSERT_NO_THROW(Spatial2D heightmap(heightmap_points_side));
    ASSERT_THROW(Spatial2D heightmap(heightmap_segments_side), std::out_of_range);
    ASSERT_THROW(Spatial2D heightmap(heightmap_points_side-4), std::out_of_range);
    Spatial2D heightmap0(heightmap_points_side);
    EXPECT_EQ(heightmap_points_side * heightmap_points_side, heightmap0.size());
    // set height of three points at center
    int half = heightmap_segments_side / 2;
    heightmap0.setHeight(half, half, 0.0f);
    heightmap0.setHeight(half - 1, half, 0.2f);
    heightmap0.setHeight(half, half - 1, 0.3f);
    EXPECT_FALSE(heightmap0.isAllPointsSet());
    vector<LineDef> lines;
    heightmap0.getLines(lines);
    ASSERT_EQ(2, lines.size());

    Spatial2D heightmap(heightmap_points_side);
    int lastPos = heightmap_segments_side;
    // down left and right corner
    heightmap.setHeight(0, 0, 0.0f);
    heightmap.setHeight(lastPos, 0, 100.0f);
    // top left and right corner
    heightmap.setHeight(0, lastPos, 10.0f);
    heightmap.setHeight(lastPos, lastPos, 50.0f);
    // do one iteration
    heightmap.diamondSquare(0.0f, 0.0f, 0);
    lines.clear();
    heightmap.getLines(lines);
    EXPECT_EQ(4, lines.size());
    // all steps:
    heightmap.diamondSquare(10.0f, 0.99f);
    vector<vec3> plist;
    heightmap.getPoints(plist);
    // we should have a height point in all line/row crossings
    //EXPECT_EQ((4096 * 2 + 1)*(4096 * 2 + 1), plist.size());
    EXPECT_TRUE(heightmap.isAllPointsSet());
}

TEST(Threads, BasicTasks) {
    ThreadGroup threadGroup(4); // Create a thread pool with 4 threads

    auto future1 = threadGroup.asyncSubmit([] {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Log("Task 1 completed" << std::endl);
        });

    auto future2 = threadGroup.asyncSubmit([] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        Log("Task 2 completed" << std::endl);
        });

    future1.get();
    future2.get();
    Log("Active threads: " << threadGroup.getActiveThreadCount() << std::endl);
    EXPECT_EQ(1, 1);
}

TEST_F(EngineImageConsumer, Dump) {
    {
        ShadedPathEngine my_engine;
        my_engine
            .setEnableLines(true)
            .setDebugWindowPosition(true)
            .setEnableUI(true)
            .setEnableSound(true)
            .setVR(false)
            //.setSingleThreadMode(true)
            .overrideCPUCores(4)
            .configureParallelAppDrawCalls(2)
        ;

        static ShadedPathEngine* engine = &my_engine;
        engine->initGlobal();
        class TestApp : public ShadedPathApplication
        {
        public:
            void prepareFrame(FrameResources* fi) override {
                if (fi->frameNum >= 10) {
                    shouldStopEngine = true;
                }
            };
            // drawFrame is called for each topic in parallel!! Beware!
            void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) override {
                if (topic == 0) {
                    directImage.rendered = false;
                    engine->util.writeRawImageTestData(directImage, 0);
                    directImage.rendered = true;
                    fi->renderedImage = &directImage;
                }
            };
            void run(ContinuationInfo* cont) override {
                di.setEngine(engine);
                gpui = engine->createImage("Test Image");
                engine->globalRendering.createDumpImage(directImage);
                di.openForCPUWriteAccess(gpui, &directImage);

                engine->shaders.initActiveShaders();
                engine->eventLoop();

                // cleanup
                di.closeCPUWriteAccess(gpui, &directImage);
                engine->globalRendering.destroyImage(&directImage);
            };
            bool shouldClose() override {
                return shouldStopEngine;
            }
        private:
            bool shouldStopEngine = false;
            DirectImage di;
            GPUImage* gpui = nullptr;
            GPUImage directImage;

        };
        TestApp testApp;
        engine->registerApp((ShadedPathApplication*)&testApp);
        ImageConsumerDump imageConsumerDump(engine);
        imageConsumerDump.configureFramesToDump(false, {4L, 7L});
        engine->setImageConsumer(&imageConsumerDump);
        engine->app->run();
        EXPECT_TRUE(engine->app);
    }
    Log("Test end. (Should appear after destructor log)\n");
    LogfileScanner log;
    // we should not get warnings about missing image consumer as we have set one:
    EXPECT_EQ(log.searchForLine("WARNING: No image consumer set, defaulting to discarding image"), -1);
    // TODO: check that dumping and drawing is done in parallel
    // TODO: check for dumped images
}

TEST(CommandBufferIterator, Count) {
    FrameResources fi;
    //for (auto& commandBuffer : fi) {
    //    // should not enter here
    //    EXPECT_TRUE(false);
    //}
    // simple cases
    EXPECT_EQ(0, fi.countCommandBuffers());
    DrawResult dr1, dr2;

    // init all command buffers to nullptr
    for (auto& commandBuffer : dr1.commandBuffers) {
        commandBuffer = nullptr;
    }
    for (auto& commandBuffer : dr2.commandBuffers) {
        commandBuffer = nullptr;
    }

    // 1 draw result with 1 command buffer
    dr1.commandBuffers[0] = (VkCommandBuffer)0xcaffee;
    fi.drawResults.push_back(dr1);
    EXPECT_EQ(1, fi.countCommandBuffers());
    fi.drawResults.pop_back(); // remove dr1

    // 1 draw result with 2 command buffers
    dr1.commandBuffers[1] = (VkCommandBuffer)0xcaffee;
    fi.drawResults.push_back(dr1);
    EXPECT_EQ(2, fi.countCommandBuffers());
    fi.drawResults.pop_back(); // remove dr1

    // 1 draw result with 3 command buffers, but with nullptr in between
    // nullptr means and of list and should stop iterating
    dr1.commandBuffers[3] = (VkCommandBuffer)0xcaffee;
    fi.drawResults.push_back(dr1);
    EXPECT_EQ(2, fi.countCommandBuffers());
    fi.drawResults.pop_back(); // remove dr1

    // 2 draw result with 3 command buffers each, but first list has nullptr in between
    fi.drawResults.push_back(dr1);
    dr2.commandBuffers[0] = (VkCommandBuffer)0x0042;
    dr2.commandBuffers[1] = (VkCommandBuffer)0x0042;
    dr2.commandBuffers[2] = (VkCommandBuffer)0x0042;
    fi.drawResults.push_back(dr2);
    EXPECT_EQ(5, fi.countCommandBuffers());
    fi.drawResults.pop_back(); // remove dr2
    fi.drawResults.pop_back(); // remove dr1

    // multiple draw results
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // enable single tests
    //::testing::GTEST_FLAG(filter) = "Environment.GLFW";
    //::testing::GTEST_FLAG(filter) = "engine->Headless";
    // all test but excluded one:
    //::testing::GTEST_FLAG(filter) = "-engine->Headless";
    
    return RUN_ALL_TESTS();
}