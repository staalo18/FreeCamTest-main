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

        if (m_transitionMode == FreeCameraTransitionMode::kToPrevious) {
            TransitionToPrevious();
        } else if (m_transitionMode == FreeCameraTransitionMode::kToTarget) {
            TransitionToTarget();
        } else {
            UpdateFreeCamera();
        }
    }

    void FreeCameraManager::StartFreeCameraMode() {
        RE::PlayerCamera* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            log::error("FCSE - {}: PlayerCamera singleton not found", __func__);
            return;
        }

        m_previousCameraPos = _ts_SKSEFunctions::GetCameraPos();

        m_prevYaw = 0.0f;
        m_prevPitch = 0.0f;
        RE::ThirdPersonState* thirdPersonState = nullptr;
        if (playerCamera->currentState) {
            m_previousCameraState = playerCamera->currentState->id;

            if (playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
                playerCamera->currentState->id == RE::CameraState::kMount ||
                playerCamera->currentState->id == RE::CameraState::kDragon) {
                thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
                m_prevYaw = _ts_SKSEFunctions::GetCameraYaw();
                m_prevPitch = _ts_SKSEFunctions::GetCameraPitch();
            }
        } else{
            log::warn("FCSE - {}: PlayerCamera currentState is null", __func__);
        }

        m_prevFreeRotation = thirdPersonState ? thirdPersonState->freeRotation : RE::NiPoint2{ 0.0f, 0.0f };

        m_isFreeCameraToggled = true;
        playerCamera->ToggleFreeCameraMode(false);
    }
    
    void FreeCameraManager::StopFreeCameraMode() {
        m_percentageTransitioned = 0.0f;
        m_transitionMode = FreeCameraTransitionMode::kToPrevious;
    }

    void FreeCameraManager::FindTarget() {
        m_target = _ts_SKSEFunctions::FindClosestActorInCameraDirection(50.f, 8000.f, false);

log::info("FCSE - {}: Target found: {}", __func__, m_target ? m_target->GetName() : "None");
    }

    void FreeCameraManager::TransitionToTarget() {
        if (m_transitionMode != FreeCameraTransitionMode::kToTarget) {
            return;
        }

        if (!m_target) {
            log::warn("FCSE - {}: No target to transition to", __func__);
            return;
        }

        auto targetPoint = GetTargetPoint(m_target);
        if (targetPoint) {
            RE::BSTPoint2<float> targetRotation{0.0f, m_target->GetHeading(false)};
            float fMinTime = 0.5f; // 0.5secs
            float fMaxTime = 2.0f; // 2.0secs
            float transitionTime = ComputeTransitionTime(2000.f, 20000.f, fMinTime, fMaxTime); // between fMinTime and fMaxTime seconds
            float rotationToMovement_End = 0.5f * fMinTime / transitionTime; // The time the camera finishes rotating towards the movement direction, normalized to [0,1]
            float rotationToTarget_Start = 1.0f - rotationToMovement_End; // the time the camera starts rotating towards the target, normalized to [0,1]
            TransitionTo(targetPoint->world.translate, targetRotation, transitionTime, rotationToMovement_End, rotationToTarget_Start);
            m_yawOffset = 0.0f;
        }
    }

    void FreeCameraManager::TransitionToPrevious() {
        if (m_transitionMode != FreeCameraTransitionMode::kToPrevious) {
            return;
        }

        RE::BSTPoint2<float> previousRotation{-m_prevPitch, m_prevYaw};
        float transitionTime = ComputeTransitionTime(2000.f, 20000.f, 0.5f, 2.0f);
        TransitionTo(m_previousCameraPos, previousRotation, transitionTime, 0.0f, 0.0f);
        m_yawOffset = 0.0f;
    }

    float FreeCameraManager::ComputeTransitionTime(float a_minDist, float a_maxDist, float a_minTime, float a_maxTime) {
        if (!m_target) {
            log::warn("FCSE - {}: No target to compute transition time to", __func__);
            return 1.0f;
        }

        float distance = _ts_SKSEFunctions::GetCameraPos().GetDistance(m_target->GetPosition());

        float relDistance = (distance - a_minDist) / (a_maxDist - a_minDist);
        relDistance = std::clamp(relDistance, 0.0f, 1.0f);
        
        float transitionTime = a_minTime + (a_maxTime - a_minTime) * relDistance;

        return transitionTime;
    }
    
    void FreeCameraManager::UpdateFreeCamera() {
        if (!m_target) {
            log::warn("FCSE - {}: No target to transition to", __func__);
            return;
        }

		RE::FreeCameraState* freeCameraState = GetFreeCameraState();
		if (!freeCameraState) {
            log::warn("FCSE - {}: Not in Free Camera State", __func__);
			return;
		}

        float heading = m_target->GetHeading(false);

        RE::NiPoint3 position = m_target->GetPosition();
        auto targetPoint = GetTargetPoint(m_target);
        if (targetPoint) {
            RE::NiPoint3 targetWorldPos = targetPoint->world.translate;
        
            freeCameraState->translation = targetWorldPos;
            freeCameraState->translation.x += 30.0f*std::sin(heading);
            freeCameraState->translation.y += 30.0f*std::cos(heading);

            if (m_userTurning) {
                m_yawOffset = freeCameraState->rotation.y - heading;

                // reset m_userTurning - Is set to true in LookHook::ProcessMouseMove() in case of user-triggered camera rotation
                m_userTurning = false;
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
    }

    void FreeCameraManager::TransitionTo(RE::NiPoint3 a_targetPos, 
        RE::BSTPoint2<float> a_targetRotation, float a_transitionTime, 
        float a_rotationToMovement_End, float a_rotationToTarget_Start) {

        if (m_transitionMode == FreeCameraTransitionMode::kNone) {
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
            freeCameraState->rotation.x = a_targetRotation.x; // pitch
            freeCameraState->rotation.y = a_targetRotation.y; // yaw

            if  (m_transitionMode == FreeCameraTransitionMode::kToPrevious) {
                RE::PlayerCamera* playerCamera = RE::PlayerCamera::GetSingleton();
                if (playerCamera) {
                    m_isFreeCameraToggled = true;
                    playerCamera->ToggleFreeCameraMode(false);

                    
                    if (playerCamera->currentState && playerCamera->currentState->id == m_previousCameraState) {
                        RE::ThirdPersonState* thirdPersonState = nullptr;
                        if (playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
                            playerCamera->currentState->id == RE::CameraState::kMount ||
                            playerCamera->currentState->id == RE::CameraState::kDragon) {
                            thirdPersonState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());

                            if (thirdPersonState) {
                                thirdPersonState->freeRotation = m_prevFreeRotation;
                            }
                        }
                    }

                } else {
                    log::error("FCSE - {}: PlayerCamera singleton not found", __func__);
                }
            }
            m_transitionMode = FreeCameraTransitionMode::kNone;
            return;
        }

        // translation
        RE::NiPoint3 currentPos = freeCameraState->translation;
        RE::NiPoint3 transitionVec = a_targetPos - currentPos;

        float denominator = std::max(1.0f - m_percentageTransitioned, 0.0001f);
        RE::NiPoint3 step = transitionVec * transitionDelta / denominator;

        freeCameraState->translation += step;

        // rotation
        freeCameraState->rotation = ComputeRotation(transitionVec, transitionDelta, 
            freeCameraState->rotation, a_targetRotation, a_rotationToMovement_End, a_rotationToTarget_Start);
    }

    RE::BSTPoint2<float> FreeCameraManager::ComputeRotation(
        RE::NiPoint3& a_transitionVec, float a_transitionDelta, 
        RE::BSTPoint2<float>& a_currentRotation, RE::BSTPoint2<float>& a_targetRotation,
        float a_rotationToMovement_End, float a_rotationToTarget_Start) {
        // Three-phase rotation transition
    
        RE::NiPoint3 movementDir = a_transitionVec;
        movementDir.z = 0.0f;
        float movementYaw = std::atan2(movementDir.x, movementDir.y);

        float horizontalDist = std::sqrt(a_transitionVec.x * a_transitionVec.x + a_transitionVec.y * a_transitionVec.y);
        float movementPitch = std::atan2(-a_transitionVec.z, horizontalDist);

        RE::BSTPoint2<float> newRotation = a_currentRotation;
        if (m_percentageTransitioned < a_rotationToMovement_End) {
            // Phase 1: Transition to movement direction
            float pitchStep = ComputeAngleStep(a_transitionDelta, 0.0f, a_rotationToMovement_End, a_currentRotation.x, movementPitch);
            float yawStep = ComputeAngleStep(a_transitionDelta, 0.0f, a_rotationToMovement_End, a_currentRotation.y, movementYaw);

            newRotation.x = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.x + pitchStep);
            newRotation.y = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.y + yawStep);

        } else if (m_percentageTransitioned < a_rotationToTarget_Start) {
            // Phase 2: Track movement direction
            newRotation.x = _ts_SKSEFunctions::NormalRelativeAngle(movementPitch);
            newRotation.y = _ts_SKSEFunctions::NormalRelativeAngle(movementYaw);
        } else {
            // Phase 3: Transition to target angle
            float yawStep = ComputeAngleStep(a_transitionDelta, a_rotationToTarget_Start, 1.0f, a_currentRotation.y, a_targetRotation.y);
            float pitchStep = ComputeAngleStep(a_transitionDelta, a_rotationToTarget_Start, 1.0f, a_currentRotation.x, a_targetRotation.x);

            newRotation.x = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.x + pitchStep);
            newRotation.y = _ts_SKSEFunctions::NormalRelativeAngle(a_currentRotation.y + yawStep);
        }

        return newRotation;
    }

    float FreeCameraManager::ComputeAngleStep(float a_transitionDelta, float a_rotationStart, float a_rotationEnd, float a_currentAngle, float a_targetAngle) {
        if (a_rotationEnd <= a_rotationStart + 0.0001f) {
            return a_targetAngle - a_currentAngle;
        }
        float angleProgress = (m_percentageTransitioned - a_rotationStart)/ (a_rotationEnd - a_rotationStart);
        float smoothProgress = _ts_SKSEFunctions::SCurveFromLinear(angleProgress, 0.3f, 0.7f);

        float angleDelta = _ts_SKSEFunctions::NormalRelativeAngle(a_targetAngle - a_currentAngle);

        float rotationDelta = a_transitionDelta / (a_rotationEnd - a_rotationStart);
        float angleDenominator = std::max(1.0f - smoothProgress, 0.0001f);
        float angleStep = angleDelta * rotationDelta / angleDenominator;

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
            SetActive(false);
            return;
        }

        if (!m_target) {
            SetActive(false);
            return;
        }

        if (isInFreeCameraMode) {
            StopFreeCameraMode();
        } else {
            StartFreeCameraMode();
        }
    }

    void FreeCameraManager::SetActive(bool a_activate) {
        m_isFreeCameraActive = a_activate;
        m_percentageTransitioned = 0.0f;
        if (a_activate) {
            m_transitionMode = FreeCameraTransitionMode::kToTarget;
        } else {
            m_transitionMode = FreeCameraTransitionMode::kNone;
        }
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
