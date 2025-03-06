#include "mainheader.h"
#include "AppSupport.h"
#include "TextureViewer.h"

using namespace std;
using namespace glm;

void TextureViewer::run(ContinuationInfo* cont)
{
    Log("TextureViewer started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        // camera initialization
        vec3 startPos = vec3(0.0f, 0.0f, 20.2f);
        initCamera(startPos, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
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

void TextureViewer::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);

    // load skybox cube texture
    //engine->textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine->textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    engine->textureStore.loadTexture("debug.ktx", "2dTexture");
    engine->textureStore.loadTexture("eucalyptus.ktx2", "tree");
    unsigned int texIndexTree = engine->textureStore.getTexture("tree")->index;
    engine->textureStore.loadTexture("shadedpath_logo.ktx2", "logo");
    unsigned int texIndexLogo = engine->textureStore.getTexture("logo")->index;
    engine->textureStore.loadTexture("height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT, TextureFlags::KEEP_DATA_BUFFER);
    //engine->textureStore.loadTexture("heightbig.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT);
    unsigned int texIndex = texIndexTree;
    unsigned int texIndexHeightmap = engine->textureStore.getTexture("heightmap")->index;
    engine->shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);
    // rocks
    //engine->objectStore.createGroup("rocks_group");
    //engine->meshStore.loadMesh("rocks_cmp.glb", "Rocks");
    engine->meshStore.loadMesh("DamagedHelmet_cmp.glb", "LogoBox");
    engine->objectStore.createGroup("group");
    //bottle = engine->objectStore.addObject("group", "LogoBox", vec3(0.0f, 0.0f, 0.0f));
    engine->textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTextureOrig");
    //engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTextureOrig");
    //engine->textureStore.loadTexture("irradiance.ktx2", "skyboxTexture");
    //engine->globalRendering.writeCubemapToFile(engine->textureStore.getTexture("skyboxTextureOrig"), "../../../../data/texture/wrt.ktx2");
    //engine->textureStore.loadTexture("wrt.ktx2", "skyboxTexture");
    engine->textureStore.generateBRDFLUT();
    engine->textureStore.generateCubemaps("skyboxTextureOrig");
    //engine->globalRendering.writeCubemapToFile(engine->textureStore.getTexture("skyboxTexture"), "../../../../data/texture/wrt.ktx2");
    engine->textureStore.loadTexture("irradiance.ktx2", "skyboxTexture");

    // add some lines:
    //scale tree height to 10m
    float height = 10.0f;
    float width = height;
    BillboardDef myBillboards[] = {
        { vec4(0.0f, 0.0f, 1.0f, 1.0f), // pos
          vec4(1.0f, 0.0f, 0.0f, 0.0f), // dir
          width, // w
          height, // h
          2,    // type
          0
        },
        { vec4(10.2f, 0.0f, 1.0f, 1.0f), // pos
          vec4(1.0f, 0.0f, 0.0f, 0.0f), // dir
          width, // w
          height, // h
          2,    // type
          1
        }
    };
    vector<BillboardDef> billboards;
    //for_each(begin(myBillboards), end(myBillboards), [&billboards](BillboardDef l) {billboards.push_back(l); });
    auto& allTex = engine->textureStore.getTexturesMap();
    vector<BillboardDef> billboardsToAdd;
    for (auto& tex : allTex) {
        auto& ti = tex.second;
        if (ti.isAvailable()) {
            int i = textureNames.size();
            BillboardDef b;
            b.pos = vec4(0.0f + 10.2f * i, 6.0f, -4.0f, 1.0f);
            b.dir = vec4(1.0f, 0.0f, 0.0f, 0.0f);
            b.w = width;
            b.h = height;
            b.type = 2;
            b.textureIndex = ti.index;
            textureNames.push_back(ti.id.c_str());
            billboards.push_back(b);
        }
    }

    engine->shaders.billboardShader.add(billboards);

    // select texture by uncommenting:
    //engine->globalRendering.createCubeMapFrom2dTexture("2dTexture", "2dTextureCube");
    //engine->textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    //engine->globalRendering.createCubeMapFrom2dTexture("Knife1", "2dTextureCube");
    //engine->globalRendering.createCubeMapFrom2dTexture("WaterBottle2", "2dTextureCube");
    //engine->globalRendering.createCubeMapFrom2dTexture(engine->textureStore.BRDFLUT_TEXTURE_ID, "2dTextureCube"); // works ok now
    //engine->globalRendering.createCubeMapFrom2dTexture(engine->textureStore.IRRADIANCE_TEXTURE_ID, "2dTextureCube"); // works ok now
    engine->shaders.cubeShader.setFarPlane(1.0f); // cube around center
    //engine->shaders.cubeShader.setSkybox("2dTextureCube");
    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    //engine->shaders.cubeShader.setSkybox("skyboxTextureOrig");
    //engine->shaders.cubeShader.setSkybox(engine->textureStore.IRRADIANCE_TEXTURE_ID);

    //engine->shaders.lineShader.initialUpload();
    //engine->shaders.pbrShader.initialUpload();
    //engine->shaders.cubeShader.initialUpload();
    engine->shaders.billboardShader.initialUpload();

    prepareWindowOutput("Texture Viewer");
    engine->presentation.startUI();
}

void TextureViewer::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void TextureViewer::prepareFrame(FrameResources* fr)
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
void TextureViewer::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        //engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
        engine->shaders.cubeShader.addCommandBuffers(fr, drawResult);
    }
    else if (topic == 1) {
        engine->shaders.billboardShader.addCommandBuffers(fr, drawResult);
    }
}

void TextureViewer::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void TextureViewer::processImage(FrameResources* fr)
{
    present(fr);
}

bool TextureViewer::shouldClose()
{
    return shouldStopEngine;
}

void TextureViewer::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}

static void HelpMarker(const char* desc)
{
    //ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void TextureViewer::buildCustomUI()
{
    int n;
    bool useAutoCameraCheckbox;
    bool vr = engine->isVR();
    if (!vr) {
        ImGui::Separator();
        ImGui::Text("Texture count: %d", textureNames.size());
    }
    if (ImGui::CollapsingHeader("Texture Names"))
    {
        if (!vr) {
            for (auto& t : textureNames) {
                ImGui::Separator();
                ImGui::Text(t.c_str());
            }
        }
    }
    ImGui::SameLine(); HelpMarker(helpText.c_str());
};
