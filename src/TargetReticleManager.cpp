#include "TargetReticleManager.h"
#include "Offsets.h"
#include "_ts_SKSEFunctions.h"
#include "ControlsManager.h"
#include "APIManager.h"

namespace FCSE {
    void TargetReticleManager::Initialize()
    {
        m_isReticleLocked = false;
        m_isWidgetActive = false;
        m_reticleTarget = nullptr;
        
        if (m_isInitialized) {
            log::info("FCSE - {}: CombatTargetReticle already initialized", __func__);
            return;
        }

        if (APIs::TrueHUD) {
            APIs::TrueHUD->LoadCustomWidgets(SKSE::GetPluginHandle(), "IntuitiveDragonRideControl/IDRC_Widgets.swf"sv, [this](TRUEHUD_API::APIResult a_apiResult) {
                if (a_apiResult == TRUEHUD_API::APIResult::OK) {
                    log::info("FCSE - TrueHUD API: IDRC TargetReticle loaded successfully.");
                    APIs::TrueHUD->RegisterNewWidgetType(SKSE::GetPluginHandle(), 'FCSE');
                    this->m_isInitialized = true;
                }
            });
        }
    }

    void TargetReticleManager::SetMaxTargetDistance(float a_maxReticleDistance) {
        m_maxReticleDistance = a_maxReticleDistance;
    }

    void TargetReticleManager::SetDistanceMultiplierSmall(float a_distanceMultiplierSmall) {
        m_distanceMultiplierSmall = a_distanceMultiplierSmall;
    }

    void TargetReticleManager::SetDistanceMultiplierLarge(float a_distanceMultiplierLarge) {
        m_distanceMultiplierLarge = a_distanceMultiplierLarge;
    }

    void TargetReticleManager::SetDistanceMultiplierExtraLarge(float a_distanceMultiplierExtraLarge) {
        m_distanceMultiplierExtraLarge = a_distanceMultiplierExtraLarge;
    }
    void TargetReticleManager::SetMaxTargetScanAngle(float a_maxTargetScanAngle) {
        m_maxTargetScanAngle = a_maxTargetScanAngle;
    }

    void TargetReticleManager::Update()
    {
        if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }

        if (!APIs::TrueHUD) {
            return;
        }

        if (!m_isInitialized) {
            log::warn("FCSE - {}: TargetReticle not initialized", __func__);
            return;
        }

        if (m_reticleMode == ReticleMode::kOff) {
            DisposeReticle(true);
            return;
        }

        RE::Actor* newTarget = nullptr;

        RE::Actor* player = RE::PlayerCharacter::GetSingleton();
        if (m_isReticleLocked && m_reticleTarget && m_reticleTarget->GetDistance(player) > m_maxReticleDistance * GetDistanceRaceSizeMultiplier(m_reticleTarget->GetRace())) {
            ToggleLockReticle();
            m_reticleTarget = nullptr;
        }

        if (!m_isReticleLocked) {
            newTarget = GetSelectedActor();      

            if ((!m_isWidgetActive && newTarget) ||
                m_reticleTarget != newTarget)
            {
                m_reticleTarget = newTarget;
                SetReticleTarget();
            }
        }
        UpdateReticleState();
    }


    RE::Actor* TargetReticleManager::GetCurrentTarget() {
        if (!IsReticleLocked()) {
            return GetSelectedActor();
        }
        return m_reticleTarget; 
    }

    RE::Actor* TargetReticleManager::GetSelectedActor() const {
        auto* playerActor = RE::PlayerCharacter::GetSingleton();
        auto* processLists = RE::ProcessLists::GetSingleton();
        auto* playerCamera = RE::PlayerCamera::GetSingleton();

        if (!playerActor) {
            log::error("FCSE - {}: PlayerActor is null", __func__);
            return nullptr;
        }
        if (!processLists) {
            log::error("FCSE - {}: ProcessLists is null", __func__);
            return nullptr;
        }
        if (!playerCamera) {
            log::error("FCSE - {}: PlayerCamera is null", __func__);
            return nullptr;
        }

        auto cameraPos = playerCamera->cameraRoot->world.translate;
        auto playerPos = playerActor->GetPosition();

        auto root = playerCamera->cameraRoot;
        const auto& worldTransform = root->world; // RE::NiTransform

        // The forward vector is the third column of the rotation matrix
        RE::NiPoint3 cameraForward = worldTransform.rotate * RE::NiPoint3{ 0.0f, 1.0f, 0.0f };
        float cameraToPlayerDistance = playerPos.GetDistance(cameraPos);

        std::vector<RE::Actor*> excludeActors;

        RE::Actor* selectedActor = nullptr;
        for (auto handle : processLists->highActorHandles) {
            auto actor = handle.get().get();
            if (actor
                && std::find(excludeActors.begin(), excludeActors.end(), actor) == excludeActors.end() 
                && actor->Get3D() 
                && (m_maxReticleDistance <= 0.0f || actor->GetPosition().GetDistance(playerPos) <= m_maxReticleDistance * GetDistanceRaceSizeMultiplier(actor->GetRace()))) {

                RE::NiPoint3 actorPos = actor->GetPosition();
                float fAngleForward = _ts_SKSEFunctions::GetAngleBetweenVectors(actorPos - cameraPos, cameraForward);
                float fAngleBackwards = _ts_SKSEFunctions::GetAngleBetweenVectors(playerPos - actorPos, -cameraForward);
                float distanceToPlayer = actorPos.GetDistance(playerPos);
                
                // forward cone: starting at camerapos, opening towards playerpos with angle m_maxTargetScanAngle
                bool inForwardCone = (fAngleForward <= m_maxTargetScanAngle);
                // backwards cone: starting at player, opening towards camerapos with angle m_maxTargetScanAngle
                bool inBackwardCone = (fAngleBackwards >= 180.f - m_maxTargetScanAngle) && (distanceToPlayer < cameraToPlayerDistance);
                if (inForwardCone || inBackwardCone || m_maxTargetScanAngle <= 0.0f) {
                    if (selectedActor) {
                        if (actor->GetPosition().GetDistance(playerPos) < selectedActor->GetPosition().GetDistance(playerPos)) {
                            selectedActor = actor;
                        }
                    } else {
                        selectedActor = actor;
                    }
                }
            }
        }

        return selectedActor;
    }


    bool TargetReticleManager::IsReticleLocked() const {
        return m_isReticleLocked;
    }

    void TargetReticleManager::ToggleLockReticle() {
        if (!APIs::TrueHUD) {
            log::info("FCSE - {}: TrueHUD API not available", __func__);
            return;
        }

        if (m_reticleTarget || m_isReticleLocked) {
            m_isReticleLocked = !m_isReticleLocked;
            std::string sMessage = "Zoom " + std::string(m_isReticleLocked ? "locked on " + std::string(m_reticleTarget->GetName()) + "." : "unlocked.");
            RE::DebugNotification(sMessage.c_str());
        } else {
            std::string sMessage = "No zoom target to lock on.";
            RE::DebugNotification(sMessage.c_str());
        }

        UpdateReticleState();
    }

    void TargetReticleManager::UpdateReticleState() {

        if (!m_reticleTarget) {
            DisposeReticle();
            return;
        }
        
        auto widget = m_TargetReticle.lock();
        if (!widget) {
            return;
        }
        
        widget->UpdateState(m_isReticleLocked, false, 0);
    }

    void TargetReticleManager::DisposeReticle(bool a_keepTarget) {
        if (!APIs::TrueHUD) {
            log::info("FCSE - {}: TrueHUD API not available", __func__);
            return;
        }

        if (!a_keepTarget) {
            m_reticleTarget = nullptr;
        }

        auto widget = m_TargetReticle.lock();
        if (!widget || !m_isWidgetActive) {
            return;
        }

        widget->WidgetReadyToRemove();

        APIs::TrueHUD->RemoveWidget(SKSE::GetPluginHandle(), 'FCSE', 0, TRUEHUD_API::WidgetRemovalMode::Normal);
        m_isWidgetActive = false;
    }

    void TargetReticleManager::SetReticleMode(ReticleMode a_mode) {

        if (m_reticleMode == kOff && a_mode == kOn && m_reticleTarget) {
           SetReticleTarget();
        }

        m_reticleMode = a_mode; 
    }


    void TargetReticleManager::SetReticleLockAnimationStyle(int a_style) {
        m_reticleLockAnimationStyle = a_style;
        auto widget = m_TargetReticle.lock();
        if (widget) {
            widget->SetReticleLockAnimationStyle(m_reticleLockAnimationStyle);
        }
    }

    void TargetReticleManager::SetReticleTarget() {
        if (!APIs::TrueHUD) {
            log::info("FCSE - {}: TrueHUD API not available", __func__);
            return;
        }

        if (!m_reticleTarget) {
            DisposeReticle();
            return;
        }

        auto actorHandle = m_reticleTarget->GetHandle();
        if (!actorHandle) {
            log::warn("FCSE - {}: Actor handle is invalid", __func__);
            DisposeReticle();
            return;
        }

        auto targetPoint = GetTargetPoint(m_reticleTarget);
        if (!targetPoint) {
            log::warn("FCSE - {}: Target point is nullptr", __func__);
            DisposeReticle();
            return;
        }

        auto widget = m_TargetReticle.lock();
        if (widget) {
            widget->ChangeTarget(actorHandle, targetPoint);
        } else {
            widget = std::make_shared<TargetReticle>(actorHandle.native_handle(), actorHandle, targetPoint,
                                                            m_reticleLockAnimationStyle);
            if (!widget) {
                log::warn("FCSE - {}: Failed to create TargetReticle widget", __func__);
                return;
            }
            
            m_TargetReticle = widget;
            APIs::TrueHUD->AddWidget(SKSE::GetPluginHandle(), 'FCSE', 0, "IDRC_TargetReticle", widget);
            }
        m_isWidgetActive = true;

        UpdateReticleState();
    }

    RE::NiPointer<RE::NiAVObject> TargetReticleManager::GetTargetPoint(RE::Actor* a_actor) const {
        RE::NiPointer<RE::NiAVObject> targetPoint = nullptr;

        if (!a_actor) {
            return nullptr;
        }

        auto race = a_actor->GetRace();
        if (!race) {
            return nullptr;
        }

        RE::BGSBodyPartData* bodyPartData = race->bodyPartData;
        if (!bodyPartData) {
            return nullptr;
        }

        auto actor3D = a_actor->Get3D2();
        if (!actor3D) {
            return nullptr;
        }
    
        RE::BGSBodyPart* bodyPart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kHead];
        if (!bodyPart) {
            bodyPart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kTotal];
        }
        if (bodyPart) {
            targetPoint = RE::NiPointer<RE::NiAVObject>(NiAVObject_LookupBoneNodeByName(actor3D, bodyPart->targetName, true));
        }

        return targetPoint;
    }

    float TargetReticleManager::GetDistanceRaceSizeMultiplier(RE::TESRace* a_race) const {
        if (a_race) {
            switch (a_race->data.raceSize.get())
            {
            case RE::RACE_SIZE::kMedium:
            default:
                return 1.f;
            case RE::RACE_SIZE::kSmall:
                return m_distanceMultiplierSmall;
            case RE::RACE_SIZE::kLarge:
                return m_distanceMultiplierLarge;
            case RE::RACE_SIZE::kExtraLarge:
                return m_distanceMultiplierExtraLarge;
            }
        }

        return 1.f;
    }
}  // namespace FCSE
