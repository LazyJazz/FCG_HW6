#include "entity.h"

void Entity::Render(VkCommandBuffer cmd_buffer,
                    VkPipelineLayout pipeline_layout) const {
  VkDescriptorSet descriptor_sets[] = {
      descriptor_sets_[app_->CurrentFrame()]->Handle()};
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout, 1, 1, descriptor_sets, 0, nullptr);

  VkBuffer vertex_buffers[] = {model_->VertexBuffer()->GetBuffer()->Handle()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);

  vkCmdBindIndexBuffer(cmd_buffer, model_->IndexBuffer()->GetBuffer()->Handle(),
                       0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(cmd_buffer, model_->IndexCount(), 1, 0, 0, 0);
}

Entity::Entity(Application *app, Model *model, TextureImage *image)
    : app_(app), model_(model), image_(image) {
  descriptor_sets_.resize(app->MaxFramesInFlight());
  entity_uniform_object_ =
      std::make_unique<DynamicBuffer<EntityUniformObject>>(app, 1);
  for (int i = 0; i < app->MaxFramesInFlight(); i++) {
    app->EntityDescriptorPool()->AllocateDescriptorSet(
        app->EntityDescriptorSetLayout()->Handle(), &descriptor_sets_[i]);

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;

    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = image->GetImage()->ImageView();
    image_info.sampler = VK_NULL_HANDLE;

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_sets_[i]->Handle();
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &image_info;

    write_descriptor_sets.push_back(write_descriptor_set);

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = entity_uniform_object_->GetBuffer(i)->Handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(EntityUniformObject);

    write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_sets_[i]->Handle();
    write_descriptor_set.dstBinding = 1;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pBufferInfo = &buffer_info;

    write_descriptor_sets.push_back(write_descriptor_set);

    vkUpdateDescriptorSets(app->Device()->Handle(),
                           write_descriptor_sets.size(),
                           write_descriptor_sets.data(), 0, nullptr);
  }
}

void Entity::SetEntityInfo(const EntityUniformObject &entity_info) const {
  entity_uniform_object_->At(0) = entity_info;
}
