#include "mainheader.h"

using namespace glm;
using namespace std;

void CameraPositionerInterface::calcMovement(Movement& mv, glm::quat orientation, glm::vec3& moveSpeed,
			float acceleration_,	float damping_,	float maxSpeed_, float fastCoef_, double deltaSeconds, bool VRMode)
{
    // used only for default flying mode - use Paths for all other modes
	if (isModeFlying()) {
		glm::vec3 accel(0.0f);
		if (mv.forward_) accel += mv.forward;
		if (mv.backward_) accel -= mv.forward;
		if (mv.left_) accel -= mv.right;
		if (mv.right_) accel += mv.right;
		if (mv.up_) accel += mv.up;
		if (mv.down_) accel -= mv.up;
		if (mv.fastSpeed_) accel *= fastCoef_;

		if (accel == glm::vec3(0)) {
			moveSpeed -= moveSpeed * std::min((1.0f / damping_) * static_cast<float>(deltaSeconds), 1.0f);
		} else {
			moveSpeed += accel * acceleration_ * static_cast<float>(deltaSeconds);
			const float maxSpeed = mv.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
			if (glm::length(moveSpeed) > maxSpeed)
				moveSpeed = glm::normalize(moveSpeed) * maxSpeed;
		}
	} else {
		ShadedPathEngine::getInstance()->getWorld()->paths.updateCameraPosition(this, movement, deltaSeconds);
		//Error("Not implemented");
	}
}

void CameraPositioner_FirstPerson::update(double deltaSeconds,
	const glm::vec2& mousePos, bool mousePressed) {
	if (mousePressed) {
		const glm::vec2 delta = mousePos - this->mousePos;
		const glm::quat deltaQuat = glm::quat(glm::vec3(mouseSpeed_ * delta.y, mouseSpeed_ * delta.x, 0.0f));
		cameraOrientation = glm::normalize(deltaQuat * cameraOrientation);
	}
	this->mousePos = mousePos;

	calcMovementVectors(movement, cameraOrientation);
	if (isModeFlying()) {
		calcMovement(movement, cameraOrientation, moveSpeed, acceleration_, damping_, maxSpeed_, fastCoef_, deltaSeconds);
		cameraPosition += moveSpeed * static_cast<float>(deltaSeconds);
	} else {
		//Log("deltaSeconds: " << deltaSeconds << endl);
		ShadedPathEngine::getInstance()->getWorld()->paths.updateCameraPosition(this, movement, deltaSeconds);
	}
}

void CameraPositioner_HMD::update(int viewNum, glm::vec3 pos, glm::quat ori, glm::mat4 proj, glm::mat4 view, glm::mat4 viewCam) {
	//Log("HMD update" << std::endl);
	//static glm::quat lastOriLeft = glm::vec3(1.0f);
	auto normori = glm::normalize(ori);
	lastOrientation = ori;
	lastCameraViewMatrix = viewCam;
	calcMovementVectors(movement, normori);
	if (isModeFlying()) {
		float moveLength = 0.001f; // 1 cm per input key recognition
		movement.forward = (moveLength / glm::length(movement.forward)) * movement.forward;
		movement.right = (moveLength / glm::length(movement.right)) * movement.right;
		calcMovement(movement, normori, moveSpeed, acceleration_, damping_, maxSpeed_, fastCoef_, deltaSeconds, true);
		cameraPosition += moveSpeed;// *static_cast<float>(deltaSeconds);
	}
	else {
		//Log("deltaSeconds: " << deltaSeconds << endl);
		ShadedPathEngine::getInstance()->getWorld()->paths.updateCameraPosition(this, movement, deltaSeconds);
	}
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

