#pragma once

class CameraPositionerInterface {
public:
	virtual ~CameraPositionerInterface() = default;
	virtual glm::mat4 getViewMatrix() const = 0;
	virtual glm::vec3 getPosition() const = 0;
};

class  Camera final
{
public:
	explicit Camera(CameraPositionerInterface& positioner)
		: positioner(positioner) {}

	glm::mat4 getViewMatrix() const {
		return positioner.getViewMatrix();
	}

	glm::vec3 getPosition() const {
		return positioner.getPosition();
	}

private:
	CameraPositionerInterface& positioner;
};

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
		const glm::mat4 r =	glm::mat4_cast(cameraOrientation);
		return r * t;
	}

	virtual glm::vec3 getPosition() const override {
		return cameraPosition;
	}

	void setPosition(const glm::vec3& pos) {
		cameraPosition = pos;
	}

	void setUpVector(const glm::vec3& up)
	{
		const glm::mat4 view = getViewMatrix();
		const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		cameraOrientation = glm::lookAt(cameraPosition, cameraPosition + dir, up);
	}

	void resetMousePosition(const glm::vec2& p) {
		mousePos = p;
	};
};

