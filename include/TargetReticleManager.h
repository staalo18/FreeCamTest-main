#pragma once

#include "TargetReticle.h"

namespace FCSE {
    class TargetReticleManager {
        public:
            enum ReticleMode : std::uint32_t {
                kOff = 0,
                kOn = 1
            };

            static TargetReticleManager& GetSingleton() {
                static TargetReticleManager instance;
                return instance;
            }
            TargetReticleManager(const TargetReticleManager&) = delete;
            TargetReticleManager& operator=(const TargetReticleManager&) = delete;

            void Initialize();

            void SetMaxTargetDistance(float a_maxReticleDistance);
            void SetDistanceMultiplierSmall(float a_distanceMultiplierSmall);
            void SetDistanceMultiplierLarge(float a_distanceMultiplierLarge);
            void SetDistanceMultiplierExtraLarge(float a_distanceMultiplierExtraLarge);
            void SetMaxTargetScanAngle(float a_maxTargetScanAngle);

            void Update();

            void SetReticleMode(ReticleMode a_mode);

            void SetReticleLockAnimationStyle(int a_style);

            void ToggleLockReticle();

            void DisposeReticle(bool a_keepTarget = false);

            RE::Actor* GetCurrentTarget();

            RE::Actor* GetSelectedActor() const;
            
            bool IsReticleLocked () const;

            RE::NiPointer<RE::NiAVObject> GetTargetPoint(RE::Actor* a_actor) const;

        private:
            TargetReticleManager() = default;

            void UpdateReticleState();

            void SetReticleTarget();

            float GetDistanceRaceSizeMultiplier(RE::TESRace* a_race) const;

            std::weak_ptr<TargetReticle> m_TargetReticle;
            bool m_isInitialized = false;
            ReticleMode m_reticleMode = ReticleMode::kOn;
            bool m_isReticleLocked = false;
            bool m_isWidgetActive = false;
            float m_maxReticleDistance = 8000.f;
            float m_distanceMultiplierSmall = 1.0f;
            float m_distanceMultiplierLarge = 2.0f;
            float m_distanceMultiplierExtraLarge = 4.0f;
            float m_maxTargetScanAngle = 7.0f;
            RE::Actor* m_reticleTarget = nullptr;
            int m_reticleLockAnimationStyle = 0;
    }; // class TargetReticleManager
}  // namespace FCSE
