// AppSupport.h

#ifndef APPSUPPORT_H
#define APPSUPPORT_H

// Helper class to reduce redundancy in the ShadedPath apps.
// move functionality from app code to this class if it is useful to more than one app.
// Apps need to subclass this class to be able to use it.

class AppSupport
{
protected:
    bool enableLines = false;
    bool enableUI = true;
    bool vr = false;
    bool stereo = false;
    bool enableSound = true;
    bool singleThreadMode = false;
    bool debugWindowPosition = false; // if true try to open app window in right screen part
    bool enableRenderDoc = true;

    bool firstPersonCameraAlwayUpright = true;
    Camera* camera = nullptr;
    Camera camera2;
    CameraPositioner_FirstPerson fpPositioner;
    CameraPositioner_HMD hmdPositioner;
    InputState input;
    ShadedPathEngine* engine = nullptr;
    World world;
    bool activePositionerIsHMD = false;
    void setEngine(ShadedPathEngine& engine_) {
        engine = &engine_;
    }
    void createFirstPersonCameraPositioner(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        fpPositioner = CameraPositioner_FirstPerson(pos, target, up);
        //fpPositioner.camAboveGround = 0.1f;
    }
    void createHMDCameraPositioner(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        hmdPositioner = CameraPositioner_HMD(pos, target, up);
    }
    CameraPositioner_FirstPerson* getFirstPersonCameraPositioner() {
        return &fpPositioner;
    }
    CameraPositioner_HMD* getHMDCameraPositioner() {
        return &hmdPositioner;
    }
    void initCamera() {
        camera2.setEngine(engine);
        getHMDCameraPositioner()->setCamera(&camera2);
        if (vr) {
            camera2.changePositioner(hmdPositioner);
            activePositionerIsHMD = true;
        } else {
            camera2.changePositioner(fpPositioner);
            activePositionerIsHMD = false;
        }
        this->camera = &camera2;
    }
    void initCamera(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        createFirstPersonCameraPositioner(pos, target, up);
        createHMDCameraPositioner(pos, target, up);
        initCamera();
    }

    void updateCameraPositioners(double deltaSeconds) {
        static float bug_dist = 0.0f, bug_time = 0.0f;
        bug_time += deltaSeconds;
        //Log("bug time update per frame: " << bug_time << std::endl);
        if (activePositionerIsHMD) {
            hmdPositioner.updateDeltaSeconds(deltaSeconds);
        } else {
            bool followRightMousebutton = input.pressedRight || input.stillPressedRight;
            fpPositioner.update(deltaSeconds, input.pos, followRightMousebutton, firstPersonCameraAlwayUpright);
        }
    }

	// provide input handling for regular first person and HMD cameras
    void handleInput(InputState& inputState);
    void applyViewProjection(glm::mat4& view1, glm::mat4& proj1, glm::mat4& view2, glm::mat4& proj2) {
        if (activePositionerIsHMD) {
            view1 = hmdPositioner.getViewMatrixLeft();
            proj1 = hmdPositioner.getProjectionLeft();
            view2 = hmdPositioner.getViewMatrixRight();
            proj2 = hmdPositioner.getProjectionRight();
            //Log("VR mode back image num" << tr.frameNum << endl)
        } else {
            view1 = camera->getViewMatrix();
            proj1 = *getProjection();
            view2 = camera->getViewMatrix();
            proj2 = *getProjection();

        }
    }
    void enableEventsAndModes() {
        engine->enableKeyEvents();
        engine->enableMousButtonEvents();
        engine->enableMouseMoveEvents();
        if (vr) {
            engine->enableVR();
        }
        if (stereo) {
            engine->enableStereo();
            engine->enableStereoPresentation();
        }
        if (enableRenderDoc) {
            engine->setFixedPhysicalDeviceIndex(0);
        }
    }
    void initEngine(std::string name) {
        if (engine->isVR()) {
            engine->vr.SetPositioner(getHMDCameraPositioner());
            engine->setFramesInFlight(1);
        }
        else {
            engine->setFramesInFlight(2);
        }
        if (enableSound) engine->enableSound();
        if (singleThreadMode) engine->setThreadModeSingle();
        if (debugWindowPosition) engine->debugWindowPosition = true;

        // engine initialization
        engine->init(name);
        // even if we wanted VR initialization may have failed, fallback to non-VR
        if (!engine->isVR()) {
            camera->changePositioner(fpPositioner);
            activePositionerIsHMD = false;
        }
        engine->setWorld(&world);
    }
    void eventLoop() {
        // some shaders may need additional preparation
        engine->prepareDrawing();


        // rendering
        while (!engine->shouldClose()) {
            engine->pollEvents();
            engine->drawFrame();
        }
        engine->waitUntilShutdown();
    }
    void postUpdatePerFrame(ThreadResources& tr) {
        if (enableSound && engine->isDedicatedRenderUpdateThread(tr)) {
            engine->sound.Update(camera);
        }
    }
    void logCameraPosition() {
        auto p = camera->getPosition();
        Log("Camera position: " << p.x << " / " << p.y << " / " << p.z << std::endl);
    }

    glm::mat4* getProjection() {
        if (!engine->isVR()) {
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
        if (vr) {
            engine->setBackBufferResolution(ShadedPathEngine::Resolution::HMDIndex);
        } else {
            engine->setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        }
    }
private:
    // fixed projection matrix for first person camera
    glm::mat4 fpProjection = glm::mat4(1.0f); // identity matrix
    bool fpProjectionInitialized = false;
};

#endif // APPSUPPORT_H
