// Argon: model folders read (readModel: disk + decoding, no Vulkan) on a worker thread, in order; the main
// thread takes each one as it is ready and uploads it (ModelBuilder::build), so reading never holds up a frame.
#pragma once
#include <atomic>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "model/model_source.hpp"

class ModelReadQueue
{
  public:
    explicit ModelReadQueue(std::vector<std::string> folders);   // starts reading
    ~ModelReadQueue();                                            // stops after the folder being read
    ModelReadQueue(const ModelReadQueue &) = delete;
    ModelReadQueue &operator=(const ModelReadQueue &) = delete;

    // the next folder in order once it has been read: its index in the list and its model; false while it is not ready
    bool takeNext(size_t &index, ModelSource &model);
    bool allTaken() const
    {
        return taken_ == folders_.size();
    }

  private:
    void readAll();

    const std::vector<std::string> folders_;
    std::mutex readyMutex_;
    std::deque<ModelSource> ready_;   // read, not yet taken (guarded by readyMutex_)
    size_t taken_ = 0;                // the main thread's
    std::atomic<bool> stopping_{false};
    std::thread worker_;              // last: starts once the rest is set up
};
