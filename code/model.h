#pragma once
#include "app.h"
#include "buffer.h"
#include "glm/glm.hpp"

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 tex_coord;
};

class Model {
 public:
  Model(Application *app,
        const std::vector<Vertex> &vertices,
        const std::vector<uint32_t> &indices);

  [[nodiscard]] Buffer *VertexBuffer() const {
    return vertex_buffer_.get();
  }

  [[nodiscard]] Buffer *IndexBuffer() const {
    return index_buffer_.get();
  }

  [[nodiscard]] uint32_t IndexCount() const {
    return index_buffer_->Size();
  }

 private:
  Application *app_;
  std::unique_ptr<StaticBuffer<Vertex>> vertex_buffer_;
  std::unique_ptr<StaticBuffer<uint32_t>> index_buffer_;
};
