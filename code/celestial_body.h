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
                float rotational_speed,
                float rotaional_phase,
                float revolution_speed,
                float revolution_phase,
                float revolution_radius,
                const std::string &texture_path);

  void Update(float t);

  void Render(VkCommandBuffer cmd_buffer) const;

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
};
