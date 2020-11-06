#include "glm/fwd.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

const auto upwardsInWorldSpace = glm::vec3(0.0f, 1.0f, 0.0f);

// TODO(taylon): this would go to settings at some point... probably
const float mouseSensitivity = 0.1f;

CameraState initialCameraState() {
  CameraState state;
  state.yaw = -90.0f;
  state.pitch = 0.0f;
  state.fieldOfView = 60.0f;
  state.position = glm::vec3(0.0f, 0.0f, 3.0f);
  state.directionPointingAt = glm::vec3(0.0f, 0.0f, -1.0f);

  return state;
}

auto cameraState = initialCameraState();

glm::mat4 cameraViewMatrix() {
  auto pitch = cameraState.pitch;
  auto yaw = cameraState.yaw;
  glm::vec3 direction;
  direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  direction.y = sin(glm::radians(pitch));
  direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraState.directionPointingAt = glm::normalize(direction);

  auto toTheRightFromCamera = glm::normalize(
      glm::cross(upwardsInWorldSpace, cameraState.directionPointingAt));
  auto upwardsFromCamera =
      glm::cross(cameraState.directionPointingAt, toTheRightFromCamera);

  return glm::lookAt(cameraState.position,
                     cameraState.position + cameraState.directionPointingAt,
                     upwardsFromCamera);
}

glm::mat4 cameraProjectionMatrix() {
  return glm::perspective(glm::radians(cameraState.fieldOfView),
                          800.0f / 600.0f, 0.1f, 100.0f);
}

// TODO(taylon): fix this, it is the width/height divided by 2
auto lastX = 400.0f;
auto lastY = 300.0f;
void cameraLookAround(double xPosition, double yPosition) {
  auto pitch = &cameraState.pitch;
  auto yaw = &cameraState.yaw;

  auto xOffset = xPosition - lastX;
  lastX = xPosition;

  // reversed since y-coordinates range from bottom to top
  auto yOffset = lastY - yPosition;
  lastY = yPosition;

  xOffset *= mouseSensitivity;
  yOffset *= mouseSensitivity;

  *yaw += xOffset;
  *pitch += yOffset;

  if (*pitch > 89.0f) {
    *pitch = 89.0f;
  } else if (*pitch < -89.0f) {
    *pitch = -89.0f;
  }
}

void moveCamera(float movementSpeed, CameraMovementType movementType) {
  auto directionPointingAt = cameraState.directionPointingAt;

  switch (movementType) {
  case Forward:
    cameraState.position += movementSpeed * directionPointingAt;
    break;
  case Backwards:
    cameraState.position -= movementSpeed * directionPointingAt;
    break;
  case Right:
    cameraState.position +=
        glm::normalize(glm::cross(directionPointingAt, upwardsInWorldSpace)) *
        movementSpeed;
    break;
  case Left:
    cameraState.position -=
        glm::normalize(glm::cross(directionPointingAt, upwardsInWorldSpace)) *
        movementSpeed;
    break;
  }

  // keep it at the ground level
  cameraState.position.y = 0;
}

void cameraZoomOut(double offset) {
  auto fieldOfView = &cameraState.fieldOfView;

  *fieldOfView -= (float)offset;
  if (*fieldOfView < 1.0f) {
    *fieldOfView = 1.0f;
  } else if (*fieldOfView > 60.0f) {
    *fieldOfView = 60.0f;
  }
}
