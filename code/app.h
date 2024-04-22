#pragma once
#include "glm/glm.hpp"
#include "utils.h"

class Application {
 public:
  Application();
  ~Application();
  void Run();

  [[nodiscard]] uint32_t MaxFramesInFlight() const {
    return max_frames_in_flight_;
  }
  [[nodiscard]] const vulkan::Device *Device() const {
    return device_.get();
  }
  [[nodiscard]] const vulkan::Queue *GraphicsQueue() const {
    return graphics_queue_.get();
  }
  [[nodiscard]] const vulkan::CommandPool *GraphicsCommandPool() const {
    return graphics_command_pool_.get();
  }
  [[nodiscard]] const vulkan::Queue *TransferQueue() const {
    return transfer_queue_.get();
  }
  [[nodiscard]] const vulkan::CommandPool *TransferCommandPool() const {
    return transfer_command_pool_.get();
  }
  [[nodiscard]] const vulkan::RenderPass *RenderPass() const {
    return render_pass_.get();
  }
  [[nodiscard]] const vulkan::DescriptorSetLayout *EntityDescriptorSetLayout()
      const {
    return entity_descriptor_set_layout_.get();
  }
  [[nodiscard]] const vulkan::DescriptorPool *EntityDescriptorPool() const {
    return entity_descriptor_pool_.get();
  }
  [[nodiscard]] const vulkan::Swapchain *Swapchain() const {
    return swapchain_.get();
  }
  [[nodiscard]] const vulkan::Sampler *EntitySampler() const {
    return entity_sampler_.get();
  }
  [[nodiscard]] GLFWwindow *Window() const {
    return window_;
  }
  [[nodiscard]] uint32_t CurrentFrame() const {
    return current_frame_;
  }

  void RegisterDynamicBuffer(DynamicBufferBase *buffer);
  void UnregisterDynamicBuffer(DynamicBufferBase *buffer);

 private:
  void OnInit();
  void OnUpdate();
  void OnRender();
  void OnShutdown();

  virtual void OnInitImpl() = 0;
  virtual void OnShutdownImpl() = 0;
  virtual void OnUpdateImpl() = 0;
  virtual void OnRenderImpl(VkCommandBuffer cmd_buffer) = 0;

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

  std::set<DynamicBufferBase *> dynamic_buffers_;

  std::unique_ptr<vulkan::Sampler> entity_sampler_;
  std::unique_ptr<vulkan::DescriptorSetLayout> entity_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorPool> entity_descriptor_pool_;
};
