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
            // TODO override dampening for now - look into this later
			moveSpeed = vec3(0);
			//Log("Move speed (length): " << glm::length(moveSpeed) << std::endl);
		} else {
			moveSpeed += accel * acceleration_ * static_cast<float>(deltaSeconds);
			const float maxSpeed = mv.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
			//Log("Move speed length x y z: " << glm::length(moveSpeed) << " " << moveSpeed.x << " " << moveSpeed.y << " " << moveSpeed.z << std::endl);
			if (glm::length(moveSpeed) > maxSpeed)
				moveSpeed = glm::normalize(moveSpeed) * maxSpeed;
			moveSpeed = glm::normalize(moveSpeed) * mv.walkSpeedMS;
			if (mv.constantSpeed >= 0.0f) {
				moveSpeed = glm::normalize(moveSpeed) * mv.constantSpeed;
            }
			//Log("Move speed (length): " << glm::length(moveSpeed) << std::endl);
		}
	} else {
		engine->getWorld()->paths.updateCameraPosition(this, movement, deltaSeconds);
		//Error("Not implemented");
	}
}

void CameraPositioner_FirstPerson::update(double deltaSeconds,
	const glm::vec2& mousePos, bool mousePressed, bool alwaysUpright) {
	if (mousePressed) {
		const glm::vec2 delta = mousePos - this->mousePos;
		const glm::quat deltaQuat = glm::quat(glm::vec3(mouseSpeed_ * delta.y, mouseSpeed_ * delta.x, 0.0f));
		cameraOrientation = glm::normalize(deltaQuat * cameraOrientation);
		if (alwaysUpright) {
			setUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
		}

	}
	this->mousePos = mousePos;

	calcMovementVectors(movement, cameraOrientation);
	if (isModeFlying()) {
		calcMovement(movement, cameraOrientation, moveSpeed, acceleration_, damping_, maxSpeed_, fastCoef_, deltaSeconds);
		cameraPosition += moveSpeed * static_cast<float>(deltaSeconds);
	} else {
		//Log("deltaSeconds: " << deltaSeconds << endl);
		engine->getWorld()->paths.updateCameraPosition(this, movement, deltaSeconds);
	}
}

void CameraPositioner_HMD::update(int viewNum, glm::vec3 pos, glm::quat ori, glm::mat4 proj, glm::mat4 view, glm::mat4 viewCam) {
	//Log("HMD update" << std::endl);
	auto normori = glm::normalize(ori);
	lastOrientation = ori;
	lastCameraViewMatrix = viewCam;
	calcMovementVectors(movement, normori);
	if (isModeFlying()) {
		float moveLength = 0.001f; // 1 cm per input key recognition
		movement.forward = (moveLength / glm::length(movement.forward)) * movement.forward;
		movement.right = (moveLength / glm::length(movement.right)) * movement.right;
		calcMovement(movement, normori, moveSpeed, acceleration_, damping_, maxSpeed_, fastCoef_, deltaSeconds/2.0f, true);
		cameraPosition += moveSpeed * static_cast<float>(deltaSeconds);
	}
	else {
		//Log("deltaSeconds: " << deltaSeconds << endl);
        // adjust time delta because we are called twice per frame
		engine->getWorld()->paths.updateCameraPosition(this, movement, deltaSeconds/2.0f);
	}
	auto finalPos = cameraPosition + pos;
	if (viewNum == 0) {
        movement.lastFinalPositionLeft = finalPos;
        //Log("HMD detail pos left eye " << finalPos.x << " " << finalPos.y << " " << finalPos.z << " " << std::endl);
    } else {
		movement.lastFinalPositionRight = finalPos;
		//Log("HMD detail pos right eye " << finalPos.x << " " << finalPos.y << " " << finalPos.z << " " << std::endl);
	}
    movement.lastFinalPosition = finalPos;
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

glm::mat4 CameraPositionerInterface::moveObjectToCameraSpace(WorldObject* wo, const glm::vec3& deltaPos, const glm::vec3& deltaOri, vec3* finalPosition, Movement* finalMovement) {
	Movement mv;
	quat ori = getOrientation();
	calcMovementVectors(mv, ori);
	// reposition
	vec3 pos = getLastFinalPosition() + mv.right * deltaPos.x + mv.up * deltaPos.y + mv.forward * deltaPos.z;
	if (finalPosition != nullptr) {
		*finalPosition = pos;
	}
	if (finalMovement != nullptr) {
		finalMovement->forward = mv.forward;
		finalMovement->right = mv.right;
		finalMovement->up = mv.up;
		finalMovement->lastFinalPosition = movement.lastFinalPosition;
	}
	// recalc orientation
	mat4 scaled = glm::scale(mat4(1.0f), wo->scale());
	mat4 trans = glm::translate(glm::mat4(1.0f), pos);
	glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), deltaOri.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), deltaOri.y, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), deltaOri.z, glm::vec3(0.0f, 0.0f, 1.0f));

	mat4 deltaRotation = rotationZ * rotationY * rotationX;
	mat4 rotationMatrix = glm::mat3(mv.right, mv.up, mv.forward);
	rotationMatrix = rotationMatrix * deltaRotation;
	return trans * scaled * rotationMatrix;
}
