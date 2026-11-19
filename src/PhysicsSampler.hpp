#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

enum class PlayerMode { Cube, Ship, Ufo, Ball, Wave, Robot, Spider, Swing };

// Toutes les grandeurs ici sont MESUREES a partir des positions successives
// du joueur (v = dx/dt, a = dv/dt), exprimees en unites GD (u, u/s, u/s^2).
// Aucune constante physique n'est inventee : g est estime en chute libre.
struct PhysicsSample {
    bool valid = false;
    CCPoint pos {0, 0};
    CCPoint vel {0, 0};
    CCPoint acc {0, 0};
    bool onGround = false;
    bool upsideDown = false;
    bool mini = false;
    bool gravityModel = false; // true: cube, ball, robot, spider
    PlayerMode mode = PlayerMode::Cube;
    bool gKnown = false;
    float g = 0.f;             // |g| estime (u/s^2)
    float jumpDv = 0.f;        // delta v vertical (u/s) du dernier evenement d'impulsion
    float jumpTimer = 0.f;     // temps restant d'affichage (s)
    double rawYVelocity = 0.0; // PlayerObject::m_yVelocity (valeur brute du moteur)
};

class PhysicsSampler {
public:
    static constexpr float kJumpShowTime = 0.35f;

    void reset();
    void update(PlayerObject* player, float dt);
    PhysicsSample const& get() const { return m_s; }

private:
    PhysicsSample m_s;
    CCPoint m_prevPos {0, 0};
    CCPoint m_prevVel {0, 0};
    bool m_hasPrevPos = false;
    bool m_hasPrevVel = false;
    bool m_wasOnGround = false;
    int m_gSamples = 0;
};
