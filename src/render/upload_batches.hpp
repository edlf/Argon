// Argon: the GPU uploads grouped into one submission (the models a loader reads at once); Renderer runs them on its
// device, which may start after whoever holds this.
#pragma once

class UploadBatches
{
  public:
    virtual void beginUploadBatch() = 0;
    virtual void submitUploadBatch() = 0;

  protected:
    ~UploadBatches() = default;
};
