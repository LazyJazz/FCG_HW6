#pragma once
#include "app.h"

class Buffer {
 public:
  explicit Buffer(Application *app) : app_(app) {
  }
  [[nodiscard]] virtual vulkan::Buffer *GetBuffer() const = 0;

 protected:
  Application *app_;
};

template <class Ty>
class StaticBuffer : public Buffer {
 public:
  StaticBuffer(Application *app, size_t size) : Buffer(app), size_(size) {
    IgnoreResult(app_->Device()->CreateBuffer(
        sizeof(Ty) * size_,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, &buffer_));
  }
  ~StaticBuffer() = default;

  [[nodiscard]] vulkan::Buffer *GetBuffer() const override {
    return buffer_.get();
  }

  void Upload(const std::vector<Ty> &data) {
    std::unique_ptr<vulkan::Buffer> staging_buffer;
    IgnoreResult(app_->Device()->CreateBuffer(
        sizeof(Ty) * data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer));

    void *staging_data = staging_buffer->Map();
    memcpy(staging_data, data.data(), sizeof(Ty) * data.size());
    staging_buffer->Unmap();

    vulkan::SingleTimeCommand(
        app_->TransferQueue(), app_->TransferCommandPool(),
        [&](VkCommandBuffer cmd_buffer) {
          VkBufferCopy copy_region = {};
          copy_region.size = sizeof(Ty) * data.size();
          vkCmdCopyBuffer(cmd_buffer, staging_buffer->Handle(),
                          buffer_->Handle(), 1, &copy_region);
        });
  }

  size_t Size() const {
    return size_;
  }

 private:
  size_t size_{};
  std::unique_ptr<vulkan::Buffer> buffer_;
};

class DynamicBufferBase : public Buffer {
 public:
  explicit DynamicBufferBase(Application *app) : Buffer(app) {
    app_->RegisterDynamicBuffer(this);
  }
  virtual ~DynamicBufferBase() {
    app_->UnregisterDynamicBuffer(this);
  }
  virtual void Sync(VkCommandBuffer cmd_buffer) = 0;
};

template <class Ty>
class DynamicBuffer : public DynamicBufferBase {
 public:
  DynamicBuffer(Application *app, size_t size)
      : DynamicBufferBase(app), size_(size) {
    uint32_t max_frames_in_flight = app->MaxFramesInFlight();
    IgnoreResult(app_->Device()->CreateBuffer(
        sizeof(Ty) * size_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer_));
    buffers_.resize(max_frames_in_flight);
    buffer_versions_.resize(max_frames_in_flight, 0);
    staging_version_ = 0;

    for (uint32_t i = 0; i < max_frames_in_flight; i++) {
      IgnoreResult(app_->Device()->CreateBuffer(
          sizeof(Ty) * size_,
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
          VMA_MEMORY_USAGE_GPU_ONLY, &buffers_[i]));
    }
  }

  ~DynamicBuffer() override {
  }

  [[nodiscard]] vulkan::Buffer *GetBuffer(uint32_t index) const {
    return buffers_[index].get();
  }

  [[nodiscard]] vulkan::Buffer *GetBuffer() const override {
    return GetBuffer(app_->CurrentFrame());
  }

  void Sync(VkCommandBuffer cmd_buffer) override {
    Unmap();
    uint32_t current_frame = app_->CurrentFrame();
    if (staging_version_ != buffer_versions_[current_frame]) {
      vulkan::CopyBuffer(cmd_buffer, staging_buffer_.get(),
                         buffers_[current_frame].get(),
                         staging_buffer_->Size());
      buffer_versions_[current_frame] = staging_version_;
    }
  }

  Ty &At(uint32_t index) {
    Map();
    return staging_data_[index];
  }

  const Ty &At(uint32_t index) const {
    Map();
    return staging_data_[index];
  }

  Ty *Data() {
    Map();
    return staging_data_;
  }

  const Ty *Data() const {
    Map();
    return staging_data_;
  }

 private:
  void Map() {
    if (staging_data_ == nullptr) {
      staging_data_ = reinterpret_cast<Ty *>(staging_buffer_->Map());
      staging_version_++;
    }
  }

  void Unmap() {
    if (staging_data_) {
      staging_data_ = nullptr;
      staging_buffer_->Unmap();
    }
  }

  size_t size_;
  std::unique_ptr<vulkan::Buffer> staging_buffer_;
  std::vector<std::unique_ptr<vulkan::Buffer>> buffers_;
  uint64_t staging_version_{};
  std::vector<uint64_t> buffer_versions_{};
  Ty *staging_data_{nullptr};
};
