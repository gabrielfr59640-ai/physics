#include "VectorOverlay.hpp"
#include <algorithm>
#include <cmath>

namespace {
    // Constantes d'AFFICHAGE uniquement (pas des constantes physiques) :
    // nombre d'unites de force / d'impulsion representees par un pixel a l'echelle 1.
    constexpr float kForcePerPixel = 100.f;
    constexpr float kImpulsePerPixel = 20.f;

    constexpr ccColor3B kColP  {255, 70, 70};
    constexpr ccColor3B kColN  {70, 140, 255};
    constexpr ccColor3B kColJ  {255, 220, 50};
    constexpr ccColor3B kColSF {70, 220, 110};
    constexpr ccColor3B kColF  {190, 90, 255};
    constexpr ccColor3B kColInfo {235, 235, 235};

    struct PanelEntry { char const* key; char const* text; ccColor3B color; };
    constexpr PanelEntry kPanel[] = {
        {"show-weight",    "P",   kColP},
        {"show-normal",    "N",   kColN},
        {"show-jump",      "J",   kColJ},
        {"show-resultant", "SumF", kColSF},
        {"show-formulas",  "Formules", kColInfo},
    };

    char const* modeName(PlayerMode m) {
        switch (m) {
            case PlayerMode::Cube: return "cube";
            case PlayerMode::Ship: return "vaisseau";
            case PlayerMode::Ufo: return "ufo";
            case PlayerMode::Ball: return "boule";
            case PlayerMode::Wave: return "wave";
            case PlayerMode::Robot: return "robot";
            case PlayerMode::Spider: return "araignee";
            case PlayerMode::Swing: return "swing";
        }
        return "?";
    }
}

VectorOverlay* VectorOverlay::create(PlayLayer* pl) {
    auto ret = new VectorOverlay();
    if (ret->init(pl)) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

bool VectorOverlay::init(PlayLayer* pl) {
    if (!CCNode::init()) return false;
    m_pl = pl;

    m_draw = CCDrawNode::create();
    this->addChild(m_draw, 0);

    for (int i = 0; i < kLabelCount; i++) {
        m_labels[i] = CCLabelBMFont::create("", "chatFont.fnt");
        m_labels[i]->setAnchorPoint({0.f, 0.5f});
        m_labels[i]->setVisible(false);
        this->addChild(m_labels[i], 1);
    }
    m_info = CCLabelBMFont::create("", "chatFont.fnt");
    m_info->setAnchorPoint({0.f, 1.f});
    m_info->setVisible(false);
    this->addChild(m_info, 1);

    refreshConfig();
    buildPanel();
    this->scheduleUpdate();
    return true;
}

void VectorOverlay::resetSampler() {
    m_sampler.reset();
}

void VectorOverlay::refreshConfig() {
    auto m = Mod::get();
    m_cfg.enabled   = m->getSettingValue<bool>("enabled");
    m_cfg.weight    = m->getSettingValue<bool>("show-weight");
    m_cfg.normal    = m->getSettingValue<bool>("show-normal");
    m_cfg.friction  = m->getSettingValue<bool>("show-friction");
    m_cfg.jump      = m->getSettingValue<bool>("show-jump");
    m_cfg.resultant = m->getSettingValue<bool>("show-resultant");
    m_cfg.formulas  = m->getSettingValue<bool>("show-formulas");
    m_cfg.values    = m->getSettingValue<bool>("show-values");
    m_cfg.info      = m->getSettingValue<bool>("show-info");
    m_cfg.panel     = m->getSettingValue<bool>("show-panel");
    m_cfg.mode      = m->getSettingValue<std::string>("display-mode");
    m_cfg.scale     = static_cast<float>(m->getSettingValue<double>("scale"));
    m_cfg.opacity   = static_cast<float>(m->getSettingValue<double>("opacity"));
    m_cfg.textSize  = static_cast<float>(m->getSettingValue<double>("text-size"));
    m_cfg.mass      = static_cast<float>(m->getSettingValue<double>("mass"));
    if (m_panel) m_panel->setVisible(m_cfg.enabled && m_cfg.panel);
}

void VectorOverlay::buildPanel() {
    m_panel = CCMenu::create();
    m_panel->setPosition({0.f, 0.f});
    auto win = CCDirector::get()->getWinSize();
    float y = win.height * 0.6f;
    int tag = 0;
    for (auto const& e : kPanel) {
        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(VectorOverlay::onToggle), 0.4f);
        toggler->setTag(tag++);
        toggler->toggle(Mod::get()->getSettingValue<bool>(e.key));
        toggler->setPosition({18.f, y});
        m_panel->addChild(toggler);

        auto lbl = CCLabelBMFont::create(e.text, "chatFont.fnt");
        lbl->setColor(e.color);
        lbl->setScale(0.6f);
        lbl->setAnchorPoint({0.f, 0.5f});
        lbl->setPosition({34.f, y});
        m_panel->addChild(lbl);
        y -= 24.f;
    }
    this->addChild(m_panel, 2);
    m_panel->setVisible(m_cfg.enabled && m_cfg.panel);
}

void VectorOverlay::onToggle(CCObject* sender) {
    auto t = static_cast<CCMenuItemToggler*>(sender);
    int idx = t->getTag();
    if (idx < 0 || idx >= static_cast<int>(std::size(kPanel))) return;
    // Dans le callback, isToggled() renvoie l'etat AVANT le basculement.
    Mod::get()->setSettingValue<bool>(kPanel[idx].key, !t->isToggled());
    refreshConfig();
}

void VectorOverlay::hideAll() {
    m_draw->clear();
    for (auto l : m_labels) l->setVisible(false);
    m_info->setVisible(false);
}

void VectorOverlay::drawArrow(CCPoint from, CCPoint dir, float len, ccColor3B c, float alpha) {
    if (len < 1.f) return;
    CCPoint tip = from + dir * len;
    ccColor4F col {c.r / 255.f, c.g / 255.f, c.b / 255.f, alpha};
    float head = std::min(10.f, len * 0.4f);
    CCPoint perp {-dir.y, dir.x};
    CCPoint base = tip - dir * head;
    m_draw->drawSegment(from, base, 1.5f, col);
    CCPoint tri[3] = {tip, base + perp * (head * 0.5f), base - perp * (head * 0.5f)};
    m_draw->drawPolygon(tri, 3, col, 0.f, col);
}

void VectorOverlay::setLabel(int idx, std::string const& text, CCPoint at, ccColor3B color) {
    auto l = m_labels[idx];
    if (m_labelCache[idx] != text) {
        l->setString(text.c_str());
        m_labelCache[idx] = text;
    }
    l->setColor(color);
    l->setOpacity(static_cast<GLubyte>(m_cfg.opacity * 255.f));
    l->setScale(0.45f * m_cfg.textSize);
    l->setPosition(at);
    l->setVisible(true);
}

void VectorOverlay::update(float dt) {
    auto player = m_pl ? m_pl->m_player1 : nullptr;
    m_cfgTimer += dt;
    if (m_cfgTimer > 0.25f) { m_cfgTimer = 0.f; refreshConfig(); }

    if (!m_cfg.enabled || !player) { hideAll(); return; }

    m_sampler.update(player, dt);
    auto const& s = m_sampler.get();
    m_draw->clear();
    if (!s.valid) { hideAll(); return; }

    // Position du joueur en coordonnees ecran -> espace de cet overlay (taille constante a l'ecran)
    auto parent = player->getParent();
    CCPoint world = parent ? parent->convertToWorldSpace(player->getPosition()) : player->getPosition();
    CCPoint origin = this->convertToNodeSpace(world);

    bool minimal = m_cfg.mode == "minimal";
    bool detailed = m_cfg.mode == "detailed";
    bool showFormulas = m_cfg.formulas && !minimal;
    bool showValues = m_cfg.values && !minimal;
    float alpha = m_cfg.opacity;
    float m = m_cfg.mass;
    CCPoint gDir = s.upsideDown ? CCPoint{0.f, 1.f} : CCPoint{0.f, -1.f};

    bool used[kLabelCount] = {false, false, false, false};
    CCPoint lblPos[kLabelCount];
    std::string notes; // lignes de statut pour le tableau d'infos

    auto compose = [&](char const* sym, char const* formula, std::string const& value) {
        std::string t = sym;
        if (showValues && !value.empty()) t += " = " + value;
        if (showFormulas) t += "\n" + std::string(formula);
        return t;
    };

    // --- P : poids (modele : P = m*g, g mesure en chute libre) ---
    if (m_cfg.weight) {
        if (s.gravityModel && s.gKnown) {
            float F = m * s.g;
            float len = F / kForcePerPixel * m_cfg.scale;
            CCPoint o = origin + CCPoint{-8.f, 0.f};
            drawArrow(o, gDir, len, kColP, alpha);
            used[0] = true;
            lblPos[0] = o + gDir * len + CCPoint{6.f, 0.f};
            setLabel(0, compose("P", "P = m*g (g mesure)", fmt::format("{:.0f}", F)), lblPos[0], kColP);
        } else if (!s.gravityModel) {
            notes += fmt::format("P: non modelise en mode {}\n", modeName(s.mode));
        } else {
            notes += "P: g inconnu (attend une chute libre)\n";
        }
    }

    // --- N : reaction normale (THEORIQUE : equilibre vertical, N = -P) ---
    if (m_cfg.normal && s.gravityModel && s.onGround && s.gKnown) {
        float F = m * s.g;
        float len = F / kForcePerPixel * m_cfg.scale;
        CCPoint o = origin + CCPoint{8.f, 0.f};
        drawArrow(o, gDir * -1.f, len, kColN, alpha);
        used[1] = true;
        lblPos[1] = o - gDir * len + CCPoint{6.f, 0.f};
        setLabel(1, compose("N", "N = m*g (theorique)", fmt::format("{:.0f}", F)), lblPos[1], kColN);
    }

    // --- J : impulsion (variation de vitesse imposee par le moteur, J = m*dv) ---
    if (m_cfg.jump && s.jumpTimer > 0.f) {
        float J = m * s.jumpDv;
        float len = std::abs(J) / kImpulsePerPixel * m_cfg.scale;
        CCPoint dir {0.f, J >= 0.f ? 1.f : -1.f};
        float fade = s.jumpTimer / PhysicsSampler::kJumpShowTime;
        CCPoint o = origin + CCPoint{16.f, 0.f};
        drawArrow(o, dir, len, kColJ, alpha * fade);
        used[2] = true;
        lblPos[2] = o + dir * len + CCPoint{6.f, 0.f};
        setLabel(2, compose("J", "J = dp = m*dv (impulsion)", fmt::format("{:.0f}", J)), lblPos[2], kColJ);
    }

    // --- SF : resultante MESUREE, SF = m*a avec a = dv/dt observee ---
    if (m_cfg.resultant) {
        CCPoint Fv = s.acc * m;
        float mag = std::sqrt(Fv.x * Fv.x + Fv.y * Fv.y);
        float len = mag / kForcePerPixel * m_cfg.scale;
        CCPoint dir = mag > 1e-3f ? Fv * (1.f / mag) : CCPoint{0.f, 1.f};
        CCPoint o = origin + CCPoint{-16.f, 0.f};
        drawArrow(o, dir, len, kColSF, alpha);
        used[3] = true;
        lblPos[3] = o + (len >= 1.f ? dir * len : CCPoint{0.f, 0.f}) + CCPoint{-6.f, 12.f};
        setLabel(3, compose("SumF", "SumF = m*a (mesuree)", fmt::format("{:.0f}", mag)), lblPos[3], kColSF);
    }

    // --- anti-chevauchement simple des etiquettes (decalage vertical) ---
    for (int i = 0; i < kLabelCount; i++) {
        if (!used[i]) { m_labels[i]->setVisible(false); continue; }
        float h = m_labels[i]->getContentSize().height * m_labels[i]->getScale();
        float w = m_labels[i]->getContentSize().width * m_labels[i]->getScale();
        CCPoint p = m_labels[i]->getPosition();
        for (int j = 0; j < i; j++) {
            if (!used[j]) continue;
            CCPoint q = m_labels[j]->getPosition();
            float hj = m_labels[j]->getContentSize().height * m_labels[j]->getScale();
            float wj = m_labels[j]->getContentSize().width * m_labels[j]->getScale();
            if (std::abs(p.x - q.x) < std::max(w, wj) && std::abs(p.y - q.y) < (h + hj) * 0.5f)
                p.y = q.y + (h + hj) * 0.5f + 2.f;
        }
        m_labels[i]->setPosition(p);
    }

    // --- tableau d'infos ---
    if (m_cfg.info && !minimal) {
        if (m_cfg.friction)
            notes += "f: aucun frottement dans le moteur (vitesse X imposee)\n";
        char const* state = s.onGround ? (s.upsideDown ? "au plafond" : "au sol")
                          : (s.jumpTimer > 0.f ? "en saut" : "en l'air");
        std::string txt = fmt::format(
            "Mode: {}{}  Etat: {}\nX={:.1f} Y={:.1f}\nvx={:.0f} vy={:.0f} u/s (mesure)\nax={:.0f} ay={:.0f} u/s2 (mesure)\n",
            modeName(s.mode), s.mini ? " (mini)" : "", state, s.pos.x, s.pos.y,
            s.vel.x, s.vel.y, s.acc.x, s.acc.y);
        if (detailed) {
            txt += fmt::format("m_yVelocity brut (moteur)={:.3f}\n", s.rawYVelocity);
            txt += s.gKnown ? fmt::format("g estime={:.0f} u/s2 (derive)\n", s.g)
                            : std::string("g estime=inconnu\n");
            txt += fmt::format("m={:.2f} (parametre d'affichage)\n", m);
        }
        txt += notes;

        if (txt != m_infoCache) { m_info->setString(txt.c_str()); m_infoCache = txt; }
        m_info->setColor(kColInfo);
        m_info->setOpacity(static_cast<GLubyte>(alpha * 255.f));
        m_info->setScale(0.4f * m_cfg.textSize);
        auto win = CCDirector::get()->getWinSize();
        float w = m_info->getContentSize().width * m_info->getScale();
        float h = m_info->getContentSize().height * m_info->getScale();
        float x = std::clamp(origin.x + 70.f, 4.f, win.width - w - 4.f);
        float y = std::clamp(origin.y + 60.f, h + 4.f, win.height - 4.f);
        m_info->setPosition({x, y});
        m_info->setVisible(true);
    } else {
        m_info->setVisible(false);
    }
}
