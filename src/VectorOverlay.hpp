#pragma once
#include <Geode/Geode.hpp>
#include "PhysicsSampler.hpp"

using namespace geode::prelude;

class VectorOverlay : public CCNode {
public:
    static VectorOverlay* create(PlayLayer* pl);
    void resetSampler();

private:
    struct Config {
        bool enabled = true, weight = true, normal = true, friction = true, jump = true,
             resultant = true, formulas = true, values = true, info = true, panel = true;
        std::string mode = "standard";
        float scale = 1.f, opacity = 0.9f, textSize = 1.f, mass = 1.f;
    };

    static constexpr int kLabelCount = 4; // P, N, J, SF

    bool init(PlayLayer* pl);
    void update(float dt) override;
    void refreshConfig();
    void buildPanel();
    void onToggle(CCObject* sender);
    void drawArrow(CCPoint from, CCPoint dir, float len, ccColor3B color, float alpha);
    void setLabel(int idx, std::string const& text, CCPoint at, ccColor3B color);
    void hideAll();

    PlayLayer* m_pl = nullptr;
    CCDrawNode* m_draw = nullptr;
    CCLabelBMFont* m_labels[kLabelCount] {};
    std::string m_labelCache[kLabelCount];
    CCLabelBMFont* m_info = nullptr;
    std::string m_infoCache;
    CCMenu* m_panel = nullptr;
    PhysicsSampler m_sampler;
    Config m_cfg;
    float m_cfgTimer = 0.f;
};
