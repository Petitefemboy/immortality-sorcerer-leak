#include "../Hooked.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../Features/Miscellaneous/SkinChanger.hpp"
#include "../../Features/Rage/LagCompensation.hpp"
#include "../../Features/Visuals/EventLogger.hpp"
#include "../../Utils/tinyformat.h"
#include <intrin.h>

#include "../../Utils/Threading/threading.h"

ClientClass* CCSPlayerClass;
CreateClientClassFn oCreateCCSPlayer;
std::map< int, Hooked::PlayerHook > Hooked::player_hooks;

namespace Hooked
{
	void __fastcall PreDataUpdate(uintptr_t ecx, void* edx, int updateType);
	void __fastcall PostDataUpdate(uintptr_t ecx, void* edx, int updateType);

	void __fastcall DoExtraBonesProccesing(C_CSPlayer* ecx, void* edx, CStudioHdr* hdr, Vector* pos, Quaternion* rotations, matrix3x4_t* transforma, void* bone_list, void* ik_context) {
		g_Vars.globals.szLastHookCalled = XorStr("22");
		//printf( "debp called\n" );

		auto& hook = player_hooks[ecx->m_entIndex];

		using Fn = void(__thiscall*)(C_CSPlayer*, CStudioHdr*, Vector*, Quaternion*, matrix3x4_t*, void*, void*);
		auto _do_extra_bone_processing = hook.clientHook.VCall< Fn >(197);

		if (ecx->m_fEffects() & 8)
			return;

		auto animState = ecx->m_PlayerAnimState();

		if (!animState)
			_do_extra_bone_processing(ecx, hdr, pos, rotations, transforma, bone_list, ik_context);

		const auto backup_tickcount = *reinterpret_cast<int32_t*>(animState + 8);
		*reinterpret_cast<int32_t*>(animState + 8) = 0;
		_do_extra_bone_processing(ecx, hdr, pos, rotations, transforma, bone_list, ik_context);
		*reinterpret_cast<int32_t*>(animState + 8) = backup_tickcount;
	}

	Hooked::PlayerHook::~PlayerHook() {
		clientHook.Destroy();
		renderableHook.Destroy();
		networkableHook.Destroy();
	}

	void Hooked::PlayerHook::SetHooks() {
		networkableHook.Hook(hkEntityRelease, 1);
		networkableHook.Hook(PreDataUpdate, 6);
		networkableHook.Hook(PostDataUpdate, 7);
		renderableHook.Hook(hkSetupBones, 13);
		//clientHook.Hook( DoExtraBonesProccesing, 197 );
	}

	bool __fastcall hkSetupBones(uintptr_t ecx, void* edx, matrix3x4_t* matrix, int bone_count, int bone_mask, float time) {
		if (!ecx)
			return false;

		auto player = reinterpret_cast<C_BasePlayer*>(ecx - 0x4);
		if (!player)
			return false;

		auto& hook = player_hooks[player->m_entIndex];

		using SetupBonesFn = bool(__thiscall*)(uintptr_t, matrix3x4_t*, int, int, float);
		auto oSetupBones = hook.renderableHook.VCall< SetupBonesFn >(13);

		C_CSPlayer* pLocal = C_CSPlayer::GetLocalPlayer();

		if (!pLocal)
			return false;

		if (player->EntIndex() != pLocal->EntIndex()) {
			auto ret = oSetupBones(ecx, matrix, bone_count, bone_mask, time);
			return ret;
		}

		if (matrix) {
			if (bone_count < player->m_CachedBoneData().Count())
				return false;

			std::memcpy(matrix, player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * player->m_CachedBoneData().Count());
		}

		return true;
	}

	void __fastcall hkEntityRelease(uintptr_t ecx, void* edx) {
		g_Vars.globals.szLastHookCalled = XorStr("24");
		auto entity = reinterpret_cast<C_BaseEntity*>(ecx - 0x8);

		auto& hook = player_hooks[entity->m_entIndex];

		using Fn = void(__thiscall*)(uintptr_t);
		auto orig = hook.networkableHook.VCall< Fn >(1);

		player_hooks.erase(entity->m_entIndex);

		orig(ecx);
	}

	static float simtime2 = 9999999237498237498723984729999999999.0f;
	static float simtime3 = 9999999237498237498723984729999999999.0f;
	void __fastcall PreDataUpdate(uintptr_t ecx, void* edx, int updateType) {
		g_Vars.globals.szLastHookCalled = XorStr("25");
		auto entity = reinterpret_cast<C_CSPlayer*>(ecx - 0x8);

		auto& hook = player_hooks[entity->m_entIndex];

		using Fn = void(__thiscall*)(uintptr_t, int);
		auto orig = hook.networkableHook.VCall< Fn >(6);

		auto local = C_CSPlayer::GetLocalPlayer();

		if (local == entity)
			return orig(ecx, updateType);

		auto lagData = Engine::LagCompensation::Get()->GetLagData(entity->m_entIndex);
		if (!lagData.IsValid())
			return orig(ecx, updateType);

		// determine whether we should update our networking or not
		lagData->m_bShouldNetUpdate = updateType <= 1; // DATA_UPDATE_DATATABLE_CHANGED

		// store pre update data
		auto& ppduDataPointer = lagData->m_sPPDUData;
		entity->CopyPoseParameters(ppduDataPointer.m_flPrePoseParams);
		entity->CopyAnimLayers(ppduDataPointer.m_PreAnimLayers);
		ppduDataPointer.m_flPreSimulationTime = entity->m_flSimulationTime();
		ppduDataPointer.m_vecPreNetOrigin = entity->GetNetworkOrigin();
		ppduDataPointer.m_angPreAbsAngles = entity->GetAbsAngles();
		ppduDataPointer.m_angPreEyeAngles = entity->m_angEyeAngles();
		ppduDataPointer.m_flPreLowerBodyYaw = entity->m_flLowerBodyYawTarget();
		//ppduDataPointer.m_flLowerBodyUpdateTime = entity->m_flAnimationTime();
		//std::clamp(ppduDataPointer.m_flLowerBodyUpdateTime, 0.0f, 10.0f);
		ppduDataPointer.m_flPreVelocityModifier = entity->m_flVelocityModifier();
		ppduDataPointer.m_flPreShotTime = 0.0f;
		if (auto weapon = (C_WeaponCSBaseGun*)entity->m_hActiveWeapon().Get())
			ppduDataPointer.m_flPreShotTime = weapon->m_fLastShotTime();

		// stop animstate->update from calling so when we store our layers later our data will be accurate with the server.
		if (auto animState = entity->m_PlayerAnimState())
			animState->m_last_update_frame = Interfaces::m_pGlobalVars->framecount;

		// initalize these variables as false before continuation.
		// commented this out as this is done every tick now.
		ppduDataPointer.m_bOriginDiffersFromRecord = false;
		ppduDataPointer.m_bLayersDiffersFromRecord = false;
		ppduDataPointer.m_bAbsAnglesDiffersFromRecord = false;
		ppduDataPointer.m_bEyeAnglesDiffersFromRecord = false;
		ppduDataPointer.m_bVelocityModifierDiffersFromRecord = false;
		ppduDataPointer.m_bShotTimeDiffersFromRecord = false;
		ppduDataPointer.m_bSimulationTimeDiffersFromRecord = false;
		ppduDataPointer.m_bLowerBodyYawDiffersFromRecord = false;

		if (local != entity)
			simtime3 = entity->m_flSimulationTime(); 

		orig(ecx, updateType);

		if (local != entity)
			simtime2 = entity->m_flSimulationTime();
	}

	void __fastcall PostDataUpdate(uintptr_t ecx, void* edx, int updateType) {
		g_Vars.globals.szLastHookCalled = XorStr("25");
		auto entity = reinterpret_cast<C_CSPlayer*>(ecx - 0x8);

		auto& hook = player_hooks[entity->m_entIndex];

		using Fn = void(__thiscall*)(uintptr_t, int);
		auto orig = hook.networkableHook.VCall< Fn >(7);

		auto local = C_CSPlayer::GetLocalPlayer();

		if (local == entity)
			return orig(ecx, updateType);

		auto lagData = Engine::LagCompensation::Get()->GetLagData(entity->m_entIndex);
		if (!lagData.IsValid() /*|| !lagData->m_History.front().m_bIsValid*/)
			return orig(ecx, updateType);

		auto& ppduDataPointer = lagData->m_sPPDUData;

		// variable initialization
		ppduDataPointer.m_bShouldUseServerData = false;

		bool dataModified = false;

		Vector networkOrigin = entity->GetNetworkOrigin();
		if (ppduDataPointer.m_vecPreNetOrigin != networkOrigin)
		{
			ppduDataPointer.m_vecPostNetOrigin = networkOrigin;

			if (!lagData->m_History.empty() && lagData->m_History.size() >= 2)
			{
				auto record = &lagData->m_History.front();
				
				// i would set this to animRecord but that is where we are setting these variables anyways.
				record->m_vecOrigin = networkOrigin;
			}

			ppduDataPointer.m_bOriginDiffersFromRecord = true;
			dataModified = true;
		}

		float velocityModifier = entity->m_flVelocityModifier();
		if (ppduDataPointer.m_flPreVelocityModifier != velocityModifier)
		{
			ppduDataPointer.m_flPostVelocityModifier = velocityModifier;
			ppduDataPointer.m_bVelocityModifierDiffersFromRecord = true;
			dataModified = true;
		}

		ppduDataPointer.m_flPostShotTime = ppduDataPointer.m_flPreShotTime;
		if (auto weapon = (C_WeaponCSBaseGun*)entity->m_hActiveWeapon().Get())
		{
			float shotTime = weapon->m_fLastShotTime();
			ppduDataPointer.m_flPostShotTime = shotTime;

			if (ppduDataPointer.m_flPreShotTime != 0.0f && ppduDataPointer.m_flPreShotTime != shotTime)
			{
				ppduDataPointer.m_bShotTimeDiffersFromRecord = true;
				dataModified = true;
			}
		}

		for (int i = 0; i < entity->m_AnimOverlay().Count(); i++)
		{
			C_AnimationLayer* currentLayer = &entity->m_AnimOverlay()[i];
			C_AnimationLayer* previousLayer = &ppduDataPointer.m_PreAnimLayers[i];
			if (currentLayer->m_nSequence != previousLayer->m_nSequence
			||  currentLayer->m_flWeightDeltaRate != previousLayer->m_flWeightDeltaRate
			||  currentLayer->m_flCycle != previousLayer->m_flCycle
			||  currentLayer->m_flWeight != previousLayer->m_flWeight
			||  currentLayer->m_flPlaybackRate != previousLayer->m_flPlaybackRate)
			{
				ppduDataPointer.m_bLayersDiffersFromRecord = true;
				dataModified = true;
				break;
			}
		}

		QAngle absAngles = entity->GetAbsAngles();
		if (ppduDataPointer.m_angPreAbsAngles != absAngles) 
		{
			ppduDataPointer.m_angPostAbsAngles = absAngles;

			// this check isn't the most needed but fuck it we might as well be as accurate as possible!
			ppduDataPointer.m_bAbsAnglesDiffersFromRecord = true;
		}

		QAngle eyeAngles = entity->m_angEyeAngles();
		if (ppduDataPointer.m_angPreEyeAngles != eyeAngles) 
		{
			ppduDataPointer.m_angPostEyeAngles = eyeAngles;
			ppduDataPointer.m_bEyeAnglesDiffersFromRecord = true;
			dataModified = true;
		}

		float flLBY = entity->m_flLowerBodyYawTarget();
		if (ppduDataPointer.m_flPreLowerBodyYaw != flLBY)
		{
			ppduDataPointer.m_flPostLowerBodyYaw = flLBY;
			//ILoggerEvent::Get()->PushEvent(tfm::format(XorStr("LBY UPDATED AT %f"), ppduDataPointer.m_flLowerBodyUpdateTime), FloatColor(0.5f, 0.5f, 0.5f), true);
			//ppduDataPointer.m_flRecordedLBYUpdateTime = ppduDataPointer.m_flLowerBodyUpdateTime;
			//ppduDataPointer.m_flLowerBodyUpdateTime = 0.0f; // null lby update time so we don't over stack it.
			ppduDataPointer.m_bLowerBodyYawDiffersFromRecord = true;
		}

		const float simTime = entity->m_flSimulationTime();
		if (ppduDataPointer.m_flPreSimulationTime != simTime)
		{
			ppduDataPointer.m_bSimulationTimeDiffersFromRecord = true;
			dataModified = true;
		}

		if (dataModified)
		{
			if (ppduDataPointer.m_bLayersDiffersFromRecord)
				entity->CopyAnimLayers(ppduDataPointer.m_PostAnimLayers);

			if (!ppduDataPointer.m_bSimulationTimeDiffersFromRecord || simTime < ppduDataPointer.m_flPreSimulationTime)
				lagData->m_bNetUpdateWasSilent = true;

			ppduDataPointer.m_bShouldUseServerData = true;
		} 

		orig(ecx, updateType);
	}

	IClientNetworkable* hkCreateCCSPlayer(int entnum, int serialNum) {
		g_Vars.globals.szLastHookCalled = XorStr("26");
		auto entity = (IClientNetworkable*)oCreateCCSPlayer(entnum, serialNum);

		auto& new_hook = player_hooks[entnum];
		new_hook.clientHook.Create((void*)((uintptr_t)entity - 0x8));
		new_hook.renderableHook.Create((void*)((uintptr_t)entity - 0x4));
		new_hook.networkableHook.Create(entity);
		new_hook.SetHooks();
		return entity;
	}
}
