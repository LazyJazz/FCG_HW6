#include "celestial_body.h"

#include "solar_system.h"

CelestialBody::CelestialBody(SolarSystem *solar_system,
                             CelestialBody *parent,
                             float radius,
                             float revolution_radius,
                             float rotational_speed,
                             float revolution_speed,
                             float rotaional_phase,
                             float revolution_phase,
                             const std::string &texture_path)
    : solar_system_(solar_system),
      parent_(parent),
      radius_(radius),
      rotation_speed_(rotational_speed),
      rotation_phase_(rotaional_phase),
      revolution_speed_(revolution_speed),
      revolution_phase_(revolution_phase),
      revolution_radius_(revolution_radius) {
  Image image;
  image.ReadFromFile(texture_path);
  texture_ = std::make_unique<TextureImage>(solar_system_, image);
  entity_ = std::make_unique<Entity>(
      solar_system_, solar_system_->GetSphereModel(), texture_.get());
}

void CelestialBody::Render(VkCommandBuffer cmd_buffer) const {
  entity_->Render(cmd_buffer, solar_system_->EntityPipelineLayout()->Handle());
}

void CelestialBody::Update(float t) {
  glm::mat4 ref_transform{1.0f};
  if (parent_) {
    ref_transform = parent_->WorldTransform();
  }
  float revolution_angle = revolution_speed_ * t + revolution_phase_;
  float rotation_angle = rotation_speed_ * t + rotation_phase_;

  glm::mat4 revolution_transform =
      glm::rotate(glm::mat4{1.0f}, revolution_angle,
                  glm::vec3{0.0f, 1.0f, 0.0f}) *
      glm::translate(glm::mat4{1.0f},
                     glm::vec3{revolution_radius_, 0.0f, 0.0f});
  glm::mat4 rotation_transform =
      glm::rotate(glm::mat4{1.0f}, rotation_angle,
                  glm::vec3{0.0f, 1.0f, 0.0f}) *
      glm::scale(glm::mat4{1.0f}, glm::vec3{radius_});

  world_transform_ = ref_transform * revolution_transform;
  local_transform_ = rotation_transform;

  EntityUniformObject entity_info{};
  entity_info.model_ = world_transform_ * local_transform_;
  entity_->SetEntityInfo(entity_info);
}
