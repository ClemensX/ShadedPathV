#include "mainheader.h"
#include "AppSupport.h"
#include "TextureViewer.h"

using namespace std;
using namespace glm;

void TextureViewer::run()
{
    Log("TextureViewer started" << endl);
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
        engine.setMaxTextures(10);
        setHighBackbufferResolution();
        int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Texture Viewer");
        camera->saveProjectionParams(glm::radians(45.0f), engine.getAspect(), 0.01f, 2000.0f);

        engine.registerApp(this);
        initEngine("TextureViewer");

        engine.textureStore.generateBRDFLUT();
        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)  // enable to render central cube with debug texture
            .addShader(shaders.billboardShader)
            //.addShader(shaders.lineShader)  // enable to see zero cross and billboard debug lines
            //.addShader(shaders.pbrShader)
            ;
        if (enableUI) shaders.addShader(shaders.uiShader);
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        eventLoop();
    }
    Log("TextureViewer ended" << endl);
}

void TextureViewer::addRandomBillboards(vector<BillboardDef>& billboards, World &world, unsigned int textureIndex, float aspectRatio) {
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

void TextureViewer::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);

    // load skybox cube texture
    //engine.textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine.textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    engine.textureStore.loadTexture("debug.ktx", "2dTexture");
    engine.textureStore.loadTexture("eucalyptus.ktx2", "tree");
    engine.textureStore.loadTexture("shadedpath_logo.ktx2", "logo");
    //engine.textureStore.loadTexture("height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT, TextureFlags::KEEP_DATA_BUFFER);
    engine.textureStore.loadTexture("heightbig.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT);
    unsigned int texIndexTree = engine.textureStore.getTexture("tree")->index;
    unsigned int texIndexLogo = engine.textureStore.getTexture("logo")->index;
    unsigned int texIndex = texIndexTree;
    unsigned int texIndexHeightmap = engine.textureStore.getTexture("heightmap")->index;
    shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);
    // add some lines:
    float aspectRatio = engine.getAspect();
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

    engine.shaders.billboardShader.add(billboards);

    // Grid with 1m squares, floor on -10m, ceiling on 372m
    Grid* grid = world.createWorldGrid(1.0f, 0.0f);
    //engine.shaders.lineShader.add(grid->lines);
    engine.shaders.lineShader.addFixedGlobalLines(lines);

    // select texture by uncommenting:
    engine.global.createCubeMapFrom2dTexture("2dTexture", "2dTextureCube");
    //engine.global.createCubeMapFrom2dTexture("Knife1", "2dTextureCube");
    //engine.global.createCubeMapFrom2dTexture("WaterBottle2", "2dTextureCube");
    //engine.global.createCubeMapFrom2dTexture(engine.textureStore.BRDFLUT_TEXTURE_ID, "2dTextureCube"); // doesn't work (missing mipmaps? format?)
    engine.shaders.cubeShader.setFarPlane(1.0f); // cube around center
    engine.shaders.cubeShader.setSkybox("2dTextureCube");

    //engine.shaders.lineShader.initialUpload();
    //engine.shaders.pbrShader.initialUpload();
    //engine.shaders.cubeShader.initialUpload();
    engine.shaders.billboardShader.initialUpload();
}

void TextureViewer::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void TextureViewer::updatePerFrame(ThreadResources& tr)
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
    //cubo.view = camera->getViewMatrixAtCameraPos();
    //cubo.view = lubo.view; // uncomment to have stationary cube, not centered at camera
    //cubo.proj = lubo.proj;
    //cubo2.view = lubo2.view; // uncomment to have stationary cube, not centered at camera
    //cubo2.proj = lubo2.proj;
    engine.shaders.cubeShader.uploadToGPU(tr, cubo, cubo2, true);
 
    // billboards
    BillboardShader::UniformBufferObject bubo{};
    BillboardShader::UniformBufferObject bubo2{};
    bubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    bubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(bubo.view, bubo.proj, bubo2.view, bubo2.proj);
    engine.shaders.billboardShader.uploadToGPU(tr, bubo, bubo2);
    //Util::printMatrix(bubo.proj);
}

void TextureViewer::handleInput(InputState& inputState)
{
    AppSupport::handleInput(inputState);
}