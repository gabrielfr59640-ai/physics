#include "PhysicsSampler.hpp"
#include <algorithm>
#include <cmath>

void PhysicsSampler::reset() {
    m_s = PhysicsSample{};
    m_hasPrevPos = false;
    m_hasPrevVel = false;
    m_wasOnGround = false;
    m_gSamples = 0;
}

static PlayerMode detectMode(PlayerObject* p) {
    if (p->m_isShip)   return PlayerMode::Ship;
    if (p->m_isBird)   return PlayerMode::Ufo;
    if (p->m_isBall)   return PlayerMode::Ball;
    if (p->m_isDart)   return PlayerMode::Wave;
    if (p->m_isRobot)  return PlayerMode::Robot;
    if (p->m_isSpider) return PlayerMode::Spider;
    if (p->m_isSwing)  return PlayerMode::Swing;
    return PlayerMode::Cube;
}

void PhysicsSampler::update(PlayerObject* p, float dt) {
    if (!p || dt <= 0.f || dt > 0.1f) return;

    CCPoint pos = p->getPosition();
    m_s.onGround = p->m_isOnGround;
    m_s.upsideDown = p->m_isUpsideDown;
    m_s.mini = p->m_vehicleSize < 1.f;
    m_s.mode = detectMode(p);
    m_s.gravityModel = m_s.mode == PlayerMode::Cube || m_s.mode == PlayerMode::Ball ||
                       m_s.mode == PlayerMode::Robot || m_s.mode == PlayerMode::Spider;
    m_s.rawYVelocity = p->m_yVelocity;
    m_s.jumpTimer = std::max(0.f, m_s.jumpTimer - dt);

    // Premiere frame ou teleportation (respawn, portail de teleportation...)
    if (!m_hasPrevPos || ccpDistance(pos, m_prevPos) > 600.f) {
        m_prevPos = pos;
        m_s.pos = pos;
        m_s.vel = {0, 0};
        m_s.acc = {0, 0};
        m_hasPrevPos = true;
        m_hasPrevVel = false;
        m_wasOnGround = m_s.onGround;
        m_s.valid = true;
        return;
    }

    CCPoint vNow = (pos - m_prevPos) * (1.f / dt);
    m_s.pos = pos;
    m_s.vel = vNow;

    if (m_hasPrevVel) {
        CCPoint aNow = (vNow - m_prevVel) * (1.f / dt);
        float gravSign = m_s.upsideDown ? 1.f : -1.f; // sens de g sur l'axe Y ecran
        float dvY = vNow.y - m_prevVel.y;
        float opposed = dvY * -gravSign; // > 0 : variation de vitesse contre la gravite

        bool jump = false;
        if (m_s.gravityModel) {
            bool leftGroundUp = m_wasOnGround && !m_s.onGround && (vNow.y * -gravSign) > 0.f;
            bool strongKick = m_s.gKnown && opposed > 4.f * m_s.g * dt;
            jump = leftGroundUp || strongKick;
        }

        if (jump) {
            // Impulsion : le moteur impose directement un changement de vitesse.
            m_s.jumpDv = dvY;
            m_s.jumpTimer = kJumpShowTime;
            // acceleration non mise a jour sur cette frame (non definie pour un Dirac)
        } else {
            m_s.acc.x += (aNow.x - m_s.acc.x) * 0.5f;
            m_s.acc.y += (aNow.y - m_s.acc.y) * 0.5f;

            // Estimation de g : uniquement en chute libre, modele gravitaire, accel dans le sens de g
            if (m_s.gravityModel && !m_s.onGround && aNow.y * gravSign > 0.f) {
                float mag = std::abs(aNow.y);
                if (!m_s.gKnown || (mag > 0.5f * m_s.g && mag < 1.5f * m_s.g)) {
                    m_s.g = (m_gSamples == 0) ? mag : m_s.g + (mag - m_s.g) * 0.1f;
                    if (++m_gSamples >= 8) m_s.gKnown = true;
                }
            }
        }
    }

    m_prevPos = pos;
    m_prevVel = vNow;
    m_hasPrevVel = true;
    m_wasOnGround = m_s.onGround;
    m_s.valid = true;
}
