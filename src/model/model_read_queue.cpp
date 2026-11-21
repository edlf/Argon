#include "model/model_read_queue.hpp"

#include <utility>

ModelReadQueue::ModelReadQueue(std::vector<std::string> folders) : folders_(std::move(folders)), worker_(&ModelReadQueue::readAll, this)
{
}

ModelReadQueue::~ModelReadQueue()
{
    stopping_ = true;
    worker_.join();
}

void ModelReadQueue::readAll()
{
    for (const std::string &folder : folders_)
    {
        if (stopping_)
        {
            return;
        }
        ModelSource model = readModel(folder);
        const std::lock_guard<std::mutex> lock(readyMutex_);
        ready_.push_back(std::move(model));
    }
}

bool ModelReadQueue::takeNext(size_t &index, ModelSource &model)
{
    const std::lock_guard<std::mutex> lock(readyMutex_);
    if (ready_.empty())
    {
        return false;
    }
    model = std::move(ready_.front());
    ready_.pop_front();
    index = taken_++;
    return true;
}
