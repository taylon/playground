#include <glm/glm.hpp>

struct CameraState {
  float yaw;
  float pitch;
  float fieldOfView;
  glm::vec3 position;
  glm::vec3 directionPointingAt;
};

// TODO(taylon): namespace this maybe?
enum CameraMovementType {
  Left,
  Right,
  Forward,
  Backwards,
};

glm::mat4 cameraViewMatrix();
glm::mat4 cameraProjectionMatrix();

void moveCamera(float speed, CameraMovementType movementType);
void cameraLookAround(double xPosition, double yPosition);
void cameraZoomOut(double offset);
