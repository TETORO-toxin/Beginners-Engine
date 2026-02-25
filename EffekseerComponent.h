#pragma once
#include "Component.h"
#include <string>

// If Effekseer headers are available, expose types here
#if defined(__has_include)
#  if __has_include(<Effekseer.h>)
#    include <Effekseer.h>
#    define HAVE_EFFEKSEER 1
#  endif
#endif

struct EffekseerComponent : public Component {
    EffekseerComponent();
    EffekseerComponent(const std::string& path);

    void Awake() override;
    void Update() override;

    void Play();
    void Stop();

    void SetLoop(bool loop);
    void SetSpeed(float s);
    void SetScale(float sc);

    std::shared_ptr<Component> Clone() const override;

private:
    std::string path_;
#if defined(HAVE_EFFEKSEER)
    Effekseer::EffectRef effect_ = nullptr;
#endif
    int handle_ = -1;

    bool playing_ = false;
    bool looping_ = false;
    float speed_ = 1.0f;
    float scale_ = 1.0f;
};
