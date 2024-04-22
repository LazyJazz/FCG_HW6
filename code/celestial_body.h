#pragma once
#include "app.h"
#include "buffer.h"
#include "entity.h"
#include "texture_image.h"

class SolarSystem;

class CelestialBody {
 public:
  CelestialBody(SolarSystem *solar_system,
                CelestialBody *parent,
                float radius,
                float revolution_radius,
                float rotation_speed,
                float revolution_speed,
                float rotation_phase,
                float revolution_phase,
                const std::string &texture_path,
                std::string name);

  void Update(float t);

  void Render(VkCommandBuffer cmd_buffer) const;

  [[nodiscard]] glm::mat4 WorldTransform() const {
    return world_transform_;
  }

  [[nodiscard]] glm::mat4 LocalTransform() const {
    return local_transform_;
  }

  [[nodiscard]] glm::vec3 GetPosition() const {
    return glm::vec3(world_transform_[3]);
  }

  [[nodiscard]] std::string GetName() const {
    return name_;
  }

  [[nodiscard]] float GetRadius() const {
    return radius_;
  }

 private:
  SolarSystem *solar_system_;
  CelestialBody *parent_;
  float radius_;
  float rotation_speed_;
  float rotation_phase_;
  float revolution_speed_;
  float revolution_phase_;
  float revolution_radius_;

  std::unique_ptr<TextureImage> texture_;
  std::unique_ptr<Entity> entity_;

  glm::mat4 world_transform_;
  glm::mat4 local_transform_;

  std::string name_;
};
