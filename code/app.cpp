#include "app.h"

Application::Application() {
  if (!glfwInit()) {
    throw std::runtime_error("glfwInit failed.");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(1280, 720, "FCG HW2", nullptr, nullptr);
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
  CreateDescriptorComponents();
}

void Application::OnShutdown() {
  DestroyDescriptorComponents();
  DestroyFrameCommonAssets();
  DestroySwapchain();
  DestroyDevice();
}

void Application::OnUpdate() {
}

void Application::OnRender() {
  BeginFrame();

  VkCommandBuffer cmd_buffer = command_buffers_[current_frame_]->Handle();

  VkImage swapchain_image = swapchain_->Image(image_index_);

  vulkan::TransitImageLayout(
      cmd_buffer, swapchain_image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_IMAGE_ASPECT_COLOR_BIT);

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

void Application::CreateDescriptorComponents() {
}

void Application::DestroyDescriptorComponents() {
}

void Application::BeginFrame() {
  VkFence fence = in_flight_fences_[current_frame_]->Handle();
  vkWaitForFences(device_->Handle(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device_->Handle(), 1, &fence);

  if (window_) {
    VkSemaphore image_available_semaphore =
        image_available_semaphores_[current_frame_]->Handle();
    VkResult result = swapchain_->AcquireNextImage(
        std::numeric_limits<uint64_t>::max(), image_available_semaphore,
        VK_NULL_HANDLE, &image_index_);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      // Recreate swapchain
      THROW_IF_FAILED(device_->WaitIdle(),
                      "Failed to wait for device idle, on swapchain recreate.");
      CreateSwapchain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("Failed to acquire next image");
    }
  }

  VkCommandBuffer command_buffer = command_buffers_[current_frame_]->Handle();

  vkResetCommandBuffer(command_buffer, 0);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin recording command buffer");
  }
}

void Application::EndFrame() {
  VkResult result;
  VkCommandBuffer command_buffer = command_buffers_[current_frame_]->Handle();
  THROW_IF_FAILED(vkEndCommandBuffer(command_buffer),
                  "Failed to record command buffer")

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
      "Failed to submit command buffer")

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
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to present image");
    }
  }

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
}
