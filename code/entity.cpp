#include "entity.h"

void Entity::Render(VkCommandBuffer cmd_buffer) const {
  VkBuffer vertex_buffers[] = {model_->VertexBuffer()->GetBuffer()->Handle()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);

  vkCmdBindIndexBuffer(cmd_buffer, model_->IndexBuffer()->GetBuffer()->Handle(),
                       0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(cmd_buffer, model_->IndexCount(), 1, 0, 0, 0);
}
