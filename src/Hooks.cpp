#include "Hooks.h"
#include "_ts_SKSEFunctions.h"
#include "FreeCameraManager.h"
#include "TargetReticleManager.h"

namespace Hooks
{
	void Install()
	{
		log::info("Hooking...");

		MainUpdateHook::Hook();
		LookHook::Hook();
		FreeCameraStateHook::Hook();
		MovementHook::Hook();
//        PlayerCameraHook::Hook();

		log::info("...success");
	}

	void MainUpdateHook::Nullsub()
	{
		_Nullsub();

		FCSE::TargetReticleManager::GetSingleton().Update();
	}

	void FreeCameraStateHook::OnEnterState(RE::FreeCameraState* a_this)
	{
		_OnEnterState(a_this);

		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();

		if (cameraManager.GetFreeCameraToggled()) {
			// triggered by camera manager
log::info("FCSE - {}: Activating Free Camera via camera manager", __func__);
			cameraManager.SetActive(true);
		} else {
			// triggered by console command
log::info("FCSE - {}: Activating Free Camera via console command", __func__);
			cameraManager.SetActive(false);
		}

		cameraManager.SetFreeCameraToggled(false);
	}

	void FreeCameraStateHook::OnExitState(RE::FreeCameraState* a_this)
	{
		_OnExitState(a_this);
		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();

		if (cameraManager.GetFreeCameraToggled()) {
			cameraManager.SetActive(false);
log::info("FCSE - {}: De-Activating Free Camera via camera manager", __func__);
		} else {
			if (cameraManager.IsActive()) {
				cameraManager.SetActive(false);
//				RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
			}
//			cameraManager.SetActive(false);
			// triggered by console command
log::info("FCSE - {}: De-Activating Free Camera via console command", __func__);
		}

		cameraManager.SetFreeCameraToggled(false);
	}

	void FreeCameraStateHook::Update(RE::FreeCameraState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
	{

		_Update(a_this, a_nextState);

		FCSE::FreeCameraManager::GetSingleton().Update();
	}

	void LookHook::ProcessThumbstick(RE::LookHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();

		if (!RE::UI::GetSingleton()->GameIsPaused()) {
			cameraManager.SetUserTurning(true);
		}

		_ProcessThumbstick(a_this, a_event, a_data);
	}

	void LookHook::ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();

		if (!RE::UI::GetSingleton()->GameIsPaused()) {
			cameraManager.SetUserTurning(true);
		}

		_ProcessMouseMove(a_this, a_event, a_data);
	}

	void MovementHook::ProcessThumbstick(RE::MovementHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();
		if (a_event && a_event->IsLeft() && cameraManager.IsActive()) {
			return;
		}

		_ProcessThumbstick(a_this, a_event, a_data);
	}

	void MovementHook::ProcessButton(RE::MovementHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		bool bRelevant = false;
		if (a_event)
		{
			auto& userEvent = a_event->QUserEvent();
			auto userEvents = RE::UserEvents::GetSingleton();

			if (userEvent == userEvents->forward || 
				userEvent == userEvents->back ||
				userEvent == userEvents->strafeLeft ||
				userEvent == userEvents->strafeRight) {
				bRelevant = a_event->IsPressed();
			}
		}
		
		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();
		if (bRelevant && cameraManager.IsActive()) {
			return;
		}

		_ProcessButton(a_this, a_event, a_data);
	}

    void PlayerCameraHook::ToggleFreeCameraMode(RE::PlayerCamera* a_this, bool a_freezeTime)
    {
log::info("FCSE - {}: ToggleFreeCameraMode called, freezeTime={}", __func__, a_freezeTime);
/*        
        auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();
        
        // Check if currently in free camera mode to determine direction of toggle
        bool wasInFreeCamera = a_this->IsInFreeCameraMode();
        
        // Set flag so OnEnterState/OnExitState knows this came from our code vs console
        if (cameraManager.IsActive() || cameraManager.GetFreeCameraToggled()) {
            cameraManager.SetFreeCameraToggled(true);
            log::info("FCSE - {}: Plugin-initiated toggle", __func__);
        } else {
            log::info("FCSE - {}: Console-initiated toggle", __func__);
        }
  */  
        // Call original function
//        _ToggleFreeCameraMode(a_this, a_freezeTime);
    }
} // namespace Hooks
