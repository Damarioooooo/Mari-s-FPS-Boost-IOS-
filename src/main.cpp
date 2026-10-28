#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>

using namespace geode::prelude;

namespace {

bool on(char const* key) {
    return Mod::get()->getSettingValue<bool>(key);
}

// Interval to go back to after the pause menu lowered it. 0 = nothing to restore.
double g_savedInterval = 0.0;

void applyFps() {
    if (!on("override-fps")) return;
    auto fps = Mod::get()->getSettingValue<int64_t>("target-fps");
    if (fps < 30) fps = 30;
    CCDirector::sharedDirector()->setAnimationInterval(1.0 / static_cast<double>(fps));
}

void restoreFps() {
    if (g_savedInterval > 0.0) {
        CCDirector::sharedDirector()->setAnimationInterval(g_savedInterval);
        g_savedInterval = 0.0;
    }
    applyFps();
}

void applyVisuals(PlayLayer* pl) {
    if (on("hide-background") && pl->m_background) pl->m_background->setVisible(false);
    if (on("hide-middleground") && pl->m_middleground) pl->m_middleground->setVisible(false);
    if (on("hide-ground")) {
        if (pl->m_groundLayer) pl->m_groundLayer->setVisible(false);
        if (pl->m_groundLayer2) pl->m_groundLayer2->setVisible(false);
    }
    if (on("hide-trails")) {
        if (pl->m_player1 && pl->m_player1->m_regularTrail) pl->m_player1->m_regularTrail->setVisible(false);
        if (pl->m_player2 && pl->m_player2->m_regularTrail) pl->m_player2->m_regularTrail->setVisible(false);
    }
}

void hideGlow(PlayLayer* pl) {
    if (!on("hide-glow") || !pl->m_objects) return;
    for (auto obj : CCArrayExt<GameObject*>(pl->m_objects)) {
        if (obj && obj->m_glowSprite) obj->m_glowSprite->setVisible(false);
    }
}

} // namespace

$on_mod(Loaded) {
    applyFps();
    listenForSettingChanges<bool>("override-fps", [](bool) { applyFps(); });
    listenForSettingChanges<int64_t>("target-fps", [](int64_t) { applyFps(); });
}

class $modify(FpsMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        restoreFps();
        return true;
    }
};

class $modify(FpsPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        restoreFps();
        applyVisuals(this);
        hideGlow(this);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        applyVisuals(this);
    }

    void onQuit() {
        PlayLayer::onQuit();
        if (on("free-memory")) {
            // Next frame, once the scene switch has dropped the level's nodes.
            Loader::get()->queueInMainThread([] {
                CCTextureCache::sharedTextureCache()->removeUnusedTextures();
            });
        }
    }
};

class $modify(FpsPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (on("pause-fps-saver")) {
            if (g_savedInterval <= 0.0) {
                g_savedInterval = CCDirector::sharedDirector()->getAnimationInterval();
            }
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / 30.0);
        }
    }

    void onResume(CCObject* sender) {
        restoreFps();
        PauseLayer::onResume(sender);
    }
};

class $modify(FpsParticles, CCParticleSystemQuad) {
    void draw() {
        // Only inside a level, so menu particles still look normal.
        if (PlayLayer::get() && on("disable-particles")) return;
        CCParticleSystemQuad::draw();
    }
};
