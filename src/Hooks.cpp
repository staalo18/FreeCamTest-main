#include "Hooks.h"
#include "_ts_SKSEFunctions.h"
#include "FreeCameraManager.h"

namespace Hooks
{
	void Install()
	{
		log::info("Hooking...");

		LookHook::Hook();
		FreeCameraStateHook::Hook();
//        PlayerCameraHook::Hook();

		log::info("...success");
	}

	void FreeCameraStateHook::OnEnterState(RE::FreeCameraState* a_this)
	{
		_OnEnterState(a_this);

		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();

		if (cameraManager.GetFreeCameraToggled()) {
			// triggered by camera manager
log::info("FCSE - {}: Activating Free Camera via camera manager", __func__);
			cameraManager.Activate(true);
		} else {
			// triggered by console command
			// already in free camera mode, so just detach from target if any
log::info("FCSE - {}: Activating Free Camera via console command", __func__);
			cameraManager.Activate(false);
			cameraManager.DetachTarget();
		}

		cameraManager.SetFreeCameraToggled(false);
	}

	void FreeCameraStateHook::OnExitState(RE::FreeCameraState* a_this)
	{
		_OnExitState(a_this);
		auto& cameraManager = FCSE::FreeCameraManager::GetSingleton();

		if (cameraManager.GetFreeCameraToggled()) {
			cameraManager.Activate(false);
log::info("FCSE - {}: De-Activating Free Camera via camera manager", __func__);
		} else {
			if (cameraManager.IsActive()) {
				cameraManager.Activate(false);
//				RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
			}
//			cameraManager.Activate(false);
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
		auto& cameraLockManager = FCSE::FreeCameraManager::GetSingleton();
		if (a_event && a_event->IsRight() && cameraLockManager.IsCameraLocked())
		{
			return;
		}
		else
		{
			if (!RE::UI::GetSingleton()->GameIsPaused()) {
				cameraLockManager.SetUserTurning(true);
			}

			_ProcessThumbstick(a_this, a_event, a_data);
		}
	}

	void LookHook::ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& cameraLockManager = FCSE::FreeCameraManager::GetSingleton();
		if (a_event && cameraLockManager.IsCameraLocked())
		{
			return;
		}
		else
		{
			if (!RE::UI::GetSingleton()->GameIsPaused()) {
				cameraLockManager.SetUserTurning(true);
			}

			_ProcessMouseMove(a_this, a_event, a_data);
		}
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
        _ToggleFreeCameraMode(a_this, a_freezeTime);
    }
} // namespace Hooks
