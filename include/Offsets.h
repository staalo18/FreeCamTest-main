#pragma once
// Offsets are from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

// variables
static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address();                       // 2F4C910, 2FE75F0
static RE::NiRect<float>* g_viewPort = (RE::NiRect<float>*)RELOCATION_ID(519618, 406160).address();  // 2F4DED0, 2FE8B98

// functions
typedef RE::NiAVObject*(__fastcall* tNiAVObject_LookupBoneNodeByName)(RE::NiAVObject* a_this, const RE::BSFixedString& a_name, bool a3);
static REL::Relocation<tNiAVObject_LookupBoneNodeByName> NiAVObject_LookupBoneNodeByName{ RELOCATION_ID(74481, 76207) };

typedef void(__fastcall* tNiQuaternion_SomeRotationManipulation)(RE::NiQuaternion& a1, float a2, float a3, float a4);
static REL::Relocation<tNiQuaternion_SomeRotationManipulation> NiQuaternion_SomeRotationManipulation{ RELOCATION_ID(69466, 70843) };
