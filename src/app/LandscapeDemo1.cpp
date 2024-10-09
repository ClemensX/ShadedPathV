#include "mainheader.h"
#include "AppSupport.h"
#include "LandscapeDemo1.h"

using namespace std;
using namespace glm;

void LandscapeDemo::run()
{
    Log("LandscapeDemo started" << endl);
    {
        setEngine(engine);
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(0.0f, 0.0f, 1.2f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        createHMDCameraPositioner(glm::vec3(0.0f, 0.0f, 1.2f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(5.0f);
        initCamera();
        // engine configuration
        enableEventsAndModes();
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(20);
        //engine.setFrameCountLimit(1000);
        setHighBackbufferResolution();
        int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Landscape Demo");
        //camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.01f, 4300.0f));
        camera->saveProjectionParams(glm::radians(45.0f), engine.getAspect(), 0.01f, 4300.0f);

        engine.registerApp(this);
        initEngine("LandscapeDemo");

        engine.textureStore.generateBRDFLUT();
        // add shaders used in this app
        shaders
            .addShader(shaders.uiShader)
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)  // enable to render central cube with debug texture
            .addShader(shaders.billboardShader)
            //.addShader(shaders.terrainShader)
            .addShader(shaders.lineShader)  // enable to see zero cross and billboard debug lines
            .addShader(shaders.pbrShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();
        shaders.initiateShader_BackBufferImageDump(false); // enable image dumps upon request


        // init app rendering:
        init();
        eventLoop();
    }
    Log("LandscapeDemo ended" << endl);
}

void LandscapeDemo::addRandomBillboards(vector<BillboardDef>& billboards, World &world, unsigned int textureIndex, float aspectRatio) {
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

    // create randomly positioned billboards with certain min distance to origin:
    for (unsigned long num = 0; num < total_billboards; num++) {
        vec3 rnd;
        float dist;
        do {
            rnd = world.getRandomPos();
            dist = sqrt(rnd.x * rnd.x + rnd.z * rnd.z);
        } while (dist < 100.0f);
        b.pos.x = rnd.x;
        //b.pos.y = rnd.y;
        b.pos.z = rnd.z;
        billboards.push_back(b);
    }
}

void LandscapeDemo::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);

    //world.setWorldSize(10.0f, 382.0f, 10.0f);

    engine.textureStore.loadTexture("heightbig.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT);
    //engine.textureStore.loadTexture("height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT, TextureFlags::KEEP_DATA_BUFFER);
    // load skybox cube texture
    engine.textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine.textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    engine.textureStore.loadTexture("debug.ktx", "2dTexture");
    engine.textureStore.loadTexture("eucalyptus.ktx2", "tree");
    engine.textureStore.loadTexture("shadedpath_logo.ktx2", "logo");
    unsigned int texIndexTree = engine.textureStore.getTexture("tree")->index;
    unsigned int texIndexLogo = engine.textureStore.getTexture("logo")->index;
    unsigned int texIndexHeightmap = engine.textureStore.getTexture("heightmap")->index;
    shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);
    // set texture index for billboards
    unsigned int texIndex = texIndexTree;
    //unsigned int texIndex = texIndexHeightmap;
    // add some lines:
    float aspectRatio = engine.getAspect();

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
    addRandomBillboards(billboards, world, texIndex, aspectRatio);

    engine.shaders.billboardShader.add(billboards);

    // Grid with 1m squares, floor on -10m, ceiling on 372m
    //Grid* grid = world.createWorldGrid(1.0f, 0.0f);
    //engine.shaders.lineShader.add(grid->lines);
    Spatial2D heightmap(1024 * 2 + 1);
    // set height of three points at center
    //heightmap.setHeight(4096, 4096, 0.0f);
    //heightmap.setHeight(4095, 4096, 0.2f);
    //heightmap.setHeight(4096, 4095, 0.3f);
    int lastPos = 1024 * 2;
    // down left and right corner
    heightmap.setHeight(0, 0, 0.0f);
    heightmap.setHeight(lastPos, 0, 300.0f);
    // top left and right corner
    heightmap.setHeight(0, lastPos, 10.0f);
    heightmap.setHeight(lastPos, lastPos, 50.0f);
    // do two iteration
    heightmap.diamondSquare(200.0f, 0.5f);
    vector<LineDef> lines;
    heightmap.getLines(lines);
    heightmap.adaptLinesToWorld(lines, world);
    //vector<vec3> plist;
    //heightmap.getPoints(plist);
    //Log("num points: " << plist.size() << endl);
    engine.shaders.lineShader.addFixedGlobalLines(lines);

    // select texture by uncommenting:
    if (isSkybox) {
        engine.shaders.cubeShader.setSkybox("skyboxTexture");
        engine.shaders.cubeShader.setFarPlane(2000.0f);
    } else {
        engine.global.createCubeMapFrom2dTexture("2dTexture", "2dTextureCube");
        engine.shaders.cubeShader.setFarPlane(1.0f); // cube around center
        engine.shaders.cubeShader.setSkybox("2dTextureCube");
    }


    //engine.shaders.lineShader.initialUpload();
    //engine.shaders.pbrShader.initialUpload();
    //engine.shaders.cubeShader.initialUpload();
    engine.shaders.billboardShader.initialUpload();
}

void LandscapeDemo::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void LandscapeDemo::updatePerFrame(ThreadResources& tr)
{
    static double old_seconds = 0.0f;
    double seconds = engine.gameTime.getTimeSeconds();
    if (old_seconds > 0.0f && old_seconds == seconds) {
        Log("DOUBLE TIME" << endl);
        return;
    }
    if (old_seconds > seconds) {
        Log("INVERTED TIME" << endl);
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
    engine.shaders.lineShader.prepareAddLines(tr);
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
    engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    // cube
    CubeShader::UniformBufferObject cubo{};
    CubeShader::UniformBufferObject cubo2{};
    cubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    cubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(cubo.view, cubo.proj, cubo2.view, cubo2.proj);
    if (isSkybox) {
        cubo.view = camera->getViewMatrixAtCameraPos();
        cubo2.view = camera->getViewMatrixAtCameraPos();
    }
    // reset view matrix to camera orientation without using camera position (prevent camera movin out of skybox)
    engine.shaders.cubeShader.uploadToGPU(tr, cubo, cubo2);

    // billboards
    BillboardShader::UniformBufferObject bubo{};
    BillboardShader::UniformBufferObject bubo2{};
    bubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    bubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(bubo.view, bubo.proj, bubo2.view, bubo2.proj);
    engine.shaders.billboardShader.uploadToGPU(tr, bubo, bubo2);
    //Util::printMatrix(bubo.proj);

    postUpdatePerFrame(tr);
}

void LandscapeDemo::handleInput(InputState& inputState)
{
    AppSupport::handleInput(inputState);
}