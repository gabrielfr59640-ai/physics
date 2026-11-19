#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "VectorOverlay.hpp"

using namespace geode::prelude;

class $modify(PVPlayLayer, PlayLayer) {
    struct Fields {
        VectorOverlay* overlay = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (auto overlay = VectorOverlay::create(this)) {
            m_fields->overlay = overlay;
            this->addChild(overlay, 1000);
        }
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->overlay) m_fields->overlay->resetSampler();
    }
};
