#include "mainheader.h"
#include "AppSupport.h"

using namespace std;
using namespace glm;

void AppSupport::handleInput(InputState& inputState)
{
    if (inputState.mouseButtonEvent) {
        //Log("mouse button pressed (left/right): " << inputState.pressedLeft << " / " << inputState.pressedRight << endl);
        input.pressedLeft = inputState.pressedLeft;
        input.pressedRight = inputState.pressedRight;
    }
    input.stillPressedLeft = inputState.stillPressedLeft;
    input.stillPressedRight = inputState.stillPressedRight;
    if (inputState.mouseMoveEvent) {
        //Log("mouse pos (x/y): " << inputState.pos.x << " / " << inputState.pos.y << endl);
        input.pos.x = inputState.pos.x;
        input.pos.y = inputState.pos.y;
    }
    if (inputState.keyEvent) {
        //Log("key pressed: " << inputState.key << endl);
        auto key = inputState.key;
        auto action = inputState.action;
        auto mods = inputState.mods;
        const bool press = action != GLFW_RELEASE;
        if (key == GLFW_KEY_W) {
            fpPositioner.movement.forward_ = press;
            hmdPositioner.movement.forward_ = press;
        }
        if (key == GLFW_KEY_S) {
            fpPositioner.movement.backward_ = press;
            hmdPositioner.movement.backward_ = press;
        }
        if (key == GLFW_KEY_A) {
            fpPositioner.movement.left_ = press;
            hmdPositioner.movement.left_ = press;
        }
        if (key == GLFW_KEY_D) {
            fpPositioner.movement.right_ = press;
            hmdPositioner.movement.right_ = press;
        }
        if (key == GLFW_KEY_1) {
            fpPositioner.movement.up_ = press;
            hmdPositioner.movement.up_ = press;
        }
        if (key == GLFW_KEY_2) {
            fpPositioner.movement.down_ = press;
            hmdPositioner.movement.down_ = press;
        }
        if (mods & GLFW_MOD_SHIFT) {
            fpPositioner.movement.fastSpeed_ = press;
            hmdPositioner.movement.fastSpeed_ = press;
        }
        if (key == GLFW_KEY_SPACE) {
            fpPositioner.setUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
            //hmdPositioner->movement.backward_ = press;
        }
        if (key == GLFW_KEY_Q && action == GLFW_RELEASE) {
            Log("Quit key pressed.\n")
            engine->shouldCloseThisEngine = true; // load next chapter in another engine, keep window
        }
        if (key == GLFW_KEY_F && action == GLFW_RELEASE) {
            if (fpPositioner.isModeFlying()) {
                fpPositioner.setModeWalking();
                hmdPositioner.setModeWalking();
                Log("Camera set to walking mode.\n")
            } else {
                fpPositioner.setModeFlying();
                hmdPositioner.setModeFlying();
                Log("Camera set to flying mode.\n")
            }
        }
    }
}
