#include "model.h"

Model::Model(Application *app,
             const std::vector<Vertex> &vertices,
             const std::vector<uint32_t> &indices)
    : app_(app) {
  vertex_buffer_ = std::make_unique<StaticBuffer<Vertex>>(app, vertices.size());
  index_buffer_ = std::make_unique<StaticBuffer<uint32_t>>(app, indices.size());
  vertex_buffer_->Upload(vertices);
  index_buffer_->Upload(indices);
}
