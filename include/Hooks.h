#pragma once

#include "RE/F/FreeCameraState.h"
namespace Hooks
{
	class FreeCameraStateHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> FreeCameraStateVtbl{ RE::VTABLE_FreeCameraState[0] };
			_OnEnterState = FreeCameraStateVtbl.write_vfunc(0x1, OnEnterState);
			_OnExitState = FreeCameraStateVtbl.write_vfunc(0x2, OnExitState);
			_Update = FreeCameraStateVtbl.write_vfunc(0x3, Update);
		}

	private:
		static void OnEnterState(RE::FreeCameraState* a_this);
		static void OnExitState(RE::FreeCameraState* a_this);
        static void Update(RE::FreeCameraState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState);

		static inline REL::Relocation<decltype(OnEnterState)> _OnEnterState;
		static inline REL::Relocation<decltype(OnExitState)> _OnExitState;
		static inline REL::Relocation<decltype(Update)> _Update;
	};

    class LookHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> LookHandlerVtbl{ RE::VTABLE_LookHandler[0] };
			_ProcessThumbstick = LookHandlerVtbl.write_vfunc(0x2, ProcessThumbstick);
			_ProcessMouseMove = LookHandlerVtbl.write_vfunc(0x3, ProcessMouseMove);
		}

	private:
		static void ProcessThumbstick(RE::LookHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data);
		static void ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data);

		static inline REL::Relocation<decltype(ProcessThumbstick)> _ProcessThumbstick;
		static inline REL::Relocation<decltype(ProcessMouseMove)> _ProcessMouseMove;
	};
	
    class PlayerCameraHook
    {
    public:
        static void Hook()
        {
            REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(49876, 50809) };
            auto& trampoline = SKSE::GetTrampoline();
            _ToggleFreeCameraMode = trampoline.write_call<5>(hook.address() + 0x10, ToggleFreeCameraMode); // TBD: 0x10 is not the correct offset!!
        }

    private:
        static void ToggleFreeCameraMode(RE::PlayerCamera* a_this, bool a_freezeTime);

        static inline REL::Relocation<decltype(ToggleFreeCameraMode)> _ToggleFreeCameraMode;
    };

	void Install();
} // namespace Hooks	

