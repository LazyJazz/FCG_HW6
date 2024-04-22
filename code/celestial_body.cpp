#include "celestial_body.h"

#include "solar_system.h"

CelestialBody::CelestialBody(SolarSystem *solar_system,
                             CelestialBody *parent,
                             float radius,
                             float rotational_speed,
                             float rotaional_phase,
                             float revolution_speed,
                             float revolution_phase,
                             float revolution_radius,
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
