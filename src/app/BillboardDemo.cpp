#include "mainheader.h"
#include "AppSupport.h"
#include "BillboardDemo.h"

using namespace std;
using namespace glm;

void BillboardDemo::run(ContinuationInfo* cont)
{
    Log("BillboardDemo started" << endl);
    {
        AppSupport::setEngine(engine);
        Shaders& shaders = engine->shaders;
        // camera initialization
        initCamera(glm::vec3(0.0f, 0.0f, 1.2f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        setMaxSpeed(5.0f);

        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.01f, 2000.0f);

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)  // enable to render central cube with debug texture
            .addShader(shaders.billboardShader)
            .addShader(shaders.lineShader)  // enable to see zero cross and billboard debug lines
            //.addShader(shaders.pbrShader)
            ;
        //if (enableUI) shaders.addShader(shaders.uiShader);
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("BillboardDemo ended" << endl);
}

void BillboardDemo::addRandomBillboards(vector<BillboardDef>& billboards, World &world, unsigned int textureIndex, float aspectRatio) {
    BillboardDef b;
    b.pos = vec4(0.0f, 4.05f, 0.0f, 0);
    b.dir = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    b.w = b.h = 10.0f;
    b.w = b.h / aspectRatio;
    b.type = 0;
    b.textureIndex = textureIndex;
    //unsigned long total_billboards = 50000000; // close to 4GB on GPU
    //unsigned long total_billboards = 1000000;
    //unsigned long total_billboards = 500000;
    unsigned long total_billboards = 200000;
    //unsigned long total_billboards = 5000;
    //unsigned long total_billboards = 12;
    unsigned long billboards_per_texture = total_billboards / 12;

    // create randomly positioned billboards for each vacXX texture we have:
    for (unsigned long num = 0; num < total_billboards; num++) {
        vec3 rnd = world.getRandomPos();
        b.pos.x = rnd.x;
        //b.pos.y = rnd.y;
        b.pos.z = rnd.z;
        billboards.push_back(b);
    }
}

void BillboardDemo::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    //world.setWorldSize(10.0f, 382.0f, 10.0f);

    // load skybox cube texture
    //engine->textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine->textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    engine->textureStore.loadTexture("debug.ktx", "2dTexture");
    engine->textureStore.loadTexture("eucalyptus.ktx2", "tree");
    engine->textureStore.loadTexture("shadedpath_logo.ktx2", "logo");
    //engine->textureStore.loadTexture("height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT, TextureFlags::KEEP_DATA_BUFFER);
    engine->textureStore.loadTexture("heightbig.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT);
    unsigned int texIndexTree = engine->textureStore.getTexture("tree")->index;
    unsigned int texIndexLogo = engine->textureStore.getTexture("logo")->index;
    unsigned int texIndex = texIndexTree;
    unsigned int texIndexHeightmap = engine->textureStore.getTexture("heightmap")->index;
    engine->shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);
    // add some lines:
    float aspectRatio = engine->getAspect();
    float plus = 0.0f;
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) }
    };
    vector<LineDef> lines;
    //scale tree height to 10m
    float height = 10.0f;
    float width = height / aspectRatio;
    BillboardDef myBillboards[] = {
        { vec4(-0.1f, 4.05f, -0.1f, 1.0f), // pos
          vec4(0.0f, 0.0f, 1.0f, 0.0f), // dir
          width, // w
          height, // h
          1,    // type
          texIndex
        },
        { vec4(-0.2f, 0.2f, 0.0f, 1.0f), // pos
          vec4(0.3f, 0.1f, 0.0f, 0.0f), // dir
          0.3f, // w
          0.9f, // h
          0,    // type
          texIndex
        }
    };
    vector<BillboardDef> billboards;
    //for_each(begin(myBillboards), end(myBillboards), [&billboards](BillboardDef l) {billboards.push_back(l); });
    addRandomBillboards(billboards, world, texIndex, aspectRatio);

    engine->shaders.billboardShader.add(billboards);

    // Grid with 1m squares, floor on -10m, ceiling on 372m
    Grid* grid = world.createWorldGrid(1.0f, 0.0f);
    //engine->shaders.lineShader.add(grid->lines);
    engine->shaders.lineShader.addFixedGlobalLines(lines);

    // select texture by uncommenting:
    engine->globalRendering.createCubeMapFrom2dTexture("2dTexture", "2dTextureCube");
    //engine->globalRendering.createCubeMapFrom2dTexture("Knife1", "2dTextureCube");
    //engine->globalRendering.createCubeMapFrom2dTexture("WaterBottle2", "2dTextureCube");
    //engine->globalRendering.createCubeMapFrom2dTexture(engine->textureStore.BRDFLUT_TEXTURE_ID, "2dTextureCube"); // doesn't work (missing mipmaps? format?)
    engine->shaders.cubeShader.setFarPlane(1.0f); // cube around center
    engine->shaders.cubeShader.setSkybox("2dTextureCube");

    //engine->shaders.lineShader.initialUpload();
    //engine->shaders.pbrShader.initialUpload();
    //engine->shaders.cubeShader.initialUpload();
    engine->shaders.billboardShader.initialUpload();

    prepareWindowOutput("Billboard Demo");
}

void BillboardDemo::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void BillboardDemo::prepareFrame(FrameResources* fr)
{
    FrameResources& tr = *fr;
    double seconds = engine->gameTime.getTimeSeconds();
    if ((old_seconds > 0.0f && old_seconds == seconds) || old_seconds > seconds) {
        Error("APP TIME ERROR - should not happen");
        return;
    }
    double deltaSeconds = seconds - old_seconds;

    updateCameraPositioners(deltaSeconds);
    old_seconds = seconds;

    // lines
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    // we still need to call prepareAddLines() even if we didn't actually add some
    engine->shaders.lineShader.prepareAddLines(tr);
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    // cube
    CubeShader::UniformBufferObject cubo{};
    CubeShader::UniformBufferObject cubo2{};
    cubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    cubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(cubo.view, cubo.proj, cubo2.view, cubo2.proj);
    //cubo.view = camera->getViewMatrixAtCameraPos();
    //cubo.view = lubo.view; // uncomment to have stationary cube, not centered at camera
    //cubo.proj = lubo.proj;
    //cubo2.view = lubo2.view; // uncomment to have stationary cube, not centered at camera
    //cubo2.proj = lubo2.proj;
    engine->shaders.cubeShader.uploadToGPU(tr, cubo, cubo2, true);
 
    // billboards
    BillboardShader::UniformBufferObject bubo{};
    BillboardShader::UniformBufferObject bubo2{};
    bubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    bubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(bubo.view, bubo.proj, bubo2.view, bubo2.proj);
    engine->shaders.billboardShader.uploadToGPU(tr, bubo, bubo2);
    //Util::printMatrix(bubo.proj);

    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void BillboardDemo::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
        engine->shaders.cubeShader.addCommandBuffers(fr, drawResult);
    }
    else if (topic == 1) {
        engine->shaders.billboardShader.addCommandBuffers(fr, drawResult);
    }
}

void BillboardDemo::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void BillboardDemo::processImage(FrameResources* fr)
{
    present(fr);
}

bool BillboardDemo::shouldClose()
{
    return shouldStopEngine;
}

void BillboardDemo::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}
