#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

reina::graphics::Camera::Camera(const reina::window::Window& renderWindow, float fov, float aspectRatio, glm::vec3 pos, glm::vec3 cameraFront)
    : cameraPos(pos), cameraFront(cameraFront) {

    lastMousePos = glm::vec2(static_cast<double>(renderWindow.getWidth()) / 2, static_cast<double>(renderWindow.getHeight()) / 2);

    inverseProjection = glm::inverse(glm::perspective(fov, aspectRatio, 0.1f, 100.0f));
    inverseView = glm::inverse(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp));

    glfwSetWindowUserPointer(renderWindow.getGlfwWindow(), this);  // I feel like this is kinda bad design but it works
    glfwSetCursorPosCallback(renderWindow.getGlfwWindow(), mouseCallback);

    pitch = static_cast<float>(glm::degrees(asin(cameraFront.y)));
    yaw = static_cast<float>(glm::degrees(atan2(cameraFront.z, cameraFront.x)));
}

void reina::graphics::Camera::processInput(const reina::window::Window& window, double timeDelta) {
    const float cameraSpeed = 2.5f * static_cast<float>(timeDelta);

    if (window.keyPressed(GLFW_KEY_W)) {
        changed = true;
        cameraPos += cameraSpeed * cameraFront;
    } if (window.keyPressed(GLFW_KEY_S)) {
        changed = true;
        cameraPos -= cameraSpeed * cameraFront;
    } if (window.keyPressed(GLFW_KEY_A)) {
        changed = true;
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    } if (window.keyPressed(GLFW_KEY_D)) {
        changed = true;
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    } if (window.keyPressed(GLFW_KEY_SPACE)) {
        changed = true;
        cameraPos += cameraSpeed * cameraUp;
    } if (window.keyPressed(GLFW_KEY_LEFT_CONTROL)) {
        changed = true;
        cameraPos -= cameraSpeed * cameraUp;
    }
}

void reina::graphics::Camera::refresh() {
    if (changed) {
        changed = false;
        inverseView = glm::inverse(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp));
    }
}

const glm::mat4& reina::graphics::Camera::getInverseView() const {
    return inverseView;
}

const glm::mat4& reina::graphics::Camera::getInverseProjection() const {
    return inverseProjection;
}

void reina::graphics::Camera::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    if (abs(cam->lastMousePos.x + 1) < 0.00001 && abs(cam->lastMousePos.y + 1) < 0.00001) {
        cam->lastMousePos.x = static_cast<float>(xpos);
        cam->lastMousePos.y = static_cast<float>(ypos);
        return;
    }

    auto xOffset = static_cast<float>(xpos - cam->lastMousePos.x);
    auto yOffset = -1 * static_cast<float>(ypos - cam->lastMousePos.y);

    cam->lastMousePos.x = static_cast<float>(xpos);
    cam->lastMousePos.y = static_cast<float>(ypos);

    // prevent large jumps (due to weird initialization stuff)
    if (abs(xOffset) > 100 || abs(yOffset) > 100) {
        return;
    }

    cam->changed = true;

    const float sensitivity = 0.05f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cam->yaw += xOffset;
    cam->pitch += yOffset;

    if (cam->pitch > 89.0f) {
        cam->pitch = 89.0f;
    } else if (cam->pitch < -89.0f) {
        cam->pitch = -89.0f;
    }

    cam->cameraFront = glm::normalize(glm::vec3{
        static_cast<float>(cos(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch))),
        static_cast<float>(sin(glm::radians(cam->pitch))),
        static_cast<float>(sin(glm::radians(cam->yaw)) * cos(glm::radians(cam->pitch)))
    });
}

bool reina::graphics::Camera::hasChanged() const {
    return changed;
}
