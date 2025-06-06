#include "mainheader.h"
#include "AppSupport.h"
#include "GeneratedTexturesApp.h"

using namespace std;
using namespace glm;

void GeneratedTexturesApp::run(ContinuationInfo* cont)
{
    Log("Generated Textures App started" << endl);
    {
        AppSupport::setEngine(engine);
        Shaders& shaders = engine->shaders;
        // camera initialization
        initCamera(glm::vec3(0.0f, 0.0f, 1.2f), glm::vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(0.1f);

        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.01f, 2000.0f);

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            //.addShader(shaders.cubeShader)  // enable to render central cube with debug texture
            .addShader(shaders.billboardShader)
            .addShader(shaders.pbrShader)
            ;
        if (enableLines) shaders.addShader(shaders.lineShader);
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("Generated Textures App ended" << endl);
}


void createDebugLines(vector<LineDef>& lines, vector<BillboardDef>& billboards) {

    for (auto& b : billboards) {
        if (b.type == 1) {
            vector<vec3> verts;
            BillboardShader::calcVertsOrigin(verts);
            // draw triangle with every 3 verts:
            for (size_t i = 0; i < verts.size(); i += 3) {
                vec3 v0 = verts[i];
                vec3 v1 = verts[i+1];
                vec3 v2 = verts[i+2];
                LineDef l;
                l.color = Colors::Silver;
                l.start = v0 + vec3(b.pos);
                l.end = v1 + vec3(b.pos);
                lines.push_back(l);
                l.start = v1 + vec3(b.pos);
                l.end = v2 + vec3(b.pos);
                lines.push_back(l);
                l.start = v2 + vec3(b.pos);
                l.end = v0 + vec3(b.pos);
                lines.push_back(l);
                // add rotation direction:
                l.color = Colors::Cyan;
                l.start = vec3(0, 0, 0) + vec3(b.pos);
                // add with unit length
                l.end = l.start + normalize(vec3(b.dir));
                lines.push_back(l);

                // now the rotated vertices:
                glm::vec3 defaultDir(0.0f, 0.0f, 1.0f); // towards positive z
                glm::vec3 toDir(b.dir);
                glm::quat q = MathHelper::RotationBetweenVectors(defaultDir, toDir);
                v0 = q * v0;
                v1 = q * v1;
                v2 = q * v2;
                //vec4 q_vec = vec4(q.x, q.y, q.z, q.w);
                //vec4 q_vec = vec4(q.w, q.x, q.y, q.z);
                //vec4 v0_vec = vec4(v0.x, v0.y, v0.z, 0.0f);
                //vec4 v0_temp = q_vec * v0_vec;
                //vec3 v0_via_vec = vec3(v0_temp);
                //assert(glm::epsilonEqual(v0.x, v0_via_vec.x, 0.001f));
                //assert(glm::epsilonEqual(v0.y, v0_via_vec.y, 0.001f));
                //assert(glm::epsilonEqual(v0.z, v0_via_vec.z, 0.001f));
                l.color = Colors::Yellow;
                l.start = v0 + vec3(b.pos);
                l.end = v1 + vec3(b.pos);
                Log("v1 in app: " << l.end.x << " " << l.end.y << " " << l.end.z << endl);
                lines.push_back(l);
                l.start = v1 + vec3(b.pos);
                l.end = v2 + vec3(b.pos);
                lines.push_back(l);
                l.start = v2 + vec3(b.pos);
                l.end = v0 + vec3(b.pos);
                lines.push_back(l);
            }
        }
    }

}

void GeneratedTexturesApp::init() {
    // load skybox cube texture
    //engine->textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine->textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    engine->textureStore.generateBRDFLUT();
    engine->textureStore.loadTexture("debug.ktx", "2dTexture");
    engine->textureStore.loadTexture("eucalyptus.ktx2", "tree");
    unsigned int texIndex = engine->textureStore.getTexture("tree")->index;
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
    BillboardDef myBillboards[] = {
        { vec4(-0.5f, 0.1f, -0.5f, 1.0f), // pos
          vec4(0.1f, 0.1f, 0.1f, 0.0f), // dir
          0.5f, // w
          0.6f, // h
          0,    // type
          texIndex
        },
        { vec4(-0.2f, 0.2f, 0.0f, 1.0f), // pos
          vec4(0.3f, 0.1f, 0.0f, 0.0f), // dir
          0.3f, // w
          0.9f, // h
          1,    // type
          texIndex
        }
    };
    vector<BillboardDef> billboards;
    for_each(begin(myBillboards), end(myBillboards), [&billboards](BillboardDef l) {billboards.push_back(l); });
    createDebugLines(lines, billboards);

    engine->shaders.billboardShader.add(billboards);
    // loading objects wireframe:
    //engine->objectStore.loadObject("WaterBottle.glb", "WaterBottle", lines);
    //engine->objectStore.loadMeshWireframe("small_knife_dagger/scene.gltf", "Knife", lines);

    // loading objects:
    //engine->meshStore.loadMesh("WaterBottle.glb", "WaterBottle");
    engine->meshStore.loadMesh("bottle2.glb", "WaterBottle");
    engine->meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
    //auto o = engine->meshStore.getMesh("Knife");
    // add bottle and knife to the scene:
    engine->objectStore.createGroup("bottle_group");
    engine->objectStore.addObject("bottle_group", "WaterBottle", vec3(0.0f, 0.0f, 0.0f));
    engine->objectStore.createGroup("knife_group");
    engine->objectStore.addObject("knife_group", "Knife", vec3(0.3f, 0.0f, 0.0f));
    //engine->objectStore.addObject("knife_group", "WaterBottle", vec3(0.3f, 0.0f, 0.0f));
    //Log("Object loaded: " << o->id.c_str() << endl);

    engine->textureStore.loadTexture("height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT, TextureFlags::KEEP_DATA_BUFFER);
    unsigned int texIndexHeightmap = engine->textureStore.getTexture("heightmap")->index;
    engine->shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);


    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    LineShader::addZeroCross(lines);
    //LineShader::addCross(lines, vec3(1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f));

    engine->shaders.lineShader.addFixedGlobalLines(lines);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    engine->setWorld(&world);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    // select texture by uncommenting:
    engine->globalRendering.createCubeMapFrom2dTexture("2dTexture", "2dTextureCube");
    //engine->global.createCubeMapFrom2dTexture("Knife1", "2dTextureCube");
    //engine->global.createCubeMapFrom2dTexture("WaterBottle2", "2dTextureCube");
    //engine->global.createCubeMapFrom2dTexture(engine->textureStore.BRDFLUT_TEXTURE_ID, "2dTextureCube"); // doesn't work (missing mipmaps? format?)
    //engine->shaders.cubeShader.setFarPlane(1.0f); // cube around center
    //engine->shaders.cubeShader.setSkybox("2dTextureCube");

    //engine->shaders.lineShader.initialUpload();
    engine->shaders.pbrShader.initialUpload();
    //engine->shaders.cubeShader.initialUpload();
    engine->shaders.billboardShader.initialUpload();
    // last thing in init() should always be texture description creation

    prepareWindowOutput("View Textures");
    engine->presentation.startUI();
}

void GeneratedTexturesApp::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void GeneratedTexturesApp::prepareFrame(FrameResources* fr)
{
    FrameResources& tr = *fr;
    double seconds = engine->gameTime.getTimeSeconds();
    if ((old_seconds > 0.0f && old_seconds == seconds) || old_seconds > seconds) {
        Error("APP TIME ERROR - should not happen");
        return;
    }
    double deltaSeconds = seconds - old_seconds;

    //positioner->update(deltaSeconds, input.pos, input.pressedLeft);
    old_seconds = seconds;
    updateCameraPositioners(deltaSeconds);

    // lines
    LineShader::UniformBufferObject lubo{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo.view = camera->getViewMatrix();
    lubo.proj = *getProjection();

    // TODO hack 2nd view
    mat4 v2 = translate(lubo.view, vec3(0.3f, 0.0f, 0.0f));
    auto lubo2 = lubo;
    lubo2.view = v2;

    // dynamic lines:
    engine->shaders.lineShader.clearLocalLines(tr);
    float aspectRatio = engine->getAspect();
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) }
    };
    plus += 0.001f;
    vector<LineDef> lines;
    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    engine->shaders.lineShader.addOneTime(lines, tr);

    engine->shaders.lineShader.prepareAddLines(tr);
    //Log("lines proj" << endl);
    //Util::printMatrix(lubo.proj);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    // cube
    CubeShader::UniformBufferObject cubo{};
    cubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    cubo.view = camera->getViewMatrixAtCameraPos();
    cubo.view = lubo.view; // uncomment to have stationary cube, not centered at camera
    cubo.proj = lubo.proj;
    auto cubo2 = cubo;
    cubo2.view = cubo.view;
    //engine->shaders.cubeShader.uploadToGPU(tr, cubo, cubo2, true);
 
    // pbr
    PBRShader::UniformBufferObject pubo{};
    mat4 modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    pubo.model = modeltransform;
    pubo.view = lubo.view;
    pubo.proj = lubo.proj;
    auto pubo2 = pubo;
    pubo2.view = lubo2.view;
    engine->shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);
    // change individual objects position:
    //auto grp = engine->objectStore.getGroup("knife_group");
    for (auto& wo : engine->objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        bool moveObjects = false;
        if (moveObjects) {
            if (wo->objectNum == 0) {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + (plus / 10.0f), 0.0f, 0.0f));
            }
            else {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + (plus / 100.0f), 0.0f, 0.0f));
            }
        }
        else {
            if (wo->objectNum == 0) {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.1f, 0.0f, 0.0f));
            }
            else {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, 0.0f, 0.0f));
            }
        }
        buf->model = modeltransform;
        void* data = buf;
        //Log("APP per frame dynamic buffer to address: " << hex << data << endl);
    }

    // billboards
    BillboardShader::UniformBufferObject bubo{};
    bubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    bubo.view = lubo.view;
    bubo.proj = lubo.proj;
    auto bubo2 = bubo;
    bubo2.view = lubo2.view;
    //engine->shaders.billboardShader.uploadToGPU(tr, bubo, bubo2);
    //Util::printMatrix(bubo.proj);

    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void GeneratedTexturesApp::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        // draw lines and objects
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
    }
    else if (topic == 1) {
        engine->shaders.pbrShader.addCommandBuffers(fr, drawResult);
    }
}

void GeneratedTexturesApp::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void GeneratedTexturesApp::processImage(FrameResources* fr)
{
    present(fr);
}

bool GeneratedTexturesApp::shouldClose()
{
    return shouldStopEngine;
}

void GeneratedTexturesApp::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}
