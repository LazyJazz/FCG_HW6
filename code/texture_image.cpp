#include "texture_image.h"

TextureImage::TextureImage(Application *app, const Image &image) : app_(app) {
  IgnoreResult(app_->Device()->CreateImage(
      VK_FORMAT_R8G8B8A8_UNORM,
      {uint32_t(image.Width()), uint32_t(image.Height())},
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT, &image_));

  std::unique_ptr<vulkan::Buffer> staging_buffer;
  IgnoreResult(app_->Device()->CreateBuffer(
      image.Width() * image.Height() * sizeof(ImagePixel),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
      &staging_buffer));

  void *staging_data = staging_buffer->Map();
  memcpy(staging_data, image.Data(),
         image.Width() * image.Height() * sizeof(ImagePixel));
  staging_buffer->Unmap();

  vulkan::SingleTimeCommand(
      app_->TransferQueue(), app_->TransferCommandPool(),
      [&](VkCommandBuffer cmd_buffer) {
        vulkan::TransitImageLayout(
            cmd_buffer, image_->Handle(), VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {uint32_t(image.Width()), uint32_t(image.Height()),
                              1};

        vkCmdCopyBufferToImage(
            cmd_buffer, staging_buffer->Handle(), image_->Handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transfer back to shader read optimal layout
        vulkan::TransitImageLayout(
            cmd_buffer, image_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
      });
}
