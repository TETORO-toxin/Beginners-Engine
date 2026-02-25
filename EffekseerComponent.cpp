#include "EffekseerComponent.h"
#include "GameObject.h"
#include <fstream>

EffekseerComponent::EffekseerComponent()
: path_(), handle_(-1), playing_(false), looping_(false), speed_(1.0f), scale_(1.0f) {}

EffekseerComponent::EffekseerComponent(const std::string& path)
: path_(path), handle_(-1), playing_(false), looping_(false), speed_(1.0f), scale_(1.0f) {}

void EffekseerComponent::Awake() {
    std::ifstream ifs(path_, std::ios::binary);
    if (!ifs) {
        std::string alt = std::string("Assets/") + path_;
        std::ifstream ifs2(alt, std::ios::binary);
        if (ifs2) path_ = alt;
    }
    // No-op: real Effekseer integration requires enabling and adapting to project headers.
}

void EffekseerComponent::Update() {
    // No-op placeholder: if looping is requested, simulate by setting playing flag.
    if (looping_ && !playing_) {
        playing_ = true; // simulated
    }
}

void EffekseerComponent::Play() {
    playing_ = true;
}

void EffekseerComponent::Stop() {
    playing_ = false;
}

void EffekseerComponent::SetLoop(bool loop) { looping_ = loop; }
void EffekseerComponent::SetSpeed(float s) { speed_ = s; }
void EffekseerComponent::SetScale(float sc) { scale_ = sc; }

std::shared_ptr<Component> EffekseerComponent::Clone() const {
    return std::make_shared<EffekseerComponent>(path_);
}
