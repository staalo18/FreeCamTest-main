#pragma once
// Offsets are from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

// functions
typedef RE::NiAVObject*(__fastcall* tNiAVObject_LookupBoneNodeByName)(RE::NiAVObject* a_this, const RE::BSFixedString& a_name, bool a3);
static REL::Relocation<tNiAVObject_LookupBoneNodeByName> NiAVObject_LookupBoneNodeByName{ RELOCATION_ID(74481, 76207) };

typedef void(__fastcall* tNiQuaternion_SomeRotationManipulation)(RE::NiQuaternion& a1, float a2, float a3, float a4);
static REL::Relocation<tNiQuaternion_SomeRotationManipulation> NiQuaternion_SomeRotationManipulation{ RELOCATION_ID(69466, 70843) };
