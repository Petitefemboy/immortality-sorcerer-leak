#include "AnimationSystem.hpp"
#include "../../SDK/displacement.hpp"
#include "../../Utils/Math.h"
#include "../Game/SetupBones.hpp"
#include "../Game/SimulationContext.hpp"
#include "../../Utils/Threading/threading.h"
#include "LagCompensation.hpp"
#include "Resolver.hpp"
#include "Ragebot.hpp"
#include "../../SDK/CVariables.hpp"

bool m_bShouldDelayShot = false;

// im lazy, will eventually throw this in the math class
static float normalize_float(float angle)
{
	auto revolutions = angle / 360.f;
	if (angle > 180.f || angle < -180.f)
	{
		revolutions = round(abs(revolutions));
		if (angle < 0.f)
			angle = (angle + 360.f * revolutions);
		else
			angle = (angle - 360.f * revolutions);
		return angle;
	}
	return angle;
}

#define MT_SETUP_BONES
namespace Engine
{
	struct SimulationRestore {
		int m_fFlags;
		float m_flDuckAmount;
		float m_flFeetCycle;
		float m_flFeetYawRate;
		QAngle m_angEyeAngles;
		Vector m_vecOrigin;

		void Setup(C_CSPlayer* player) {
			m_fFlags = player->m_fFlags();
			m_flDuckAmount = player->m_flDuckAmount();
			m_vecOrigin = player->m_vecOrigin();
			m_angEyeAngles = player->m_angEyeAngles();

			auto animState = player->m_PlayerAnimState();
			m_flFeetCycle = animState->m_primary_cycle;
			m_flFeetYawRate = animState->m_move_weight;
		}

		void Apply(C_CSPlayer* player) const {
			player->m_fFlags() = m_fFlags;
			player->m_flDuckAmount() = m_flDuckAmount;
			player->m_vecOrigin() = m_vecOrigin;
			player->m_angEyeAngles() = m_angEyeAngles;

			auto animState = player->m_PlayerAnimState();
			animState->m_primary_cycle = m_flFeetCycle;
			animState->m_move_weight = m_flFeetYawRate;
		}
	};

	struct AnimationBackup {

		CCSGOPlayerAnimState anim_state;
		C_AnimationLayer layers[13];
		float pose_params[19];

		AnimationBackup() {

		}

		void Apply(C_CSPlayer* player) const;
		void Setup(C_CSPlayer* player);
	};

	void AnimationBackup::Apply(C_CSPlayer* player) const {
		*player->m_PlayerAnimState() = this->anim_state;
		std::memcpy(player->m_AnimOverlay().m_Memory.m_pMemory, layers, sizeof(layers));
		std::memcpy(player->m_flPoseParameter(), pose_params, sizeof(pose_params));
	}

	void AnimationBackup::Setup(C_CSPlayer* player) {
		this->anim_state = *player->m_PlayerAnimState();
		std::memcpy(layers, player->m_AnimOverlay().m_Memory.m_pMemory, sizeof(layers));
		std::memcpy(pose_params, player->m_flPoseParameter(), sizeof(pose_params));
	}

	//inline void FixBonesRotations(C_CSPlayer* player, matrix3x4_t* bones) {
	//	// copypasted from supremacy/fatality, no difference imo
	//	// also seen that in aimware multipoints, but was lazy to paste, kek
	//	auto studio_hdr = player->m_pStudioHdr();
	//	if (studio_hdr) {
	//		auto hdr = *(studiohdr_t**)studio_hdr;
	//		if (hdr) {
	//			auto hitboxSet = hdr->pHitboxSet(player->m_nHitboxSet());
	//			for (int i = 0; i < hitboxSet->numhitboxes; i++) {
	//				auto hitbox = hitboxSet->pHitbox(i);
	//				if (hitbox->m_angAngles.IsZero())
	//					continue;
	//
	//				matrix3x4_t hitboxTransform;
	//				hitboxTransform.AngleMatrix(hitbox->m_angAngles);
	//				bones[hitbox->bone] = bones[hitbox->bone].ConcatTransforms(hitboxTransform);
	//			}
	//		}
	//	}
	//}
	//
	// write access to const memory has been detected, the output may be wrong!
	//bool Extrapolate(float a1, float a2)
	//{
	//	auto local = C_CSPlayer::GetLocalPlayer();
	//
	//	// [COLLAPSED LOCAL DECLARATIONS. PRESS KEYPAD CTRL-"+" TO EXPAND]
	//	float v110 = a2;
	//	float v91 = a1;
	//	dword_3A8B3318 = 1;
	//	//v2 = (*(MEMORY[0x61A0C270] + 168))();
	//	//v72 = v2;
	//	//if...
	//	//	v10 = 0;
	//LABEL_18:
	//	dword_3A8B331C = v10;
	//	dword_3A8B3318 = 2;
	//	//v11 = (*(MEMORY[0x61A0C270] + 168))();
	//	//v73 = v11;
	//	//if...
	//	//	v19 = 0;
	//LABEL_36:
	//	dword_3A8B3320 = v19;
	//	int v24 = local->m_fFlags();
	//	v25 = v23;
	//	v126 = 0;
	//	v125 = 0;
	//	v124 = 0;
	//	v129 = 0.0;
	//	v128 = 0;
	//	v127 = 0;
	//	v113 = 0;
	//	v112 = 0;
	//	v111 = 0;
	//	v116 = 0.0;
	//	v115 = 0.0;
	//	v114 = 0.0;
	//	v119 = 0.0;
	//	v118 = 0;
	//	v117 = 0;
	//	v123 = 0;
	//	v90 = &dword_3A18FA7C;
	//	if (m_fFlags || (v26 = Utilities::GetNetvarOffset(2080526816), v24 = v26, (m_fFlags = v26) != 0))
	//		v26 = (v24[2] + (*v24 ^ v24[1])) ^ 0xA4A37590;
	//	v27 = m_vecOrigin;
	//	m_bOnGround = *(v25 + v26) & FL_ONGROUND;
	//	if (m_vecOrigin || (v28 = Utilities::GetNetvarOffset(-1081228600), v27 = v28, (m_vecOrigin = v28) != 0))
	//		v28 = ((v27[2] + (*v27 ^ v27[1])) ^ 0x559D81AF);
	//	v29 = m_vecVelocity;
	//	v84 = *(v25 + v28);
	//	v87 = *(v25 + v28 + 4);
	//	v85 = *(v25 + v28 + 8);
	//	if (m_vecVelocity || (v30 = Utilities::GetNetvarOffset(-1713477532), v29 = v30, (m_vecVelocity = v30) != 0))
	//		v30 = ((v29[2] + (*v29 ^ v29[1])) ^ 0x53630D14);
	//	v31 = *(v25 + v30);
	//	v83 = *(v25 + v30 + 8);
	//	m_flWishVelocityX = *(v25 + v30);
	//	v32 = *(v25 + v30 + 4);
	//	m_flWishVelocityY = *(v25 + v30 + 4);
	//	sub_398C2354(v29);
	//	v78 = 0.0;
	//	v33 = v32 * 57.29578;
	//	if (v91 <= 0.0)
	//		return 0;
	//	v34 = *&xmmword_3A193E18;
	//	v88 = v84;
	//	v89 = v87;
	//	v80 = v85;
	//	while (1)
	//	{
	//		v35 = v33 + v110;
	//		dword_3A1B21DC = 1;
	//		v86 = v35;
	//		if (v35 > v34)
	//		{
	//			do
	//				v35 = v35 - 360.0;
	//			while (v35 > v34);
	//			v86 = v35;
	//		}
	//		if (v35 < -180.0)
	//		{
	//			do
	//				v35 = v35 + 360.0;
	//			while (v35 < -180.0);
	//			v86 = v35;
	//		}
	//		v36 = LODWORD(m_flWishVelocityY);
	//		v36.m128_f32[0] = (v36.m128_f32[0] * v36.m128_f32[0]) + (m_flWishVelocityX * m_flWishVelocityX);
	//		v37 = _mm_cvtps_pd(v36);
	//		if (*v37.m128i_i64 < 0.0)
	//			v37 = sub_399A3869(v37);
	//		else
	//			v37.m128i_i64[0] = sqrt(*v37.m128i_i64);
	//		v38 = *v37.m128i_i64;
	//		v39 = v38;
	//		v40 = LODWORD(v86);
	//		v40.m128_f32[0] = v86 * 0.017453292;
	//		v41 = _mm_cvtps_pd(v40);
	//		v79 = v41.m128d_f64[0];
	//		v42 = sub_3976F112(v41, 0i64, v31);
	//		*v42.m128d_f64 = v42.m128d_f64[0];
	//		m_flWishVelocityX = *v42.m128d_f64 * v39;
	//		v43 = sub_3977CB6F(*&v79, 0i64, v31);
	//		*v43.m128i_i32 = *v43.m128i_i64;
	//		m_flWishVelocityY = *v43.m128i_i32 * v39;
	//		if (m_bOnGround)
	//		{
	//			v44 = Utilities::GetVFunc(0, 12);
	//			v44();
	//			v129 = v32;
	//			v45 = v129;
	//			v83 = v129;
	//			if (g_IsLocalServerMaybe[0])
	//			{
	//				m_flWishVelocityX = m_flWishVelocityX * 1.2;
	//				m_flWishVelocityY = m_flWishVelocityY * 1.2;
	//			}
	//		}
	//		else
	//		{
	//			v46 = Utilities::GetVFunc(0, 12);
	//			v46();
	//			v47 = v32;
	//			v48 = *((g_pGlobalVars_0 ^ g_pGlobalVars) + 0x20) * v47;
	//			v45 = v83 - v48;
	//			v83 = v83 - v48;
	//		}
	//		v49 = *((g_pGlobalVars_0 ^ g_pGlobalVars) + 0x20);
	//		v94 = 0.0;
	//		v93 = 0.0;
	//		v92 = 0.0;
	//		v98 = 0.0;
	//		v97 = 0.0;
	//		v96 = 0.0;
	//		v102 = 0.0;
	//		v101 = 0.0;
	//		v100 = 0.0;
	//		v106 = 0.0;
	//		v105 = 0.0;
	//		v104 = 0.0;
	//		v108 = 0;
	//		LOWORD(v109) = 0;
	//		dword_3A8A3828 = Utilities::GetNetvarOffset(442777232);
	//		v50 = Utilities::GetNetvarOffset(-1739035488);
	//		dword_3A8A382C = v50;
	//		v99 = LODWORD(v79);
	//		v108 = 0;
	//		v96 = ((v49 * m_flWishVelocityX) + v88) - v84;
	//		BYTE1(v109) = 1;
	//		v97 = ((v49 * m_flWishVelocityY) + v89) - v87;
	//		v98 = ((v49 * v45) + v80) - v85;
	//		if ((((v97 * v97) + (v96 * v96)) + (v98 * v98)) == 0.0)
	//			BYTE1(v109) = 0;
	//		v51 = *v25 - *(v25 + v50);
	//		v52 = v25[1] - *(v25 + v50 + 4);
	//		v53 = v25[2] - *(v25 + v50 + 8);
	//		v107 = *&v79;
	//		LOBYTE(v109) = 0;
	//		v104 = v51 * 0.5;
	//		v105 = v52 * 0.5;
	//		v106 = v53 * 0.5;
	//		v54 = (v25[2] + *(v25 + v50 + 8)) * 0.5;
	//		v55 = (*v25 + *(v25 + v50)) * 0.5;
	//		v56 = (v25[1] + *(v25 + v50 + 4)) * 0.5;
	//		v103 = *&v79;
	//		v94 = v85 + v54;
	//		v95 = *&v79;
	//		v92 = v84 + v55;
	//		v93 = v87 + v56;
	//		v100 = v55 * -1.0;
	//		v101 = v56 * -1.0;
	//		v102 = v54 * -1.0;
	//		(*(*(DWORD1(xmmword_3A4A114C) ^ xmmword_3A4A114C) + 20))(&v92, 33636363, &v90, &v111);
	//		if (v120 != 1.0 && v119 <= 0.89999998)
	//			break;
	//		if (v122 || v121)
	//			break;
	//		v57 = v116;
	//		v58 = v114;
	//		v59 = v115;
	//		v88 = v114;
	//		v89 = v115;
	//		v74 = v116;
	//		v80 = v116;
	//		v84 = v114;
	//		v87 = v115;
	//		v85 = v116;
	//		dword_3A8A3828 = Utilities::GetNetvarOffset(442777232);
	//		v60 = Utilities::GetNetvarOffset(-1739035488);
	//		dword_3A8A382C = v60;
	//		v108 = 0;
	//		BYTE1(v109) = 1;
	//		v99 = LODWORD(v79);
	//		v96 = (v58 - 0.0) - v58;
	//		v97 = (v59 - 0.0) - v59;
	//		v98 = (v57 - 2.0) - v57;
	//		if ((((v97 * v97) + (v96 * v96)) + (v98 * v98)) == 0.0)
	//			BYTE1(v109) = 0;
	//		v61 = *v25;
	//		v62 = v25[1] - *(v25 + v60 + 4);
	//		v63 = v25[2] - *(v25 + v60 + 8);
	//		v107 = *(v25 + v60);
	//		LOBYTE(v109) = 0;
	//		v104 = (v61 - v107) * 0.5;
	//		v105 = v62 * 0.5;
	//		v106 = v63 * 0.5;
	//		v64 = *(v25 + v60 + 8);
	//		v65 = *(v25 + v60);
	//		v31 = *(v25 + v60 + 4);
	//		v66 = v25[2];
	//		v67 = v25[1];
	//		v103 = *v25;
	//		v68 = (v64 + v66) * 0.5;
	//		v69 = (v65 + v103) * 0.5;
	//		*&v31 = (*&v31 + v67) * 0.5;
	//		v70 = v59 + *&v31;
	//		v92 = v58 + v69;
	//		v94 = v74 + v68;
	//		v95 = v103;
	//		*&v31 = *&v31 * -1.0;
	//		v93 = v70;
	//		v100 = v69 * -1.0;
	//		v101 = *&v31;
	//		v102 = v68 * -1.0;
	//		(*(*(DWORD1(xmmword_3A4A114C) ^ xmmword_3A4A114C) + 20))(&v92, 33636363, &v90, &v111);
	//		if (v120 >= 1.0 && !v121 && !v122 || (m_bOnGround = 1, v119 <= 0.69999999))
	//			m_bOnGround = 0;
	//		v34 = *&xmmword_3A193E18;
	//		v33 = v86;
	//		v78 = *((g_pGlobalVars_0 ^ g_pGlobalVars) + 0x20) + v78;
	//		if (v91 <= v78)
	//			return 0;
	//	}
	//
	//	return 1;
	//}

	class C_AnimationSystem : public AnimationSystem {
	public:
		virtual void CollectData();
		virtual void Update();

		virtual C_AnimationData* GetAnimationData(int index) {
			if (m_AnimatedEntities.count(index) < 1)
				return nullptr;

			return &m_AnimatedEntities[index];
		}

		std::map<int, C_AnimationData> m_AnimatedEntities = { };

		C_AnimationSystem() { };
		virtual ~C_AnimationSystem() { };
	};

	Encrypted_t<AnimationSystem> AnimationSystem::Get() {
		static C_AnimationSystem instance;
		return &instance;
	}

	void C_AnimationSystem::CollectData() {
		if (!Interfaces::m_pEngine->IsInGame() || !Interfaces::m_pEngine->GetNetChannelInfo()) {
			this->m_AnimatedEntities.clear();
			return;
		}

		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || !g_Vars.globals.HackIsReady)
			return;

		for (int i = 1; i <= Interfaces::m_pGlobalVars->maxClients; ++i) {
			auto player = C_CSPlayer::GetPlayerByIndex(i);
			if (!player || player == local)
				continue;

			player_info_t player_info;
			if (!Interfaces::m_pEngine->GetPlayerInfo(player->m_entIndex, &player_info)) {
				continue;
			}

			this->m_AnimatedEntities[i].Collect(player);
		}
	}

	void C_AnimationSystem::Update() {
		if (!Interfaces::m_pEngine->IsInGame() || !Interfaces::m_pEngine->GetNetChannelInfo()) {
			this->m_AnimatedEntities.clear();
			return;
		}

		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || !g_Vars.globals.HackIsReady)
			return;

		for (auto& [key, value] : this->m_AnimatedEntities) {
			auto entity = C_CSPlayer::GetPlayerByIndex(key);
			if (!entity)
				continue;

			auto curtime = Interfaces::m_pGlobalVars->curtime;
			auto frametime = Interfaces::m_pGlobalVars->frametime;

			Interfaces::m_pGlobalVars->curtime = entity->m_flOldSimulationTime() + Interfaces::m_pGlobalVars->interval_per_tick;
			Interfaces::m_pGlobalVars->frametime = Interfaces::m_pGlobalVars->interval_per_tick;

			if (value.m_bUpdated)
				value.Update();

			Interfaces::m_pGlobalVars->curtime = curtime;
			Interfaces::m_pGlobalVars->frametime = frametime;

			value.m_bUpdated = false;
		}
	}

	void C_AnimationData::Update() {
		if (!this->player || this->m_AnimationRecord.size() < 1)
			return;

		C_CSPlayer* pLocal = C_CSPlayer::GetLocalPlayer();
		if (!pLocal)
			return;

		auto pAnimationRecord = Encrypted_t<Engine::C_AnimationRecord>(&this->m_AnimationRecord.front());
		Encrypted_t<Engine::C_AnimationRecord> pPreviousAnimationRecord(nullptr);
		if (this->m_AnimationRecord.size() > 1) {
			pPreviousAnimationRecord = &this->m_AnimationRecord.at(1);
		}

		this->player->m_vecVelocity() = pAnimationRecord->m_vecAnimationVelocity;

		auto weapon = (C_BaseAttributableItem*)player->m_hActiveWeapon().Get();
		auto weaponWorldModel = weapon ? (C_CSPlayer*)(weapon)->m_hWeaponWorldModel().Get() : nullptr;

		auto animState = player->m_PlayerAnimState();
		if (!animState)
			return;

		pAnimationRecord->m_serverPlayerAnimState = animState;

		// simulate animations
		SimulateAnimations(pAnimationRecord, pPreviousAnimationRecord);

		// update layers
		std::memcpy(player->m_AnimOverlay().Base(), pAnimationRecord->m_serverAnimOverlays, 13 * sizeof(C_AnimationLayer));

		// generate aimbot matrix
		g_BoneSetup.SetupBonesRebuild(player, m_Bones, 128, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_BONE_MERGE, player->m_flSimulationTime(), BoneSetupFlags::UseCustomOutput);

		// generate visual matrix
		g_BoneSetup.SetupBonesRebuild(player, nullptr, 128, 0x7FF00, player->m_flSimulationTime(), BoneSetupFlags::ForceInvalidateBoneCache | BoneSetupFlags::AttachmentHelper);

		this->m_vecSimulationData.clear();
	}

	void C_AnimationData::Collect(C_CSPlayer* player) {
		if (player->IsDead())
			player = nullptr;

		auto pThis = Encrypted_t<C_AnimationData>(this);

		if (pThis->player != player) {
			pThis->m_flSpawnTime = 0.0f;
			pThis->m_flSimulationTime = 0.0f;
			pThis->m_flOldSimulationTime = 0.0f;
			pThis->m_iCurrentTickCount = 0;
			pThis->m_iOldTickCount = 0;
			pThis->m_iTicksAfterDormancy = 0;
			pThis->m_vecSimulationData.clear();
			pThis->m_AnimationRecord.clear();
			pThis->m_bIsDormant = pThis->m_bBonesCalculated = false;
			pThis->player = player;
			pThis->m_bIsAlive = false;
		}

		if (!player)
			return;

		pThis->m_bIsAlive = true;
		pThis->m_flOldSimulationTime = pThis->m_flSimulationTime;
		pThis->m_flSimulationTime = pThis->player->m_flSimulationTime();

		if (pThis->m_flSimulationTime == 0.0f || pThis->player->IsDormant()) {
			pThis->m_bIsDormant = true;
			Engine::g_ResolverData[player->EntIndex()].m_bWentDormant = true;
			Engine::g_ResolverData[player->EntIndex()].m_vecSavedOrigin = pThis->m_vecOrigin;
			return;
		}

		if (pThis->m_flOldSimulationTime == pThis->m_flSimulationTime) {
			return;
		}

		if (pThis->m_bIsDormant) {
			pThis->m_iTicksAfterDormancy = 0;
			pThis->m_AnimationRecord.clear();

			Engine::g_ResolverData[player->EntIndex()].m_bWentDormant = true;
		}

		pThis->ent_index = player->m_entIndex;

		pThis->m_bUpdated = true;
		pThis->m_bIsDormant = false;

		pThis->m_iOldTickCount = pThis->m_iCurrentTickCount;
		pThis->m_iCurrentTickCount = Interfaces::m_pGlobalVars->tickcount;

		if (pThis->m_flSpawnTime != pThis->player->m_flSpawnTime()) {
			auto animState = pThis->player->m_PlayerAnimState();
			if (animState) {
				animState->m_player = pThis->player;
				animState->Reset();
			}

			pThis->m_flSpawnTime = pThis->player->m_flSpawnTime();
		}

		int nTickRate = int(1.0f / Interfaces::m_pGlobalVars->interval_per_tick);
		while (pThis->m_AnimationRecord.size() > nTickRate) {
			pThis->m_AnimationRecord.pop_back();
		}

		pThis->m_iTicksAfterDormancy++;

		Encrypted_t<C_AnimationRecord> previous_record = nullptr;
		Encrypted_t<C_AnimationRecord> penultimate_record = nullptr;

		if (pThis->m_AnimationRecord.size() > 0) {
			previous_record = &pThis->m_AnimationRecord.front();
			if (pThis->m_AnimationRecord.size() > 1) {
				penultimate_record = &pThis->m_AnimationRecord.at(1);
			}
		}

		bool bOverrideAnimations = true;

		auto record = &pThis->m_AnimationRecord.emplace_front();

		pThis->m_vecOrigin = pThis->player->m_vecOrigin();

		record->m_bIsDormant = pThis->player->IsDormant();
		record->m_vecOrigin = pThis->player->m_vecOrigin();
		record->m_angEyeAngles = pThis->player->m_angEyeAngles();
		record->m_flSimulationTime = pThis->m_flSimulationTime;
		record->m_flLowerBodyYawTarget = pThis->player->m_flLowerBodyYawTarget();

		auto weapon = (C_WeaponCSBaseGun*)(player->m_hActiveWeapon().Get());
		if (weapon) {
			auto weaponWorldModel = (C_CSPlayer*)((C_BaseAttributableItem*)weapon)->m_hWeaponWorldModel().Get();

			for (int i = 0; i < player->m_AnimOverlay().Count(); ++i) {
				player->m_AnimOverlay().Element(i).m_pOwner = player;
				player->m_AnimOverlay().Element(i).m_pStudioHdr = player->m_pStudioHdr();

				if (weaponWorldModel) {
					if (player->m_AnimOverlay().Element(i).m_nSequence < 2 || player->m_AnimOverlay().Element(i).m_flWeight <= 0.0f)
						continue;

					using UpdateDispatchLayer = void(__thiscall*)(void*, C_AnimationLayer*, CStudioHdr*, int);
					Memory::VCall< UpdateDispatchLayer >(player, 241)(player, &player->m_AnimOverlay().Element(i),
						weaponWorldModel->m_pStudioHdr(), player->m_AnimOverlay().Element(i).m_nSequence);
				}
			}
		}

		std::memcpy(record->m_serverAnimOverlays, pThis->player->m_AnimOverlay().Base(), sizeof(record->m_serverAnimOverlays));

		record->m_flFeetCycle = record->m_serverAnimOverlays[6].m_flCycle;
		record->m_flFeetYawRate = record->m_serverAnimOverlays[6].m_flWeight;

		record->m_fFlags = player->m_fFlags();
		record->m_flDuckAmount = player->m_flDuckAmount();

		record->m_bIsShoting = false;
		record->m_flShotTime = 0.0f;
		record->m_bFakeWalking = false;

		if (previous_record.IsValid()) {
			record->m_flChokeTime = pThis->m_flSimulationTime - pThis->m_flOldSimulationTime;
			record->m_iChokeTicks = TIME_TO_TICKS(record->m_flChokeTime);
			record->m_flOldLowerBodyYaw = previous_record->m_flLowerBodyYawTarget;

			// we could possibly put this at the top of this but it's probably better if we set all of our data before storing this variable.
			if (previous_record->m_bIsDormant && !record->m_bIsDormant)
				m_bShouldDelayShot = true;
			else
				m_bShouldDelayShot = false;
		}
		else {
			record->m_flChokeTime = Interfaces::m_pGlobalVars->interval_per_tick;
			record->m_iChokeTicks = 1;
		}

		if (!previous_record.IsValid()) {
			record->m_bIsInvalid = true;
			record->m_vecVelocity.Init();
			record->m_bIsShoting = false;
			record->m_bTeleporting = false;
			record->m_flOldLowerBodyYaw = record->m_flLowerBodyYawTarget; // set this to our old lower body yaw so it doesn't == null.

			auto animstate = player->m_PlayerAnimState();
			if (animstate)
				animstate->m_abs_yaw = record->m_flLowerBodyYawTarget;//record->m_angEyeAngles.yaw;

			return;
		}

		auto flPreviousSimulationTime = previous_record->m_flSimulationTime;
		auto nTickcountDelta = pThis->m_iCurrentTickCount - pThis->m_iOldTickCount;
		auto nSimTicksDelta = record->m_iChokeTicks;
		auto nChokedTicksUnk = nSimTicksDelta;
		auto bShiftedTickbase = false;
		if (pThis->m_flOldSimulationTime > pThis->m_flSimulationTime) {
			record->m_bShiftingTickbase = true;
			record->m_iChokeTicks = nTickcountDelta;
			record->m_flChokeTime = TICKS_TO_TIME(record->m_iChokeTicks);
			flPreviousSimulationTime = record->m_flSimulationTime - record->m_flChokeTime;
			nChokedTicksUnk = nTickcountDelta;
			bShiftedTickbase = true;
		}

		if (bShiftedTickbase || abs(nSimTicksDelta - nTickcountDelta) <= 2) {
			if (nChokedTicksUnk) {
				if (nChokedTicksUnk != 1) {
					pThis->m_iTicksUnknown = 0;
				}
				else {
					pThis->m_iTicksUnknown++;
				}
			}
			else {
				record->m_iChokeTicks = 1;
				record->m_flChokeTime = Interfaces::m_pGlobalVars->interval_per_tick;

				flPreviousSimulationTime = record->m_flSimulationTime - Interfaces::m_pGlobalVars->interval_per_tick;

				pThis->m_iTicksUnknown++;
			}
		}

		if (weapon) {
			record->m_flShotTime = weapon->m_fLastShotTime();
			record->m_bIsShoting = record->m_flSimulationTime >= record->m_flShotTime && record->m_flShotTime > previous_record->m_flSimulationTime;
		}

		record->m_bIsInvalid = false;

		
		// fix velocity.
		// https://github.com/VSES/SourceEngine2007/blob/master/se2007/game/client/c_baseplayer.cpp#L659
		Vector vecVelocity;
		if (record->m_iChokeTicks > 0 && previous_record.IsValid() && !previous_record->m_bIsDormant) {
			record->m_vecVelocity = (record->m_vecOrigin - previous_record->m_vecOrigin * (1.0f / TICKS_TO_TIME(record->m_iChokeTicks)));
		
			if (!(record->m_fFlags & FL_ONGROUND)) {
				float_t flWeight = 1.0f - record->m_serverAnimOverlays[11].m_flWeight;
				if (flWeight > 0.0f)
				{
					float_t flPreviousRate = previous_record->m_serverAnimOverlays[11].m_flPlaybackRate;
					float_t flCurrentRate = record->m_serverAnimOverlays[11].m_flPlaybackRate;
		
					if (flPreviousRate == flCurrentRate)
					{
						int32_t iPreviousSequence = previous_record->m_serverAnimOverlays[11].m_nSequence;
						int32_t iCurrentSequence = record->m_serverAnimOverlays[11].m_nSequence;
		
						if (iPreviousSequence == iCurrentSequence)
						{
							float_t flSpeedNormalized = (flWeight / 2.8571432f) + 0.55f;
							if (flSpeedNormalized > 0.0f)
							{
								float_t flSpeed = flSpeedNormalized * player->GetMaxSpeed();
								if (flSpeed > 0.0f)
									if (vecVelocity.Length2D() > 0.0f)
										vecVelocity = (vecVelocity / vecVelocity.Length()) * flSpeed;
							}
						}
					}
		
					vecVelocity.z -= g_Vars.sv_gravity->GetFloat() * 0.5f * TICKS_TO_TIME(record->m_iChokeTicks);
				}
				else
					vecVelocity.z = 0.0f;
			}
		
			// used in our fakewalk resolver.
			if (previous_record->m_vecVelocity.Length() == 0.0f && record->m_vecVelocity.Length() != 0.0f) {
				Math::VectorAngles(record->m_vecVelocity, record->m_angDirWhenFirstStartedMoving);
			}
		}
		else {
			// set this as it will get overwritten
			vecVelocity/*record->m_vecVelocity*/ = player->m_vecVelocity();

			if (record->m_serverAnimOverlays[6].m_flPlaybackRate > 0.0f && record->m_serverAnimOverlays[6].m_flWeight != 0.f && record->m_vecVelocity.Length() > 0.1f) {
				auto v30 = player->GetMaxSpeed();

				if (record->m_fFlags & 6)
					v30 *= 0.34f;
				else if (player->m_bIsWalking())
					v30 *= 0.52f;

				auto v35 = record->m_serverAnimOverlays[6].m_flWeight * v30;
				vecVelocity/*record->m_vecVelocity*/ *= v35 / record->m_vecVelocity.Length();
			}
			else
				vecVelocity/*record->m_vecVelocity*/ = Vector{};

			if (record->m_fFlags & FL_ONGROUND)
				vecVelocity.z/*record->m_vecVelocity.z*/ = 0.f;
		}

		if (record->m_fFlags & FL_ONGROUND
		&&  record->m_serverAnimOverlays[4].m_flWeight == 0.0f
		&&  record->m_serverAnimOverlays[5].m_flWeight == 0.0f
		&&  record->m_serverAnimOverlays[6].m_flPlaybackRate == 0.0f
		&&  record->m_vecVelocity.Length() > 0.0f)
			record->m_bFakeWalking = true;

		// null animation velocity while entities are fakewalking to ensure our accuracy
		if (record->m_bFakeWalking) record->m_vecAnimationVelocity = record->m_vecAnimationVelocity = { 0.f, 0.f, 0.f };

		record->m_bTeleporting = record->m_vecOrigin.DistanceSquared(previous_record->m_vecOrigin) > 4096.0f;

		// previous on shot fix
		if (previous_record.IsValid() && pThis->m_AnimationRecord.size() > 1) {
			// detect players abusing micromovements or other trickery
			if ((record->m_vecVelocity.Length2D() < 20.f
				&& previous_record->m_vecVelocity.Length2D() > 0.f
				&& record->m_serverAnimOverlays[6].m_flWeight != 1.0f
				&& record->m_serverAnimOverlays[6].m_flWeight != 0.0f
				&& (record->m_serverAnimOverlays[6].m_flCycle != previous_record->m_serverAnimOverlays[6].m_flCycle || record->m_serverAnimOverlays[6].m_flCycle != record->m_serverAnimOverlays[6].m_flPrevCycle)
				&& (record->m_fFlags & FL_ONGROUND)))
				record->m_bUnsafeLBY = true;

			if (record->m_bIsShoting) {
				// we have a vaild on shot record
				if (record->m_flShotTime <= record->m_flSimulationTime) {
					// possibly switch this around
					record->m_angEyeAngles = player->m_angEyeAngles();
				}
				else
					record->m_angEyeAngles = previous_record->m_angEyeAngles;
			}
		}

		C_SimulationInfo& data = pThis->m_vecSimulationData.emplace_back();
		data.m_flTime = previous_record->m_flSimulationTime + Interfaces::m_pGlobalVars->interval_per_tick;
		data.m_flDuckAmount = record->m_flDuckAmount;
		data.m_flLowerBodyYawTarget = record->m_flLowerBodyYawTarget;
		data.m_vecOrigin = record->m_vecOrigin;
		data.m_vecVelocity = record->m_vecVelocity;
		data.bOnGround = record->m_fFlags & FL_ONGROUND;

		// thanks llama.
		if (data.bOnGround) {
			player->m_PlayerAnimState()->m_on_ground = true;
			player->m_PlayerAnimState()->m_landing = false;
		}

		// lets check if its been more than 2 ticks, so we can fix jumpfall.
		if (record->m_iChokeTicks > 2 && previous_record.IsValid() && !previous_record->m_bIsDormant) {
			bool bOnGround = record->m_fFlags & FL_ONGROUND;
			bool bJumped = false;
			bool bLandedOnServer = false;
			float flLandTime = 0.f;

			// do onetap bullshit
			if (record->m_serverAnimOverlays[4].m_flCycle < 0.5f && (!(record->m_fFlags & FL_ONGROUND) || !(previous_record->m_fFlags & FL_ONGROUND))) {
				flLandTime = record->m_flSimulationTime - float(record->m_serverAnimOverlays[4].m_flPlaybackRate / record->m_serverAnimOverlays[4].m_flCycle);
				bLandedOnServer = flLandTime >= previous_record->m_flSimulationTime;
			}

			// jump_fall fix
			if (bLandedOnServer && !bJumped) {
				if (flLandTime <= player->m_flAnimationTime()) {
					bJumped = true;
					bOnGround = true;
				}
				else {
					bOnGround = previous_record->m_fFlags & FL_ONGROUND;
				}
			}

			// do the fix. hahaha
			if (bOnGround)
				player->m_fFlags() |= FL_ONGROUND;
			else
				player->m_fFlags() &= ~FL_ONGROUND;

			// fix crouching players.
			// the duck amount we receive when people choke is of the last simulation.
			// if a player chokes packets the issue here is that we will always receive the last duckamount.
			// but we need the one that was animated.
			// therefore we need to compute what the duckamount was at animtime.

			// delta in duckamt and delta in time..
			float duck = record->m_flDuckAmount - previous_record->m_flDuckAmount;
			float time = record->m_flSimulationTime - previous_record->m_flSimulationTime;

			// get the duckamt change per tick.
			float change = (duck / time) * Interfaces::m_pGlobalVars->interval_per_tick;

			// fix crouching players.
			player->m_flDuckAmount() = previous_record->m_flDuckAmount + change;

			if (!record->m_bFakeWalking) {
				// fix the velocity till the moment of animation.
				Vector velo = record->m_vecVelocity - previous_record->m_vecVelocity;
			
				// accel per tick.
				Vector accel = (velo / time) * Interfaces::m_pGlobalVars->interval_per_tick;
			
				// set the anim velocity to the previous velocity.
				// and predict one tick ahead.
				record->m_vecAnimationVelocity = previous_record->m_vecVelocity + accel;
			}
		}
	}

	void C_AnimationData::SimulateAnimations(Encrypted_t<Engine::C_AnimationRecord> current, Encrypted_t<Engine::C_AnimationRecord> previous) {
		// is player bot?
		auto IsPlayerBot = [&]() -> bool {
			player_info_t info;
			if (Interfaces::m_pEngine->GetPlayerInfo(player->EntIndex(), &info))
				return info.fakeplayer;

			return false;
		};
		
		auto UpdateAnimations = [&](C_CSPlayer* player, float flTime) {
			auto curtime = Interfaces::m_pGlobalVars->curtime;
			auto frametime = Interfaces::m_pGlobalVars->frametime;
			auto realtime = Interfaces::m_pGlobalVars->realtime;
			auto absframetime = Interfaces::m_pGlobalVars->absoluteframetime;
			auto framecount = Interfaces::m_pGlobalVars->framecount;
			auto tickcount = Interfaces::m_pGlobalVars->tickcount;
			auto interpolation = Interfaces::m_pGlobalVars->interpolation_amount;

			// attempt to resolve the player	
			if (!player->IsTeammate(C_CSPlayer::GetLocalPlayer()) && !IsPlayerBot()) {
				g_Resolver.ResolveYaw(player, current.Xor());

				// predict lby updates
				g_Resolver.PredictBodyUpdates(player, current.Xor(), previous.Xor());

				bool bValid = previous.Xor();

				// we're sure that we resolved the player.
				if (bValid)
				{
					current.Xor()->m_bResolved = current.Xor()->m_iResolverMode == EResolverModes::RESOLVE_PRED ||
						current.Xor()->m_iResolverMode == EResolverModes::RESOLVE_WALK ||
						current.Xor()->m_flLowerBodyYawTarget != previous.Xor()->m_flLowerBodyYawTarget;
				}
				else
				{
					current.Xor()->m_bResolved = current.Xor()->m_iResolverMode == EResolverModes::RESOLVE_PRED || current.Xor()->m_iResolverMode == EResolverModes::RESOLVE_WALK;
				}

				bool bResolved = current.Xor()->m_bResolved;
				if (g_Vars.rage.override_resolver_flicks) {
					if (current.Xor()->m_iResolverMode == EResolverModes::RESOLVE_PRED)
						bResolved = false;
				}

				// if the enemy is resolved, why bother overriding?
				//g_Resolver.ResolveManual(player, current.Xor(), bResolved);
			}

			// force to use correct abs origin and velocity ( no CalcAbsolutePosition and CalcAbsoluteVelocity calls )
			player->m_iEFlags() &= ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY);

			// calculate animations based on ticks aka server frames instead of render frames
			Interfaces::m_pGlobalVars->curtime = flTime;
			Interfaces::m_pGlobalVars->realtime = flTime;
			Interfaces::m_pGlobalVars->framecount = TIME_TO_TICKS(player->m_flAnimationTime());
			Interfaces::m_pGlobalVars->tickcount = TIME_TO_TICKS(player->m_flAnimationTime());
			Interfaces::m_pGlobalVars->frametime = Interfaces::m_pGlobalVars->interval_per_tick;
			Interfaces::m_pGlobalVars->absoluteframetime = Interfaces::m_pGlobalVars->interval_per_tick;
			Interfaces::m_pGlobalVars->interpolation_amount = 0.0f;

			static auto& EnableInvalidateBoneCache = **reinterpret_cast<bool**>(Memory::Scan(XorStr("client.dll"), XorStr("C6 05 ? ? ? ? ? 89 47 70")) + 2);

			// make sure we keep track of the original invalidation state
			const auto oldInvalidationState = EnableInvalidateBoneCache;

			// fix animating in same frame.
			auto animstate = player->m_PlayerAnimState();
			if (animstate && animstate->m_last_update_frame >= Interfaces::m_pGlobalVars->framecount)
				animstate->m_last_update_frame = Interfaces::m_pGlobalVars->framecount - 1;

			for (int i = 0; i < player->m_AnimOverlay().Count(); ++i) {
				player->m_AnimOverlay().Base()[i].m_pOwner = player;
				player->m_AnimOverlay().Base()[i].m_pStudioHdr = player->m_pStudioHdr();
			}

			player->UpdateClientSideAnimationEx();

			// we don't want to enable cache invalidation by accident
			EnableInvalidateBoneCache = oldInvalidationState;

			player->InvalidatePhysicsRecursive(InvalidatePhysicsBits_t::ANIMATION_CHANGED);

			if (!player->IsTeammate(C_CSPlayer::GetLocalPlayer()) && !IsPlayerBot()) {
				// only do this bs when in air.
				if (player->m_flPoseParameter() && current.Xor()->m_iResolverMode == EResolverModes::RESOLVE_AIR) {
					// ang = pose min + pose val x ( pose range )

					// lean_yaw
					player->m_flPoseParameter()[2] = RandomInt(0, 4) * 0.25f;

					// body_yaw
					player->m_flPoseParameter()[11] = RandomInt(1, 3) * 0.25f;
				}
			}

			Interfaces::m_pGlobalVars->curtime = curtime;
			Interfaces::m_pGlobalVars->frametime = frametime;
			Interfaces::m_pGlobalVars->realtime = realtime;
			Interfaces::m_pGlobalVars->absoluteframetime = absframetime;
			Interfaces::m_pGlobalVars->framecount = framecount;
			Interfaces::m_pGlobalVars->tickcount = tickcount;
			Interfaces::m_pGlobalVars->interpolation_amount = interpolation; // possibly don't restore the interp amount...
		};

		SimulationRestore SimulationRecordBackup;
		SimulationRecordBackup.Setup(player);

		auto animState = player->m_PlayerAnimState();

		if (previous.IsValid()) {
			if (previous->m_bIsInvalid && current->m_fFlags & FL_ONGROUND) {
				animState->m_on_ground = true;
				animState->m_landing = false;
			}

			if (previous.IsValid()) {
				if (previous->m_bIsInvalid && current->m_fFlags & FL_ONGROUND) {
					animState->m_on_ground = true;
					animState->m_landing = false;
				}

				animState->m_primary_cycle = previous->m_flFeetCycle;
				animState->m_move_weight = previous->m_flFeetYawRate;
				*(float*)(uintptr_t(animState) + 0x180) = previous->m_serverAnimOverlays[12].m_flWeight;

				std::memcpy(player->m_AnimOverlay().Base(), previous->m_serverAnimOverlays, sizeof(previous->m_serverAnimOverlays));
			}
			else {
				animState->m_primary_cycle = current->m_flFeetCycle;
				animState->m_move_weight = current->m_flFeetYawRate;
				*(float*)(uintptr_t(animState) + 0x180) = current->m_serverAnimOverlays[12].m_flWeight;
			}
		}

		if (current->m_iChokeTicks > 1) {
			for (auto it = this->m_vecSimulationData.begin(); it < this->m_vecSimulationData.end(); it++) {
				m_bForceVelocity = true;
				const auto& simData = *it;
				if (simData.bOnGround) {
					player->m_fFlags() |= FL_ONGROUND;
				}
				else {
					player->m_fFlags() &= ~FL_ONGROUND;
				}

				player->m_vecOrigin() = simData.m_vecOrigin;
				player->m_flDuckAmount() = simData.m_flDuckAmount;
				player->m_vecVelocity() = simData.m_vecVelocity;
				player->SetAbsVelocity(simData.m_vecVelocity);
				player->SetAbsOrigin(simData.m_vecOrigin);
				player->m_flLowerBodyYawTarget() = simData.m_flLowerBodyYawTarget;

				UpdateAnimations(player, player->m_flOldSimulationTime() + Interfaces::m_pGlobalVars->interval_per_tick);

				m_bForceVelocity = false;
			}
		}
		else {
			m_bForceVelocity = true;
			this->player->SetAbsVelocity(current->m_vecAnimationVelocity);
			this->player->SetAbsOrigin(current->m_vecOrigin);
			this->player->m_flLowerBodyYawTarget() = current->m_flLowerBodyYawTarget;

			UpdateAnimations(player, player->m_flOldSimulationTime() + Interfaces::m_pGlobalVars->interval_per_tick);

			m_bForceVelocity = false;
		}

		SimulationRecordBackup.Apply(player);
		player->InvalidatePhysicsRecursive(8);
	}
}