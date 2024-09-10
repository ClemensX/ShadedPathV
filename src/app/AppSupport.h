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
    Camera* camera = nullptr;
    CameraPositioner_FirstPerson fpPositioner;
    CameraPositioner_HMD hmdPositioner;
    InputState input;
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
	// provide input handling for regular first person and HMD cameras
    void handleInput(InputState& inputState);
private:
};

#endif // APPSUPPORT_H
