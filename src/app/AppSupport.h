// AppSupport.h

#ifndef APPSUPPORT_H
#define APPSUPPORT_H

// Helper class to reduce redundancy in the ShadedPath apps.
// move functionality from app code to this class if it is useful to more than one app.
// Apps need to subclass this class to be able to use it.

class AppSupport
{
public:
    void setEngine(ShadedPathEngine* e) {
        app_engine = e;
        di.setEngine(e);
        imageConsumerWindow.setEngine(e);
    }
protected:
    bool autoHalfResForVR = true;
    int win_width = 1800;// 480; 960;//1800;// 800;//3700; // 2500 // 3796

    bool firstPersonCameraAlwayUpright = true;
    Camera* camera = nullptr;
    Camera camera2;
    CameraPositioner_FirstPerson fpPositioner;
    CameraPositioner_HMD hmdPositioner;
    InputState input;
    World world;
    bool activePositionerIsHMD = false;

    void createFirstPersonCameraPositioner(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        fpPositioner.init(app_engine, pos, target, up);
        //fpPositioner.camAboveGround = 0.1f;
    }
    void createHMDCameraPositioner(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        hmdPositioner.init(app_engine, pos, target, up);
        if (app_engine->isVR()) {
            app_engine->vr.SetPositioner(getHMDCameraPositioner());
            //engine->setFramesInFlight(1);
        }
    }
    CameraPositioner_FirstPerson* getFirstPersonCameraPositioner() {
        return &fpPositioner;
    }
    CameraPositioner_HMD* getHMDCameraPositioner() {
        return &hmdPositioner;
    }
    void initCamera() {
        camera2.setEngine(app_engine);
        getHMDCameraPositioner()->setCamera(&camera2);
        if (app_engine->isVR()) {
            camera2.changePositioner(&hmdPositioner);
            activePositionerIsHMD = true;
        } else {
            camera2.changePositioner(&fpPositioner);
            activePositionerIsHMD = false;
        }
        this->camera = &camera2;
        app_engine->setWorld(&world);
    }
    void initCamera(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        createFirstPersonCameraPositioner(pos, target, up);
        createHMDCameraPositioner(pos, target, up);
        initCamera();
    }

    void setMaxSpeed(float speed) {
        fpPositioner.setMaxSpeed(speed);
        hmdPositioner.setMaxSpeed(speed);
    }

    void updateCameraPositioners(double deltaSeconds) {
        if (activePositionerIsHMD) {
            hmdPositioner.updateDeltaSeconds(deltaSeconds);
        } else {
            bool followRightMousebutton = input.pressedRight || input.stillPressedRight;
            fpPositioner.update(deltaSeconds, input.pos, followRightMousebutton, firstPersonCameraAlwayUpright);
        }
    }

	// provide input handling for regular first person and HMD cameras
    void handleInput(InputState& inputState);
    void applyViewProjection(glm::mat4& view1, glm::mat4& proj1, glm::mat4& view2, glm::mat4& proj2, glm::vec3* camPos1 = nullptr, glm::vec3* camPos2 = nullptr) {
        if (activePositionerIsHMD) {
            view1 = hmdPositioner.getViewMatrixLeft();
            proj1 = hmdPositioner.getProjectionLeft();
            view2 = hmdPositioner.getViewMatrixRight();
            proj2 = hmdPositioner.getProjectionRight();
            //Log("VR mode back image num" << tr.frameNum << endl)
            // for now, we use only one left eye cam position for both eyes. This camPos should be used for coloring calculations only.
            if (camPos1 != nullptr) {
                *camPos1 = hmdPositioner.getLastFinalPositionLeft();
            }
            if (camPos2 != nullptr) {
                *camPos2 = hmdPositioner.getLastFinalPositionRight();
            }
        } else {
            view1 = camera->getViewMatrix();
            proj1 = *getProjection();
            view2 = camera->getViewMatrix();
            proj2 = *getProjection();
            // for now, we use only one cam position for both eyes. This camPos should be used for coloring calculations only.
            if (camPos1 != nullptr) {
                *camPos1 = camera->getPosition();
                //camPos1->x *= -1.0f; // flip z axis
            }
            if (camPos2 != nullptr) {
                *camPos2 = camera->getPosition();
                //camPos2->x *= -1.0f; // flip z axis
            }
        }
    }
    void enableEventsAndModes() {
        app_engine->enableKeyEvents();
        app_engine->enableMousButtonEvents();
        app_engine->enableMouseMoveEvents();
    }
    //void initEngine(std::string name) {
    //    if (app_engine->isVR()) {
    //        app_engine->vr.SetPositioner(getHMDCameraPositioner());
    //        app_engine->setFramesInFlight(1);
    //    }
    //    else {
    //        app_engine->setFramesInFlight(2);
    //    }
    //    if (enableSound) app_engine->setEnableSound(true);
    //    if (singleThreadMode) app_engine->setSingleThreadMode(true);
    //    if (debugWindowPosition) app_engine->setDebugWindowPosition(true);

    //    // engine initialization
    //    app_engine->initGlobal(name);
    //    // even if we wanted VR initialization may have failed, fallback to non-VR
    //    if (!app_engine->isVR()) {
    //        camera->changePositioner(&fpPositioner);
    //        activePositionerIsHMD = false;
    //    }
    //    app_engine->setWorld(&world);
    //}
    void eventLoop() {
        app_engine->eventLoop();
    }

    void postUpdatePerFrame(FrameResources& tr) {
        //if (enableSound && app_engine->isDedicatedRenderUpdateThread(tr)) {
        //    engine->sound.Update(camera);
        //}
    }
    void logCameraPosition() {
        auto p = camera->getPosition();
        Log("Camera position: " << p.x << " / " << p.y << " / " << p.z << std::endl);
    }

    glm::mat4* getProjection() {
        if (!app_engine->isVR()) {
            if (fpProjectionInitialized)
                return &fpProjection;
            else {
                // create fixed perspective matrix for first person camera
                float fovy, aspect, nearz, farz;
                camera->getProjectionParams(fovy, aspect, nearz, farz);
                fpProjection = glm::perspective(fovy, aspect, nearz, farz);
                fpProjection[1][1] *= -1.0f; // flip y axis
                fpProjectionInitialized = true;
                return &fpProjection;
            }
        } else {
            Error("not implemented");
            return &fpProjection;
        }
    }
    void setHighBackbufferResolution() {
        if (app_engine->isVR()) {
            app_engine->setBackBufferResolution(ShadedPathEngine::Resolution::HMD_Native);
        } else {
            app_engine->setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        }
    }
    void prepareWindowOutput(const char* title) {
        int win_height = (int)(win_width / 1.77f);
        if (app_engine->getBackBufferExtent().width == 0) {
            Error("backbuffer extent not set");
        }
        if (app_engine->isVR()) {
            if (autoHalfResForVR) {
                win_width /= 2;
            }
            if (app_engine->vr.getHMDProperties().recommendedImageSize.width == 0) {
                Error("HMD image size not available.");
            }
            if (app_engine->vr.getHMDProperties().aspectRatio != 0.0f) {
                win_height = (int)(win_width / app_engine->vr.getHMDProperties().aspectRatio);
            }
        } else {
            win_height = (int)(win_width / app_engine->getAspect());
        }
        app_engine->presentation.createWindow(&window, win_width, win_height, title);
        app_engine->enablePresentation(&window);
        app_engine->enableWindowOutput(&window);
        imageConsumerWindow.setWindow(&window);
        app_engine->setImageConsumer(&imageConsumerWindow);
    }
    void dumpToFile(FrameResources* fr) {
        app_engine->globalRendering.dumpToFile(&fr->colorImage.fba, di);
    }
    void present(FrameResources* fr) {
        app_engine->globalRendering.present(fr, di, &window);
    }
private:
    // fixed projection matrix for first person camera
    glm::mat4 fpProjection = glm::mat4(1.0f); // identity matrix
    bool fpProjectionInitialized = false;
    ShadedPathEngine* app_engine = nullptr;
    DirectImage di;
    WindowInfo window;
    ImageConsumerWindow imageConsumerWindow;
};

#endif // APPSUPPORT_H
