#pragma once
#include "long_march.h"

using namespace long_march;

class Application {
 public:
  Application();
  ~Application();
  void Run();

 private:
  void OnInit();

  void OnUpdate();

  void OnRender();

  void OnShutdown();

  void CreateDevice();
  void CreateSwapchain();
  void CreateFrameCommonAssets();
  void CreateRenderPass();
  void CreateFramebufferAssets();
  void CreateDescriptorComponents();

  void DestroyDevice();
  void DestroySwapchain();
  void DestroyFrameCommonAssets();
  void DestroyRenderPass();
  void DestroyFramebufferAssets();
  void DestroyDescriptorComponents();

  void BeginFrame();

  void EndFrame();

  GLFWwindow *window_{};

  int max_frames_in_flight_{3};

  std::shared_ptr<vulkan::Instance> instance_;
  std::shared_ptr<vulkan::Surface> surface_;
  std::shared_ptr<vulkan::Device> device_;

  std::shared_ptr<vulkan::Queue> graphics_queue_;
  std::shared_ptr<vulkan::Queue> present_queue_;
  std::shared_ptr<vulkan::Queue> transfer_queue_;

  std::shared_ptr<vulkan::CommandPool> graphics_command_pool_;
  std::shared_ptr<vulkan::CommandPool> transfer_command_pool_;

  std::shared_ptr<vulkan::Swapchain> swapchain_;

  std::vector<std::shared_ptr<vulkan::CommandBuffer>> command_buffers_;

  std::vector<std::shared_ptr<long_march::vulkan::Semaphore>>
      image_available_semaphores_;
  std::vector<std::shared_ptr<long_march::vulkan::Semaphore>>
      render_finished_semaphores_;
  std::vector<std::shared_ptr<long_march::vulkan::Fence>> in_flight_fences_;

  std::shared_ptr<vulkan::Image> frame_image_;
  std::shared_ptr<vulkan::Image> depth_image_;
  std::shared_ptr<vulkan::RenderPass> render_pass_;
  std::shared_ptr<vulkan::Framebuffer> framebuffer_;

  uint32_t current_frame_{};
  uint32_t image_index_{};
};
