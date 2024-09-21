#pragma once

struct Movement {
	bool forward_ = false;
	bool backward_ = false;
	bool left_ = false;
	bool right_ = false;
	bool up_ = false;
	bool down_ = false;
	bool fastSpeed_ = false;
};

class CameraPositionerInterface {
public:
	virtual ~CameraPositionerInterface() = default;
	// get view matrix to transform world coords to camera space
	virtual glm::mat4 getViewMatrix() const = 0;
	// get view matrix without camera movement. Think of skybox that needs to surround camera pos, but with lookAt
	virtual glm::mat4 getViewMatrixAtCameraPos() const = 0;
	virtual glm::vec3 getPosition() const = 0;
	virtual glm::vec3 getLookAt() const = 0;
    void calcMovement(Movement& mv, glm::quat orientation, glm::vec3& moveSpeed,
			float acceleration_,	float damping_,	float maxSpeed_, float fastCoef_, double deltaSeconds, bool VRMode = false) {
			const glm::mat4 v = glm::mat4_cast(orientation);
		glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
		glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
		glm::vec3 up = glm::cross(right, forward);
		if (VRMode) {
			// Default forward direction in OpenGL (negative Z-axis)
			glm::vec3 defaultForward(0.0f, 0.0f, -1.0f);
			// Rotate the default forward direction by the orientation quaternion
			glm::vec3 lookAtDirection = orientation * defaultForward;
            float moveLength = 0.001f; // 1 cm per input key recognition
            forward = (moveLength / glm::length(lookAtDirection)) * lookAtDirection;
			glm::vec3 defaultRight(1.0f, 0.0f, 0.0f);
            glm::vec3 rightDirection = orientation * defaultRight;
			right = (moveLength / glm::length(rightDirection)) * rightDirection;
			//moveSpeed = add;
			//return;
		}

		glm::vec3 accel(0.0f);
		if (mv.forward_) accel += forward;
		if (mv.backward_) accel -= forward;
		if (mv.left_) accel -= right;
		if (mv.right_) accel += right;
		if (mv.up_) accel += up;
		if (mv.down_) accel -= up;
		if (mv.fastSpeed_) accel *= fastCoef_;

		if (accel == glm::vec3(0)) {
			moveSpeed -= moveSpeed * std::min((1.0f / damping_) * static_cast<float>(deltaSeconds), 1.0f);
		}
		else {
			moveSpeed += accel * acceleration_ * static_cast<float>(deltaSeconds);
			const float maxSpeed = mv.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
			if (glm::length(moveSpeed) > maxSpeed)
				moveSpeed = glm::normalize(moveSpeed) * maxSpeed;
		}
	}
};

class  Camera {
public:
	Camera(ShadedPathEngine* engine_) {
		engine = engine_;
	}

	Camera() {
	}

	void setEngine(ShadedPathEngine* engine_) {
		engine = engine_;
	}

	ShadedPathEngine* getEngine() {
        return engine;
	}

	void changePositioner(CameraPositionerInterface& positioner_) {
		positioner = &positioner_;
	}

	glm::mat4 getViewMatrix() const {
		return positioner->getViewMatrix();
	}

	glm::mat4 getViewMatrixAtCameraPos() const {
		return positioner->getViewMatrixAtCameraPos();
	}

	glm::vec3 getPosition() const {
		return positioner->getPosition();
	}

	glm::vec3 getLookAt() const {
		return positioner->getLookAt();
	}

	// save parameters needed for creating perspective matrix. 
    // in VR mode fov (fovy and aspect) are irrelevant, because they are read from the HMD
	// at perspective creation time
	void saveProjectionParams(float fovy, float aspect, float nearZ, float farZ) {
        this->fovy = fovy;
        this->aspect = aspect;
        this->nearZ = nearZ;
        this->farZ = farZ;
	}

	// get parameters needed for creating perspective matrix. 
	// in VR mode fov (fovy and aspect) are irrelevant, because they are read from the HMD
	// at perspective creation time
	void getProjectionParams(float& fovy, float& aspect, float& nearZ, float& farZ) {
        fovy = this->fovy;
        aspect = this->aspect;
        nearZ = this->nearZ;
        farZ = this->farZ;
    }

	// get adjusted projection matrix for Vulkan Normalized Device Coordinates (flip y)
	// use this for projection matrix in shaders
	glm::mat4 getProjectionNDC() {
		Error("Not implemented");	
		return projection;
	}

	// log camera position and lookAt
	void log() {
		auto p = getPosition();
		auto l = getLookAt();
		Log(" camera pos (" << p.x << "," << p.y << "," << p.z << ") look at (" << l.x << "," << l.y << "," << l.z << ")" << std::endl);
	}
private:
	//// save projection matrix in normal screen space (y up)
	//void saveProjection(glm::mat4 p) {
	//	if (true/*!engine.isVR()*/) {
	//		p[1][1] *= -1.0f; // TODO recheck in VR mode
	//	}
	//	projection = p;
	//}

	CameraPositionerInterface* positioner = nullptr;
	glm::mat4 projection = glm::mat4(1.0f);
	ShadedPathEngine* engine = nullptr;
	float fovy, aspect, nearZ, farZ;
};

// standard first person camera, should be used for most rendering.
// auto adjust view matrix for Vulkan NDCs by flipping y
class CameraPositioner_FirstPerson final :
	public CameraPositionerInterface
{
public:
	float mouseSpeed_ = 4.0f;
	float acceleration_ = 150.0f;
	float damping_ = 0.2f;
	float maxSpeed_ = 10.0f;
	float fastCoef_ = 10.0f;
    Movement movement;

private:
	glm::vec2 mousePos = glm::vec2(0);
	glm::vec3 cameraPosition = glm::vec3( 0.0f, 10.0f, 10.0f);
	glm::quat cameraOrientation = glm::quat(glm::vec3(0));
	glm::vec3 moveSpeed = glm::vec3(0.0f);
public:
	CameraPositioner_FirstPerson() = default;
	CameraPositioner_FirstPerson(const glm::vec3 & pos, const glm::vec3 & target, const glm::vec3 & up)
		: cameraPosition(pos)
		, cameraOrientation(glm::lookAt(pos, target, up))
	{}
	void update(double deltaSeconds,
		const glm::vec2& mousePos, bool mousePressed) {
		if (mousePressed) {
			const glm::vec2 delta = mousePos - this->mousePos;
			const glm::quat deltaQuat = glm::quat(glm::vec3(mouseSpeed_ * delta.y, mouseSpeed_ * delta.x, 0.0f));
			cameraOrientation = glm::normalize(deltaQuat * cameraOrientation);
		}
		this->mousePos = mousePos;

        calcMovement(movement, cameraOrientation, moveSpeed, acceleration_, damping_, maxSpeed_, fastCoef_, deltaSeconds);
		cameraPosition += moveSpeed * static_cast<float>(deltaSeconds);
	}

	virtual glm::mat4 getViewMatrix() const override {
		const glm::mat4 t = glm::translate(glm::mat4(1.0f), -cameraPosition);
		const glm::mat4 r = glm::mat4_cast(cameraOrientation);
		auto v = r * t;
		return v;
	}

	virtual glm::mat4 getViewMatrixAtCameraPos() const override {
		const glm::mat4 r = glm::mat4_cast(cameraOrientation);
		return r;
	}

	virtual glm::vec3 getPosition() const override {
		return cameraPosition;
	}

	virtual glm::vec3 getLookAt() const override {
		const glm::mat4 view = getViewMatrix();
		const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		//Log("LookAt x y z   " << dir.x << " " << dir.y << " " << dir.z << std::endl);
		return dir;
	}

	void setPosition(const glm::vec3& pos) {
		cameraPosition = pos;
	}

	void setMaxSpeed(float max) {
		maxSpeed_ = max;
	}

	void setUpVector(const glm::vec3& up)
	{
		const glm::mat4 view = getViewMatrix();
		const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		//Log("LookAt x y z   " << dir.x << " " << dir.y << " " << dir.z << std::endl);
		cameraOrientation = glm::lookAt(cameraPosition, cameraPosition + dir, up);
	}

	void resetMousePosition(const glm::vec2& p) {
		mousePos = p;
	};
};

// move camera along a predefined path
class CameraPositioner_AutoMove final :
	public CameraPositionerInterface
{
public:
	Movement movement;
	float mouseSpeed_ = 4.0f;
	float acceleration_ = 150.0f;
	float damping_ = 0.2f;
	float maxSpeed_ = 10.0f;
	float fastCoef_ = 10.0f;

private:
	glm::vec2 mousePos = glm::vec2(0);
	glm::vec3 cameraPosition = glm::vec3(0.0f, 10.0f, 10.0f);
	glm::quat cameraOrientation = glm::quat(glm::vec3(0));
	glm::vec3 moveSpeed = glm::vec3(0.0f);
	Mover mover;
public:
	CameraPositioner_AutoMove() = default;
	CameraPositioner_AutoMove(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up, float speed)
		: cameraPosition(pos)
		, cameraOrientation(glm::lookAt(pos, target, up))
		, mover(pos, glm::vec3(0.0f, 1.0f, 0.0f), speed)
	{
	}
	void update(double deltaSeconds) {
		const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 accel(0.0f);
		if (movement.up_) accel += up;
		//if (movement.up_) Log("up" << std::endl);
		if (movement.down_) accel -= up;
		if (movement.fastSpeed_) accel *= fastCoef_;

		if (accel == glm::vec3(0)) {
			moveSpeed -= moveSpeed * std::min((1.0f / damping_) * static_cast<float>(deltaSeconds), 1.0f);
		}
		else {
			moveSpeed += accel * acceleration_ * static_cast<float>(deltaSeconds);
			const float maxSpeed = movement.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
			if (glm::length(moveSpeed) > maxSpeed)
				moveSpeed = glm::normalize(moveSpeed) * maxSpeed;
		}
		cameraPosition += moveSpeed * static_cast<float>(deltaSeconds);
		mover.position.y = cameraPosition.y;
		mover.position.y += accel.y;
		//Log("accel " << accel.y << std::endl);

		mover.moveCircle(deltaSeconds);
		cameraPosition = mover.position;
		cameraOrientation = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	virtual glm::mat4 getViewMatrix() const override {
		const glm::mat4 t = glm::translate(glm::mat4(1.0f), -cameraPosition);
		const glm::mat4 r = glm::mat4_cast(cameraOrientation);
		auto v = r * t;
		return v;
	}

	virtual glm::mat4 getViewMatrixAtCameraPos() const override {
		const glm::mat4 r = glm::mat4_cast(cameraOrientation);
		return r;
	}

	virtual glm::vec3 getPosition() const override {
		return cameraPosition;
	}

	virtual glm::vec3 getLookAt() const override {
		const glm::mat4 view = getViewMatrix();
		const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		//Log("LookAt x y z   " << dir.x << " " << dir.y << " " << dir.z << std::endl);
		return dir;
	}

	void setPosition(const glm::vec3& pos) {
		cameraPosition = pos;
	}

	void setMaxSpeed(float max) {
		maxSpeed_ = max;
	}

	void setUpVector(const glm::vec3& up)
	{
		const glm::mat4 view = getViewMatrix();
		const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		//Log("LookAt x y z   " << dir.x << " " << dir.y << " " << dir.z << std::endl);
		cameraOrientation = glm::lookAt(cameraPosition, cameraPosition + dir, up);
	}

	void resetMousePosition(const glm::vec2& p) {
		mousePos = p;
	};
};

// camera controlled by HMD
class CameraPositioner_HMD final :
	public CameraPositionerInterface
{
public:
	struct Movement movement;
	float mouseSpeed_ = 4.0f;
	float acceleration_ = 150.0f;
	float damping_ = 0.2f;
	float maxSpeed_ = 10.0f;
	float fastCoef_ = 10.0f;

private:
	glm::vec2 mousePos = glm::vec2(0);
	glm::vec3 cameraPosition = glm::vec3(0.0f, 10.0f, 10.0f);
	glm::quat cameraOrientation = glm::quat(glm::vec3(0));
	glm::vec3 moveSpeed = glm::vec3(0.0f);
	Camera* camera = nullptr;
	glm::mat4 viewMatrixLeft = glm::mat4(1.0f);
	glm::mat4 viewMatrixRight = glm::mat4(1.0f);
	glm::mat4 projectionLeft = glm::mat4(1.0f);
	glm::mat4 projectionRight = glm::mat4(1.0f);
    double deltaSeconds = 0.0;
	glm::quat lastOrientation = glm::vec3(1.0f);
    glm::mat4 lastCameraViewMatrix = glm::mat4(1.0f);
public:
	CameraPositioner_HMD() = default;
	CameraPositioner_HMD(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up)
		: cameraPosition(pos)
		, cameraOrientation(glm::lookAt(pos, target, up))
	{}
	
	void setCamera(Camera* c) {
        camera = c;
    }

    Camera* getCamera() {
        return camera;
    }

    void updateDeltaSeconds(double deltaSeconds) {
        this->deltaSeconds = deltaSeconds;
	}

	void update(int viewNum, glm::vec3 pos, glm::quat ori, glm::mat4 proj, glm::mat4 view, glm::mat4 viewCam) {
		//Log("HMD update" << std::endl);
        //static glm::quat lastOriLeft = glm::vec3(1.0f);
		auto normori = glm::normalize(ori);
		lastOrientation = ori;
        lastCameraViewMatrix = viewCam;
		calcMovement(movement, normori, moveSpeed, acceleration_, damping_, maxSpeed_, fastCoef_, deltaSeconds, true);
		cameraPosition += moveSpeed;// *static_cast<float>(deltaSeconds);
		auto finalPos = cameraPosition + pos;
		const glm::mat4 t = glm::translate(glm::mat4(1.0f), -finalPos);
		const glm::mat4 r = glm::mat4_cast(normori);
		auto v = r * t; // ok, but slurs
		if (viewNum == 0) {
			viewMatrixLeft = v;
			projectionLeft = proj;
			viewMatrixLeft = view;
			//if (v != view) {
			//    Log("view matrix differs" << std::endl);
			//}
		}
		else {
			viewMatrixRight = v;
			projectionRight = proj;
			viewMatrixRight = view;
            //if (normori == lastOriLeft) {
            //    Log("different eye ori" << std::endl);
            //}
		}
	}

	virtual glm::mat4 getViewMatrix() const override {
		Error("Not implemented");
		return viewMatrixLeft;
	}

	glm::mat4 getViewMatrixLeft() {
		return viewMatrixLeft;
	}
	glm::mat4 getViewMatrixRight() {
		return viewMatrixRight;
	}

	glm::mat4 getProjectionLeft() {
		return projectionLeft;
	}
	glm::mat4 getProjectionRight() {
		return projectionRight;
	}

    // get view matrix at camera position, independent of camera movement
	// orientation is the same for both eyes
    // so we can simply cast the quaternion to the view matrix
	virtual glm::mat4 getViewMatrixAtCameraPos() const override {
		//const glm::mat4 r = glm::mat4_cast(lastOrientation);
		const glm::mat4 r = lastCameraViewMatrix;
		return r;
	}

	virtual glm::vec3 getPosition() const override {
        //Log("HMD getPosition " << cameraPosition.x << " " << cameraPosition.y << " " << cameraPosition.z << " " << std::endl);
		return cameraPosition;
	}

    // look at vector is the same for both eyes
	// uses orientation gotten from last update()
    // TODO: currently only used in sound positioning, check other uses, maybe needs change in Canera::Update()
	virtual glm::vec3 getLookAt() const override {
		// Default forward direction in OpenGL (negative Z-axis)
		glm::vec3 defaultForward(0.0f, 0.0f, -1.0f);
		// Rotate the default forward direction by the orientation quaternion
		glm::vec3 dir = lastOrientation * defaultForward;
		//Log("LookAt x y z   " << dir.x << " " << dir.y << " " << dir.z << std::endl);
		return dir;
	}

	void setPosition(const glm::vec3& pos) {
		cameraPosition = pos;
	}

	void setMaxSpeed(float max) {
		maxSpeed_ = max;
	}

	void setUpVector(const glm::vec3& up)
	{
		const glm::mat4 view = getViewMatrix();
		const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		//Log("LookAt x y z   " << dir.x << " " << dir.y << " " << dir.z << std::endl);
		cameraOrientation = glm::lookAt(cameraPosition, cameraPosition + dir, up);
	}

	void resetMousePosition(const glm::vec2& p) {
		mousePos = p;
	};
};
