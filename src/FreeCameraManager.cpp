#include "FreeCameraManager.h"
#include "_ts_SKSEFunctions.h"
#include "Offsets.h"

namespace FCSE {

    void FreeCameraManager::Update() {
        if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }

        if (!m_isFreeCameraActive) {
            // FreeCamera activated via console command
            return;
        }

		RE::FreeCameraState* freeCameraState = GetFreeCameraState();
		if (!freeCameraState) {
            log::warn("FCSE - {}: Not in Free Camera State", __func__);
			return;
		}

		if (m_target) {
			float heading = m_target->GetHeading(false);

            RE::NiPoint3 position = m_target->GetPosition();
            auto targetPoint = GetTargetPoint(m_target);
            if (targetPoint) {
                RE::NiPoint3 targetWorldPos = targetPoint->world.translate;
            
                if (m_isTransitioning) {
                    TransitionTo(targetWorldPos, heading, 1.0f); // translate over 1 second
                    m_userTurning = false;
                    return;
                }
            
                freeCameraState->translation = targetWorldPos;
                freeCameraState->translation.x += 30.0f*std::sin(heading);
                freeCameraState->translation.y += 30.0f*std::cos(heading);

                if (!m_isAttachedToTarget) {
                    m_isAttachedToTarget = true;
                    freeCameraState->rotation.x = 0.0f;  // pitch
                    freeCameraState->rotation.y = heading; // yaw
                    m_yawOffset = 0.0f;
                }
                if (m_userTurning) {
                    m_yawOffset = freeCameraState->rotation.y - heading;
                } else {
                    freeCameraState->rotation.y = m_yawOffset + heading; 
                }

                // Clamp pitch
                freeCameraState->rotation.x = _ts_SKSEFunctions::NormalRelativeAngle(freeCameraState->rotation.x);
                freeCameraState->rotation.x = std::clamp(freeCameraState->rotation.x, - 0.45f * PI, 0.4f * PI);
                
                // Clamp yaw
                float relativeYaw = _ts_SKSEFunctions::NormalRelativeAngle(freeCameraState->rotation.y - heading);
                relativeYaw = std::clamp(relativeYaw, -0.5f * PI, 0.5f * PI);
                freeCameraState->rotation.y = _ts_SKSEFunctions::NormalRelativeAngle(heading + relativeYaw);
            } else {
                log::warn("FCSE - {}: No target point found", __func__);
            }
		} else {
			log::info("FCSE - {}: No target found", __func__);
		}

        m_userTurning = false; // reset flag. Is set to true in LookHook::ProcessMouseMove() in case of user-triggered camera rotation

    }

    void FreeCameraManager::FindTarget() {
        m_target = _ts_SKSEFunctions::FindClosestActorInCameraDirection(50.f, 8000.f, false);

log::info("FCSE - {}: Target found: {}", __func__, m_target ? m_target->GetName() : "None");
    }

    void FreeCameraManager::TransitionTo(RE::NiPoint3 a_targetPos, float a_targetYaw, float a_transitionTime) {

        if (!m_isTransitioning) {
            return;
        }

        RE::FreeCameraState* freeCameraState = GetFreeCameraState();
        if (!freeCameraState) {
            log::warn("FCSE - {}: Not in Free Camera State", __func__);
            return;
        }
         
        float realTimeDeltaTime = _ts_SKSEFunctions::GetRealTimeDeltaTime() < 0.05f ? _ts_SKSEFunctions::GetRealTimeDeltaTime() : 0.05f;;

        float transitionDelta = realTimeDeltaTime / a_transitionTime;
        m_percentageTransitioned += transitionDelta;

        if (m_percentageTransitioned >= 1.0f) {
            m_percentageTransitioned = 0.0f;
            freeCameraState->translation = a_targetPos;
            freeCameraState->rotation.x = 0.0f;
            freeCameraState->rotation.y = a_targetYaw;
            m_isTransitioning = false;
            return;
        }

        // translation
        RE::NiPoint3 currentPos = freeCameraState->translation;
        RE::NiPoint3 transitionVec = a_targetPos - currentPos;

        float denominator = std::max(1.0f - m_percentageTransitioned, 0.0001f);
        RE::NiPoint3 step = transitionVec * transitionDelta / denominator;

        freeCameraState->translation += step;

        // rotation
        RE::BSTPoint2<float> targetRotation;
        targetRotation.x = 0.0f;
        targetRotation.y = a_targetYaw;
        freeCameraState->rotation = ComputeRotation(transitionVec, transitionDelta, freeCameraState->rotation, targetRotation);
    }

    RE::BSTPoint2<float> FreeCameraManager::ComputeRotation(RE::NiPoint3& a_transitionVec, float a_transitionDelta, RE::BSTPoint2<float>& a_currentRotation, RE::BSTPoint2<float>& a_targetRotation)
    {
        // Three-phase rotation transition
        constexpr float transitionStart = 0.3f;
        constexpr float transitionEnd = 0.7f;
    
        RE::NiPoint3 movementDir = a_transitionVec;
        movementDir.z = 0.0f;
        float movementYaw = std::atan2(movementDir.x, movementDir.y);

        float horizontalDist = std::sqrt(a_transitionVec.x * a_transitionVec.x + a_transitionVec.y * a_transitionVec.y);
        float movementPitch = std::atan2(-a_transitionVec.z, horizontalDist);

        RE::BSTPoint2<float> newRotation = a_currentRotation;
        if (transitionStart > 0.0f && m_percentageTransitioned < transitionStart) {
            // Phase 1: Transition to movement direction
            float pitchStep = ComputeAngleStep(a_transitionDelta, 0.0f, transitionStart, a_currentRotation.x, movementPitch);
            float yawStep = ComputeAngleStep(a_transitionDelta, 0.0f, transitionStart, a_currentRotation.y, movementYaw);

            newRotation.x = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.x + pitchStep);
            newRotation.y = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.y + yawStep);

        } else if (m_percentageTransitioned < transitionEnd) {
            // Phase 2: Track movement direction
            newRotation.x = _ts_SKSEFunctions::NormalRelativeAngle(movementPitch);
            newRotation.y = _ts_SKSEFunctions::NormalRelativeAngle(movementYaw);
        } else {
            // Phase 3: Transition to target angle
            float yawStep = ComputeAngleStep(a_transitionDelta, transitionEnd, 1.0f, a_currentRotation.y, a_targetRotation.y);
            float pitchStep = ComputeAngleStep(a_transitionDelta, transitionEnd, 1.0f, a_currentRotation.x, a_targetRotation.x);

            newRotation.x = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.x + pitchStep);
            newRotation.y = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.y + yawStep);
        }

        return newRotation;
    }

    float FreeCameraManager::ComputeAngleStep(float a_transitionDelta, float a_transitionStart, float a_transitionEnd, float a_currentAngle, float a_targetAngle) {
        if (a_transitionEnd <= a_transitionStart + 0.0001f) {
            return a_targetAngle - a_currentAngle;
        }
        float angleProgress = (m_percentageTransitioned - a_transitionStart)/ (a_transitionEnd - a_transitionStart);
        float smoothProgress = _ts_SKSEFunctions::SShapeFromLinear(angleProgress, 0.3f, 0.7f);

        float angleDelta = _ts_SKSEFunctions::NormalRelativeAngle(a_targetAngle - a_currentAngle);

        float angleTransitionDelta = a_transitionDelta / (a_transitionEnd - a_transitionStart);
        float angleDenominator = std::max(1.0f - smoothProgress, 0.0001f);
        float angleStep = angleDelta * angleTransitionDelta / angleDenominator;

        if (angleDelta >= 0.f && angleStep > angleDelta) {
            angleStep = angleDelta;
        }
        if (angleDelta < 0.f && angleStep < angleDelta) {
            angleStep = angleDelta;
        }

        return angleStep;
    }

    void FreeCameraManager::ToggleFreeCamera() {
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            return;
        }

        bool isInFreeCameraMode = playerCamera->IsInFreeCameraMode();
        if (isInFreeCameraMode && !m_isFreeCameraActive) {
            // FreeCamera activated via console command
            Activate(false);
            return;
        }

        if (!m_target) {
            Activate(false);
            return;
        }

        m_isFreeCameraToggled = true;
        playerCamera->ToggleFreeCameraMode(false);
    }

    void FreeCameraManager::Activate(bool a_activate) {
        m_isFreeCameraActive = a_activate;
        m_isAttachedToTarget = false;
        m_percentageTransitioned = 0.0f;
        m_isTransitioning = true;        
    }

    bool FreeCameraManager::IsActive() const {
        return m_isFreeCameraActive;
    }

    bool FreeCameraManager::GetFreeCameraToggled() const {
        return m_isFreeCameraToggled;
    }

    void FreeCameraManager::SetFreeCameraToggled(bool a_toggled) {
        m_isFreeCameraToggled = a_toggled;
    }

    bool FreeCameraManager::IsCameraLocked() {
        return m_cameraLocked;
    }

    void FreeCameraManager::SetUserTurning(bool a_turning) {
        m_userTurning = a_turning;
    }

    void FreeCameraManager::DetachTarget() {
        m_isAttachedToTarget = false;
    }

    RE::FreeCameraState* FreeCameraManager::GetFreeCameraState() const {
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            return nullptr;
        }

        RE::FreeCameraState* freeCameraState = nullptr;
        if (playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kFree)) {
            freeCameraState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());
        }
        return freeCameraState;
    }

    RE::NiPointer<RE::NiAVObject> FreeCameraManager::GetTargetPoint(RE::Actor* a_actor) const {
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
} // namespace FCSE
