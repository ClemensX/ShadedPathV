// AppSupport.h

#ifndef APPSUPPORT_H
#define APPSUPPORT_H

// Helper class to reduce redundancy in the ShadedPath apps.
// move functionality from app code to this class if it is useful to more than one app.
// Apps need to subclass this class to be able to use it.
class AppSupport
{
protected:
    bool enableLines = true;
    bool enableUI = false;
    bool vr = false;
    bool stereo = true;
    bool enableSound = false;
    bool singleThreadMode = true;
    Camera* camera = nullptr;
    Camera camera2;
    CameraPositioner_FirstPerson fpPositioner;
    CameraPositioner_HMD hmdPositioner;
    InputState input;
    ShadedPathEngine* engine = nullptr;
    void setEngine(ShadedPathEngine& engine_) {
        engine = &engine_;
    }
    void createFirstPersonCameraPositioner(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
        fpPositioner = CameraPositioner_FirstPerson(pos, target, up);
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
        }
        else {
            camera2.changePositioner(fpPositioner);
        }
        this->camera = &camera2;
    }
    void updateCameraPositioners(double deltaSeconds) {
        fpPositioner.update(deltaSeconds, input.pos, input.pressedLeft);
        hmdPositioner.updateDeltaSeconds(deltaSeconds);
    }

	// provide input handling for regular first person and HMD cameras
    void handleInput(InputState& inputState);
    void applyViewProjection(glm::mat4& view1, glm::mat4& proj1, glm::mat4& view2, glm::mat4& proj2) {
        if (camera->getEngine()->isVR()) {
            view1 = hmdPositioner.getViewMatrixLeft();
            proj1 = hmdPositioner.getProjectionLeft();
            view2 = hmdPositioner.getViewMatrixRight();
            proj2 = hmdPositioner.getProjectionRight();
            //Log("VR mode back image num" << tr.frameNum << endl)
        } else {
            view1 = camera->getViewMatrix();
            proj1 = camera->getProjectionNDC();
            view2 = camera->getViewMatrix();
            proj2 = camera->getProjectionNDC();

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
        engine->setFixedPhysicalDeviceIndex(0); // needed for Renderdoc
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

        // engine initialization
        engine->init(name);
        // even if we wanted VR initialization may have failed, fallback to non-VR
        if (!engine->isVR()) {
            camera->changePositioner(fpPositioner);
        }
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
private:
};

#endif // APPSUPPORT_H
