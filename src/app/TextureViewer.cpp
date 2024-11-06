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
    auto& allTex = engine.textureStore.getTexturesMap();
    vector<BillboardDef> billboardsToAdd;
    for (auto& tex : allTex) {
        auto& ti = tex.second;
        if (ti.available) {
            int i = textureNames.size();
            BillboardDef b;
            b.pos = vec4(0.0f + 10.2f * i, 0.0f, 1.0f, 1.0f);
            b.dir = vec4(1.0f, 0.0f, 0.0f, 0.0f);
            b.w = width;
            b.h = height;
            b.type = 2;
            b.textureIndex = ti.index;
            textureNames.push_back(ti.id.c_str());
            billboards.push_back(b);
        }
    }

    engine.shaders.billboardShader.add(billboards);

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
    static string helpText =
        "click to see texture names\n(leftmost on top)";
    int n;
    bool useAutoCameraCheckbox;
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
