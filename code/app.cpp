#include "app.h"

#include "buffer.h"

Application::Application() {
  if (!glfwInit()) {
    throw std::runtime_error("glfwInit failed.");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  //  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(1280, 720, "FCG HW6", nullptr, nullptr);
  if (!window_) {
    throw std::runtime_error("glfwCreateWindow failed.");
  }
}

Application::~Application() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}

#define THROW_IF_FAILED(x, err_msg)    \
  result = (x);                        \
  if (result != VK_SUCCESS) {          \
    throw std::runtime_error(err_msg); \
  }

void Application::Run() {
  OnInit();
  while (!glfwWindowShouldClose(window_)) {
    OnUpdate();
    OnRender();
    glfwPollEvents();
  }
  VkResult result;
  THROW_IF_FAILED(device_->WaitIdle(), "Failed to wait for device idle.");
  OnShutdown();
}

void Application::OnInit() {
  CreateDevice();
  CreateSwapchain();
  CreateFrameCommonAssets();
  CreateRenderPass();
  CreateFramebufferAssets();
  CreateDescriptorComponents();
  OnInitImpl();
}

void Application::OnShutdown() {
  OnShutdownImpl();
  DestroyDescriptorComponents();
  DestroyFramebufferAssets();
  DestroyRenderPass();
  DestroyFrameCommonAssets();
  DestroySwapchain();
  DestroyDevice();
}

void Application::OnUpdate() {
  OnUpdateImpl();
  VkResult result;
  THROW_IF_FAILED(vulkan::SingleTimeCommand(
                      transfer_queue_.get(), transfer_command_pool_.get(),
                      [&](VkCommandBuffer cmd_buffer) {
                        for (auto dynamic_buffer : dynamic_buffers_) {
                          dynamic_buffer->Sync(cmd_buffer);
                        }
                      }),
                  "Failed to execute single time command.")
}

void Application::OnRender() {
  BeginFrame();

  VkCommandBuffer cmd_buffer = command_buffers_[current_frame_]->Handle();

  VkImage swapchain_image = swapchain_->Image(image_index_);

  VkClearValue clear_values[2];
  clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[1].depthStencil = {1.0f, 0};

  VkExtent2D framebuffer_extent = framebuffer_->Extent();

  VkRenderPassBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_info.renderPass = render_pass_->Handle();
  begin_info.framebuffer = framebuffer_->Handle();
  begin_info.clearValueCount = 2;
  begin_info.pClearValues = clear_values;
  begin_info.renderArea.offset = {0, 0};
  begin_info.renderArea.extent = framebuffer_extent;

  vkCmdBeginRenderPass(cmd_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

  // Set viewport and scissor
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = framebuffer_extent.width;
  viewport.height = framebuffer_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = framebuffer_extent;
  vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

  OnRenderImpl(cmd_buffer);

  vkCmdEndRenderPass(cmd_buffer);

  vulkan::TransitImageLayout(
      cmd_buffer, swapchain_image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  VkImageCopy copy_region{};
  copy_region.srcOffset = {};
  copy_region.dstOffset = {};
  copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.srcSubresource.layerCount = 1;
  copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.dstSubresource.layerCount = 1;
  copy_region.extent.width = framebuffer_->Extent().width;
  copy_region.extent.height = framebuffer_->Extent().height;
  copy_region.extent.depth = 1;

  vkCmdCopyImage(cmd_buffer, frame_image_->Handle(),
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain_image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

  vulkan::TransitImageLayout(
      cmd_buffer, swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
      VK_IMAGE_ASPECT_COLOR_BIT);

  EndFrame();
}

void Application::CreateDevice() {
  VkResult result;

  vulkan::InstanceCreateHint instance_create_hint;
  THROW_IF_FAILED(vulkan::CreateInstance(instance_create_hint, &instance_),
                  "Failed to create vulkan instance.")

  THROW_IF_FAILED(instance_->CreateSurfaceFromGLFWWindow(window_, &surface_),
                  "Failed to create surface for current glfw window.")

  vulkan::DeviceFeatureRequirement feature_requirement;
  feature_requirement.surface = surface_.get();

  THROW_IF_FAILED(instance_->CreateDevice(feature_requirement, &device_),
                  "Failed to create vulkan logical device.")

  THROW_IF_FAILED(
      device_->GetQueue(device_->PhysicalDevice().GraphicsFamilyIndex(), 0,
                        &graphics_queue_),
      "Failed to get graphics queue.")
  THROW_IF_FAILED(
      device_->GetQueue(device_->PhysicalDevice().TransferFamilyIndex(), -1,
                        &transfer_queue_),
      "Failed to get transfer queue.")
  THROW_IF_FAILED(
      device_->GetQueue(
          device_->PhysicalDevice().PresentFamilyIndex(surface_.get()), 0,
          &present_queue_),
      "Failed to get present queue.")

  THROW_IF_FAILED(device_->CreateCommandPool(
                      device_->PhysicalDevice().GraphicsFamilyIndex(),
                      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                      &graphics_command_pool_),
                  "Failed to create graphics command pool.")
  THROW_IF_FAILED(device_->CreateCommandPool(
                      device_->PhysicalDevice().TransferFamilyIndex(),
                      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                      &transfer_command_pool_),
                  "Failed to create transfer command pool.")
}

void Application::DestroyDevice() {
  transfer_command_pool_.reset();
  graphics_command_pool_.reset();
  present_queue_.reset();
  transfer_queue_.reset();
  graphics_queue_.reset();
  device_.reset();
  surface_.reset();
  instance_.reset();
}

void Application::CreateSwapchain() {
  swapchain_.reset();

  VkResult result;
  THROW_IF_FAILED(device_->CreateSwapchain(surface_.get(), &swapchain_),
                  "Failed to create swapchain.");
}

void Application::DestroySwapchain() {
  swapchain_.reset();
}

void Application::CreateFrameCommonAssets() {
  VkResult result;
  command_buffers_.resize(max_frames_in_flight_);
  render_finished_semaphores_.resize(max_frames_in_flight_);
  image_available_semaphores_.resize(max_frames_in_flight_);
  in_flight_fences_.resize(max_frames_in_flight_);

  for (int i = 0; i < max_frames_in_flight_; i++) {
    THROW_IF_FAILED(
        graphics_command_pool_->AllocateCommandBuffer(&command_buffers_[i]),
        "Failed to create graphics command buffer.")
    THROW_IF_FAILED(device_->CreateSemaphore(&render_finished_semaphores_[i]),
                    "Failed to create render finished semaphore.")
    THROW_IF_FAILED(device_->CreateSemaphore(&image_available_semaphores_[i]),
                    "Failed to create image available semaphore.")
    THROW_IF_FAILED(device_->CreateFence(true, &in_flight_fences_[i]),
                    "Failed to create in flight fence.")
  }
}

void Application::DestroyFrameCommonAssets() {
  command_buffers_.clear();
  render_finished_semaphores_.clear();
  image_available_semaphores_.clear();
  in_flight_fences_.clear();
}

void Application::CreateRenderPass() {
  VkResult result;

  VkAttachmentDescription color_attachment_description;
  VkAttachmentDescription depth_attachment_description;

  color_attachment_description.format = VK_FORMAT_B8G8R8A8_UNORM;
  color_attachment_description.flags = 0;
  color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment_description.finalLayout =
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_description.stencilStoreOp =
      VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;

  depth_attachment_description.format = VK_FORMAT_D32_SFLOAT;
  depth_attachment_description.flags = 0;
  depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment_description.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment_description.stencilStoreOp =
      VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;

  VkAttachmentReference color_reference;
  VkAttachmentReference depth_reference;

  color_reference.attachment = 0;
  color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  depth_reference.attachment = 1;
  depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  THROW_IF_FAILED(
      device_->CreateRenderPass(
          {color_attachment_description, depth_attachment_description},
          {color_reference}, depth_reference, &render_pass_),
      "Failed to create render pass.")
}

void Application::DestroyRenderPass() {
  render_pass_.reset();
}

void Application::CreateFramebufferAssets() {
  VkResult result;
  THROW_IF_FAILED(device_->CreateImage(VK_FORMAT_B8G8R8A8_UNORM,
                                       swapchain_->Extent(), &frame_image_),
                  "Failed to create frame image.")
  THROW_IF_FAILED(
      device_->CreateImage(VK_FORMAT_D32_SFLOAT, swapchain_->Extent(),
                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                           VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_),
      "Failed to create depth image.")
  THROW_IF_FAILED(render_pass_->CreateFramebuffer(
                      {frame_image_->ImageView(), depth_image_->ImageView()},
                      swapchain_->Extent(), &framebuffer_),
                  "Failed to create framebuffer.")
}

void Application::DestroyFramebufferAssets() {
  framebuffer_.reset();
  frame_image_.reset();
  depth_image_.reset();
}

void Application::CreateDescriptorComponents() {
  VkResult result;

  THROW_IF_FAILED(
      device_->CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false,
                             VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                             VK_SAMPLER_MIPMAP_MODE_LINEAR, &entity_sampler_),
      "Failed to create entity sampler.")

  VkSampler sampler = entity_sampler_->Handle();
  THROW_IF_FAILED(
      device_->CreateDescriptorSetLayout(
          {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, &sampler},
           {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr}},
          &entity_descriptor_set_layout_),
      "Failed to create entity descriptor set layout.")

  vulkan::DescriptorPoolSize pool_size =
      (entity_descriptor_set_layout_->GetPoolSize() * 1024) *
      max_frames_in_flight_;
  THROW_IF_FAILED(device_->CreateDescriptorPool(
                      pool_size.ToVkDescriptorPoolSize(),
                      max_frames_in_flight_ * 1024, &entity_descriptor_pool_),
                  "Failed to create entity descriptor pool.")
}

void Application::DestroyDescriptorComponents() {
  entity_descriptor_pool_.reset();
  entity_descriptor_set_layout_.reset();
  entity_sampler_.reset();
}

void Application::BeginFrame() {
  VkResult result;
  VkFence fence = in_flight_fences_[current_frame_]->Handle();
  vkWaitForFences(device_->Handle(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device_->Handle(), 1, &fence);

  if (window_) {
    VkSemaphore image_available_semaphore =
        image_available_semaphores_[current_frame_]->Handle();
    result = swapchain_->AcquireNextImage(std::numeric_limits<uint64_t>::max(),
                                          image_available_semaphore,
                                          VK_NULL_HANDLE, &image_index_);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      // Recreate swapchain
      THROW_IF_FAILED(device_->WaitIdle(),
                      "Failed to wait for device idle, on swapchain recreate.");
      CreateSwapchain();
      DestroyFramebufferAssets();
      CreateFramebufferAssets();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("Failed to acquire next image.");
    }
  }

  VkCommandBuffer command_buffer = command_buffers_[current_frame_]->Handle();

  vkResetCommandBuffer(command_buffer, 0);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  THROW_IF_FAILED(vkBeginCommandBuffer(command_buffer, &begin_info),
                  "Failed to begin recording command buffer.")
}

void Application::EndFrame() {
  VkResult result;
  VkCommandBuffer command_buffer = command_buffers_[current_frame_]->Handle();
  THROW_IF_FAILED(vkEndCommandBuffer(command_buffer),
                  "Failed to record command buffer.")

  VkSemaphore image_available_semaphore =
      image_available_semaphores_[current_frame_]->Handle();
  VkSemaphore render_finished_semaphore =
      render_finished_semaphores_[current_frame_]->Handle();

  VkFence fence = in_flight_fences_[current_frame_]->Handle();
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  if (window_) {
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphore;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semaphore;
  }

  THROW_IF_FAILED(
      vkQueueSubmit(graphics_queue_->Handle(), 1, &submit_info, fence),
      "Failed to submit command buffer.")

  if (window_) {
    VkSwapchainKHR swapchain = swapchain_->Handle();
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index_;

    result = vkQueuePresentKHR(present_queue_->Handle(), &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      THROW_IF_FAILED(device_->WaitIdle(),
                      "Failed to wait for device idle, on swapchain recreate.");
      CreateSwapchain();
      DestroyFramebufferAssets();
      CreateFramebufferAssets();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to present image.");
    }
  }

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
}

void Application::RegisterDynamicBuffer(DynamicBufferBase *buffer) {
  dynamic_buffers_.insert(buffer);
}

void Application::UnregisterDynamicBuffer(DynamicBufferBase *buffer) {
  dynamic_buffers_.erase(buffer);
}
