#pragma once

class CameraPositionerInterface {
public:
	virtual ~CameraPositionerInterface() = default;
	// get view matrix to transform world coords to camera space
	virtual glm::mat4 getViewMatrix() const = 0;
	// get view matrix without camera movement. Think of skybox that needs to surround camera pos, but with lookAt
	virtual glm::mat4 getViewMatrixAtCameraPos() const = 0;
	virtual glm::vec3 getPosition() const = 0;
	virtual glm::vec3 getLookAt() const = 0;
};

class  Camera {
public:
	Camera(ShadedPathEngine* engine_){
	engine = engine_;
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

	// save projection matrix in normal screen space (y up)
	void saveProjection(glm::mat4 p) {
		if (true/*!engine.isVR()*/) {
			p[1][1] *= -1.0f; // TODO recheck in VR mode
		}
		projection = p;
	}

	// get adjusted projection matrix for Vulkan Normalized Device Coordinates (flip y)
	// use this for projection matrix in shaders
	glm::mat4 getProjectionNDC() {
		//Error("Not implemented");	
		return projection;
	}

	// log camera position and lookAt
	void log() {
		auto p = getPosition();
		auto l = getLookAt();
		Log(" camera pos (" << p.x << "," << p.y << "," << p.z << ") look at (" << l.x << "," << l.y << "," << l.z << ")" << std::endl);
	}
private:
	CameraPositionerInterface* positioner = nullptr;
	glm::mat4 projection = glm::mat4(1.0f);
	ShadedPathEngine* engine = nullptr;
};

// standard first person camera, should be used for most rendering.
// auto adjust view matrix for Vulkan NDCs by flipping y
class CameraPositioner_FirstPerson final :
	public CameraPositionerInterface
{
public:
	struct Movement {
		bool forward_ = false;
		bool backward_ = false;
		bool left_ = false;
		bool right_ = false;
		bool up_ = false;
		bool down_ = false;
		bool fastSpeed_ = false;
	} movement;
	float mouseSpeed_ = 4.0f;
	float acceleration_ = 150.0f;
	float damping_ = 0.2f;
	float maxSpeed_ = 10.0f;
	float fastCoef_ = 10.0f;

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

		const glm::mat4 v = glm::mat4_cast(cameraOrientation);
		const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
		const glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
		const glm::vec3 up = glm::cross(right, forward);

		glm::vec3 accel(0.0f);
		if (movement.forward_) accel += forward;
		if (movement.backward_) accel -= forward;
		if (movement.left_) accel -= right;
		if (movement.right_) accel += right;
		if (movement.up_) accel += up;
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
	struct Movement {
		bool forward_ = false;
		bool backward_ = false;
		bool left_ = false;
		bool right_ = false;
		bool up_ = false;
		bool down_ = false;
		bool fastSpeed_ = false;
	} movement;
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
	struct Movement {
		bool forward_ = false;
		bool backward_ = false;
		bool left_ = false;
		bool right_ = false;
		bool up_ = false;
		bool down_ = false;
		bool fastSpeed_ = false;
	} movement;
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
public:
	CameraPositioner_HMD() = default;
	CameraPositioner_HMD(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up)
		: cameraPosition(pos)
		, cameraOrientation(glm::lookAt(pos, target, up))
	{}
	void setCamera(Camera* c) {
        camera = c;
    }

	void update(int viewNum, glm::vec3 pos, glm::quat ori, glm::mat4 proj, glm::mat4 view) {
		static float add = 0.0f;
		add += 0.1f;
		//Log("HMD update" << std::endl);
		auto normori = glm::normalize(ori);
		//cameraPosition = pos;
		//cameraPosition.z = pos.z + add;
		//camera->saveProjection(proj);
		//viewMatrix = view;

		const glm::mat4 t = glm::translate(glm::mat4(1.0f), -pos);
		const glm::mat4 r = glm::mat4_cast(normori);
		auto v = r * t;
		if (viewNum == 0) {
			viewMatrixLeft = v;
			projectionLeft = proj;
		}
		else {
			viewMatrixRight = v;
			projectionRight = proj;
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

	virtual glm::mat4 getViewMatrixAtCameraPos() const override {
		Error("Not implemented");
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
