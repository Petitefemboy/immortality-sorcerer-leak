#include "LagCompensation.hpp"
#include "../../source.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"

#include "../../SDK/CVariables.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/core.hpp"
#include "../../Utils/FnvHash.hpp"

#include "../Visuals/EventLogger.hpp"

#include "Autowall.h"
#include "../Miscellaneous/Movement.hpp"

#include "../Game/SetupBones.hpp"
#include "../../Utils/Threading/Threading.h"
#include "../../Utils/Threading/shared_mutex.h"

#include "../../Utils/InputSys.hpp"

#include "../Rage/FakeLag.hpp"

#include "../../Hooking/Hooked.hpp"
#include "../Game/Prediction.hpp"
#include "../Game/SimulationContext.hpp"
#include "AnimationSystem.hpp"


#include <sstream>

#define MT_SETUP_BONES
//#define DEBUG_RESOLVER
namespace Engine
{
	struct LagCompData {
		std::map< int, Engine::C_EntityLagData > m_PlayerHistory;

		float m_flLerpTime, m_flOutLatency, m_flServerLatency;
		bool m_GetEvents = false;
	};

	static LagCompData _lagcomp_data;

	class C_LagCompensation : public Engine::LagCompensation {
	public:
		virtual void Update();
		virtual bool IsRecordOutOfBounds(const Engine::C_LagRecord& record, float target_time = 0.2f, int tickbase_shift = -1, bool bDeadTimeCheck = true) const;

		virtual Encrypted_t<C_EntityLagData> GetLagData(int entindex) {
			C_EntityLagData* data = nullptr;
			if (lagData->m_PlayerHistory.count(entindex) > 0)
				data = &lagData->m_PlayerHistory.at(entindex);
			return Encrypted_t<C_EntityLagData>(data);
		}

		virtual float GetLerp() const {
			return lagData.Xor()->m_flLerpTime;
		}

		virtual void ClearLagData() {
			lagData->m_PlayerHistory.clear();
		}

		C_LagCompensation() : lagData(&_lagcomp_data) { };
		virtual ~C_LagCompensation() { };

	private:
		virtual void SetupLerpTime();
		Encrypted_t<LagCompData> lagData;
	};

	C_LagCompensation g_LagComp;
	Engine::LagCompensation* Engine::LagCompensation::Get() {
		return &g_LagComp;
	}

	Engine::C_EntityLagData::C_EntityLagData() {
		;
	}

	void C_LagCompensation::Update() {
		if (!Interfaces::m_pEngine->IsInGame() || !Interfaces::m_pEngine->IsConnected() || !Interfaces::m_pEngine->GetNetChannelInfo()) {
			lagData->m_PlayerHistory.clear();
			return;
		}

		auto updateReason = 0;

		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || !g_Vars.globals.HackIsReady)
			return;

		SetupLerpTime();

		for (int i = 1; i <= Interfaces::m_pGlobalVars->maxClients; ++i) {
			auto player = C_CSPlayer::GetPlayerByIndex(i);
			if (!player || player == local || !player->IsPlayer())
				continue;

			player_info_t player_info;
			if (!Interfaces::m_pEngine->GetPlayerInfo(player->m_entIndex, &player_info)) {
				continue;
			}

			if (!player->GetClientRenderable())
				continue;

			if (Hooked::player_hooks.count(player->m_entIndex) < 1)
				continue;

			auto lag_data = Encrypted_t<C_EntityLagData>(&lagData->m_PlayerHistory[player->m_entIndex]);
			if (!lag_data.IsValid())
				continue;

			//if (!lag_data->m_sPPDUData.m_bShouldUseServerData)
			//	continue;

			float flSimTime = player->m_flSimulationTime();
			float flOldSimTime = player->m_flOldSimulationTime();

			C_LagRecord* currentRecord = nullptr, * previousRecord = nullptr, * thirdRecord = nullptr;
			if (!lag_data->m_History.empty())
			{
				currentRecord = &lag_data->m_History.at(0);
				if (currentRecord->player->IsDormant())
					currentRecord = nullptr;
				else if (lag_data->m_History.size() > 1)
				{
					previousRecord = &lag_data->m_History.at(1);
					if (previousRecord->player->IsDormant())
						previousRecord = nullptr;
				}
			}

			bool bRecievedBadRecord = false;
			bool bWasDormant = true;
			if (currentRecord && !currentRecord->player->IsDormant())
			{
				bWasDormant = false;
				if (lag_data->m_sPPDUData.m_bSimulationTimeDiffersFromRecord)
				{
					// server sent us a bad record, restore our shit to the previous record.
					if (!lag_data->m_sPPDUData.m_bLayersDiffersFromRecord)
					{
						flSimTime = currentRecord->m_flSimulationTime;
						flOldSimTime = previousRecord ? previousRecord->m_flSimulationTime : 0.0f;
						player->m_flSimulationTime() = flSimTime;
						player->m_flOldSimulationTime() = flOldSimTime;
						bRecievedBadRecord = true;
					}
				}
			}

			lag_data->m_bUpdatedRecordData = false;

			// create a new record if we have valid information from the server.
			if (!bRecievedBadRecord)
			{
				lag_data->UpdateRecordData(lag_data, previousRecord, player, player_info, updateReason);

				//if (!local->IsDead())
				//{
				//	if (((currentRecord != nullptr && !currentRecord->player->IsDormant()) && (previousRecord != nullptr && !previousRecord->player->IsDormant())) && currentRecord != previousRecord)
				//	{
				//		// pretty soon use this in our aimbot instead of checking through all the records and wasting time now that this is implemented.
				//		int count = 0;
				//		for (auto& tick : lag_data->m_History)
				//		{
				//			if (!IsRecordOutOfBounds(tick))
				//				++count;
				//		}
				//		lag_data->m_iValidRecordCount = count;
				//
				//		if (lag_data->m_iValidRecordCount > 0)
				//			lag_data->m_flTotalInvalidRecordTime = 0.0f;
				//		else
				//			lag_data->m_flTotalInvalidRecordTime += TICKS_TO_TIME(lag_data->m_iTicksChoked + 1);
				//
				//		//if (currentRecord->m_bSkipDueToResolver || lag_data->m_iValidRecordCount == 0)
				//		//{
				//		//	if (!bWasDormant)
				//		//	{
				//		//		// simulate player movement to retrieve correct game movement values.
				//		//		float flLowerBodyYaw = player->m_flLowerBodyYawTarget();
				//		//		int iTickbaseTicks = TIME_TO_TICKS(lag_data->m_flFirstCmdTickBaseTime);
				//
				//		//		// after looking over this twice i started to think that this would overlap and stack our data for the amount of ticks they are choking so i decided to change it to this;
				//		//		// if there is any problems with the way i am doing it now just revert back to this way you dumbass.
				//		//		//for (int i = 0; i < lag_data->m_iTicksChoked + 1; ++i)
				//		//		//{
				//		//		//	// initialize our extrapolation data
				//		//		//	SimulationContext::Instance().InitSimulationContext(player);
				//
				//		//		//	// run our extrapolation
				//		//		//	SimulationContext::Instance().ExtrapolatePlayer(currentRecord->m_flEyeYaw);
				//
				//		//		//	// set our animated data to our extrapolated data.
				//		//		//	SimulationContext::Instance().WriteSimulateContextData();
				//		//		//	if (SimulationContext::Instance().m_bRebuiltGameMovementDataWritten) {
				//		//		//		currentRecord->m_vecOrigin = SimulationContext::Instance().sPostSimulationContextData.m_vecOrigin;
				//		//		//		currentRecord->m_vecVelocity = SimulationContext::Instance().sPostSimulationContextData.m_vecVelocity;
				//		//		//		currentRecord->m_iFlags = SimulationContext::Instance().sPostSimulationContextData.flags;
				//		//		//	}
				//		//		//}
				//
				//		//		// initialize our extrapolation data
				//		//		SimulationContext::Instance().InitSimulationContext(player);
				//
				//		//		// run our extrapolation.
				//		//		for (int i = 0; i < lag_data->m_iTicksChoked + 1; ++i)
				//		//			SimulationContext::Instance().ExtrapolatePlayer(currentRecord->m_flEyeYaw);
				//
				//		//		// set our animated data to our extrapolated data.
				//		//		SimulationContext::Instance().WriteSimulateContextData();
				//		//		if (SimulationContext::Instance().m_bRebuiltGameMovementDataWritten) 
				//		//		{
				//		//			currentRecord->m_vecOrigin = SimulationContext::Instance().sPostSimulationContextData.m_vecOrigin;
				//		//			currentRecord->m_vecVelocity = SimulationContext::Instance().sPostSimulationContextData.m_vecVelocity;
				//		//			currentRecord->m_iFlags = SimulationContext::Instance().sPostSimulationContextData.flags;
				//		//		}
				//		//	}
				//		//}
				//	}
				//	else
				//	{
				//		if (lag_data->m_History.size() > 1)
				//		{
				//			// pretty soon use this in our aimbot instead of checking through all the records and wasting time now that this is implemented.
				//			int count = 0;
				//			for (auto& tick : lag_data->m_History)
				//			{
				//				if (!IsRecordOutOfBounds(tick))
				//					++count;
				//			}
				//			lag_data->m_iValidRecordCount = count;
				//
				//			if (lag_data->m_iValidRecordCount > 0)
				//				lag_data->m_flTotalInvalidRecordTime = 0.0f;
				//			else
				//				lag_data->m_flTotalInvalidRecordTime += TICKS_TO_TIME(lag_data->m_iTicksChoked + 1);
				//		}
				//	}
				//}if (!local->IsDead())
				//{
				//	if (((currentRecord != nullptr && !currentRecord->player->IsDormant()) && (previousRecord != nullptr && !previousRecord->player->IsDormant())) && currentRecord != previousRecord)
				//	{
				//		// pretty soon use this in our aimbot instead of checking through all the records and wasting time now that this is implemented.
				//		int count = 0;
				//		for (auto& tick : lag_data->m_History)
				//		{
				//			if (!IsRecordOutOfBounds(tick))
				//				++count;
				//		}
				//		lag_data->m_iValidRecordCount = count;
				//
				//		if (lag_data->m_iValidRecordCount > 0)
				//			lag_data->m_flTotalInvalidRecordTime = 0.0f;
				//		else
				//			lag_data->m_flTotalInvalidRecordTime += TICKS_TO_TIME(lag_data->m_iTicksChoked + 1);
				//
				//		//if (currentRecord->m_bSkipDueToResolver || lag_data->m_iValidRecordCount == 0)
				//		//{
				//		//	if (!bWasDormant)
				//		//	{
				//		//		// simulate player movement to retrieve correct game movement values.
				//		//		float flLowerBodyYaw = player->m_flLowerBodyYawTarget();
				//		//		int iTickbaseTicks = TIME_TO_TICKS(lag_data->m_flFirstCmdTickBaseTime);
				//
				//		//		// after looking over this twice i started to think that this would overlap and stack our data for the amount of ticks they are choking so i decided to change it to this;
				//		//		// if there is any problems with the way i am doing it now just revert back to this way you dumbass.
				//		//		//for (int i = 0; i < lag_data->m_iTicksChoked + 1; ++i)
				//		//		//{
				//		//		//	// initialize our extrapolation data
				//		//		//	SimulationContext::Instance().InitSimulationContext(player);
				//
				//		//		//	// run our extrapolation
				//		//		//	SimulationContext::Instance().ExtrapolatePlayer(currentRecord->m_flEyeYaw);
				//
				//		//		//	// set our animated data to our extrapolated data.
				//		//		//	SimulationContext::Instance().WriteSimulateContextData();
				//		//		//	if (SimulationContext::Instance().m_bRebuiltGameMovementDataWritten) {
				//		//		//		currentRecord->m_vecOrigin = SimulationContext::Instance().sPostSimulationContextData.m_vecOrigin;
				//		//		//		currentRecord->m_vecVelocity = SimulationContext::Instance().sPostSimulationContextData.m_vecVelocity;
				//		//		//		currentRecord->m_iFlags = SimulationContext::Instance().sPostSimulationContextData.flags;
				//		//		//	}
				//		//		//}
				//
				//		//		// initialize our extrapolation data
				//		//		SimulationContext::Instance().InitSimulationContext(player);
				//
				//		//		// run our extrapolation.
				//		//		for (int i = 0; i < lag_data->m_iTicksChoked + 1; ++i)
				//		//			SimulationContext::Instance().ExtrapolatePlayer(currentRecord->m_flEyeYaw);
				//
				//		//		// set our animated data to our extrapolated data.
				//		//		SimulationContext::Instance().WriteSimulateContextData();
				//		//		if (SimulationContext::Instance().m_bRebuiltGameMovementDataWritten) 
				//		//		{
				//		//			currentRecord->m_vecOrigin = SimulationContext::Instance().sPostSimulationContextData.m_vecOrigin;
				//		//			currentRecord->m_vecVelocity = SimulationContext::Instance().sPostSimulationContextData.m_vecVelocity;
				//		//			currentRecord->m_iFlags = SimulationContext::Instance().sPostSimulationContextData.flags;
				//		//		}
				//		//	}
				//		//}
				//	}
				//	else
				//	{
				//		if (lag_data->m_History.size() > 1)
				//		{
				//			// pretty soon use this in our aimbot instead of checking through all the records and wasting time now that this is implemented.
				//			int count = 0;
				//			for (auto& tick : lag_data->m_History)
				//			{
				//				if (!IsRecordOutOfBounds(tick))
				//					++count;
				//			}
				//			lag_data->m_iValidRecordCount = count;
				//
				//			if (lag_data->m_iValidRecordCount > 0)
				//				lag_data->m_flTotalInvalidRecordTime = 0.0f;
				//			else
				//				lag_data->m_flTotalInvalidRecordTime += TICKS_TO_TIME(lag_data->m_iTicksChoked + 1);
				//		}
				//	}
				//}

				lag_data->m_bUpdatedRecordData = true;
			}
			//else
			//{
			//	if (!local->IsDead() && lag_data->m_History.size() > 1)
			//	{
			//		// pretty soon use this in our aimbot instead of checking through all the records and wasting time now that this is implemented.
			//		int count = 0;
			//		for (auto& tick : lag_data->m_History)
			//		{
			//			if (!IsRecordOutOfBounds(tick))
			//				++count;
			//		}
			//		lag_data->m_iValidRecordCount = count;
			//
			//		if (lag_data->m_iValidRecordCount > 0)
			//			lag_data->m_flTotalInvalidRecordTime = 0.0f;
			//		else
			//			lag_data->m_flTotalInvalidRecordTime += TICKS_TO_TIME(lag_data->m_iTicksChoked + 1);
			//	}
			//}
			//
			// commented this for testing.
			//if (!lag_data->m_bUpdatedRecordData)
			//{
			//	// reset values for next tick.
			//	lag_data->m_bShouldNetUpdate = false;
			//	lag_data->m_bNetUpdateWasSilent = false;
			//	lag_data->m_sPPDUData.ResetData();
			//}
		}
	}

	bool C_LagCompensation::IsRecordOutOfBounds(const Engine::C_LagRecord& record, float flTargetTime, int nTickbaseShiftTicks, bool bDeadTimeCheck) const {
		Encrypted_t<INetChannel> pNetChannel = Encrypted_t<INetChannel>(Interfaces::m_pEngine->GetNetChannelInfo());
		if (!pNetChannel.IsValid())
			return true;

		C_CSPlayer* pLocal = C_CSPlayer::GetLocalPlayer();
		if (!pLocal)
			return false;

		float correct = 0;
		correct += lagData.Xor()->m_flLerpTime;
		correct += pNetChannel.Xor()->GetLatency(0);
		correct += pNetChannel.Xor()->GetLatency(1);
		correct = std::clamp(correct, 0.f, g_Vars.sv_maxunlag->GetFloat());

		int latencyTicks = std::max(0, TIME_TO_TICKS(pNetChannel.Xor()->GetLatency(0)));
		int serverTickCount = Interfaces::m_pEngine->GetServerTick() + latencyTicks + 1;
		float serverTimeAtFrameEndFromLastFrame = TICKS_TO_TIME(serverTickCount - 1);

		int deltaticks = record.m_iLaggedTicks + 1;
		int updatedelta = Interfaces::m_pEngine->GetServerTick() - record.m_iServerTick;

		// only check if record is deleted if enemy would have sent another tick to the server already
		if (latencyTicks > deltaticks - updatedelta) {
			int flDeadtime = (serverTimeAtFrameEndFromLastFrame - g_Vars.sv_maxunlag->GetFloat());
			if (flTargetTime < flDeadtime) {
				return true;
			}
		}

		float deltaTime = correct - (Interfaces::m_pGlobalVars->curtime - record.m_flSimulationTime);
		if (fabsf(deltaTime) > flTargetTime) {
			return true;
		}

		return false;
	}

	void C_LagCompensation::SetupLerpTime() {
		float updaterate = g_Vars.cl_updaterate->GetFloat();
		float minupdaterate = g_Vars.sv_minupdaterate->GetFloat();
		float maxupdaterate = g_Vars.sv_maxupdaterate->GetFloat();
		float flInterpRatio = g_Vars.cl_interp_ratio->GetFloat();
		float flLerpAmount = g_Vars.cl_interp->GetFloat();
		float min_interp = g_Vars.sv_client_min_interp_ratio->GetFloat();
		float max_interp = g_Vars.sv_client_max_interp_ratio->GetFloat();

		float_t flUpdateRate = std::clamp(updaterate, minupdaterate, maxupdaterate);
		float_t flLerpRatio = std::clamp(flInterpRatio, min_interp, max_interp);
		lagData->m_flLerpTime = std::clamp(flLerpRatio / flUpdateRate, flLerpAmount, 1.0f);

		auto netchannel = Encrypted_t<INetChannel>(Interfaces::m_pEngine->GetNetChannelInfo());
		lagData->m_flOutLatency = netchannel->GetLatency(FLOW_OUTGOING);
		lagData->m_flServerLatency = netchannel->GetLatency(FLOW_INCOMING);
	}

	// sonth touched this a fucking lot.
	void Engine::C_EntityLagData::UpdateRecordData(Encrypted_t< C_EntityLagData > pThis, C_LagRecord* previousRecord, C_CSPlayer* player, const player_info_t& info, int updateType) {
		auto local = C_CSPlayer::GetLocalPlayer();
		auto team_check = g_Vars.rage.enabled && !g_Vars.rage.team_check && player->IsTeammate(C_CSPlayer::GetLocalPlayer());
		if (player->IsDead() || team_check) {
			pThis->m_History.clear();
			return;
		}

		// no need to store insane amount of data
		while (pThis->m_History.size() > int(1.0f / Interfaces::m_pGlobalVars->interval_per_tick)) { // TODO; eventually hook LevelInitPostEntity and set this as a variable in there so we get the most accurate server tickrate for this check!
			pThis->m_History.pop_back();
		}

		float flSimTime = player->m_flSimulationTime();
		float flOldSimTime = player->m_flOldSimulationTime();

		if (player->IsDormant() || player->IsDead())
		{
			if (pThis->m_bShouldNetUpdate)
			{
				pThis->m_bShouldNetUpdate = false;
				pThis->m_bNetUpdateWasSilent = false;
				pThis->m_sPPDUData.ResetData();
			}
			pThis->m_History.clear();
			return;
		}

		if (info.userId != pThis->m_iUserID) {
			pThis->Clear();
			pThis->m_iUserID = info.userId;
		}

		auto animData = AnimationSystem::Get()->GetAnimationData(player->m_entIndex);
		if (!animData)
			return;

		if (animData->m_AnimationRecord.empty() || animData->m_AnimationRecord.size() < 2)
			return;

		auto animRecord = &animData->m_AnimationRecord.front();

		// commented this as i am going to redo this check.
		if (animRecord->m_bShiftingTickbase)
			return;

		bool bJustMaterialized = false;
		bool bCreateNewRecord = true;
		pThis->m_bTickbaseShiftedBackwards = false;
		pThis->m_bTickbaseShiftedForwards = false;
		pThis->m_bIsDuplicateTickbase = false;

		float flSimulationTime = player->m_flSimulationTime();
		int iTicksSinceLastUpdate = std::clamp(pThis->m_iTicksSinceLastServerUpdate + 1, 0, 18); //clamp((Interfaces::m_pClientState->m_ClockDriftMgr.m_nServerTick - pThis->m_iServerTickCount), 0, 16);
		int iOldTickbase = TIME_TO_TICKS(pThis->m_sPPDUData.m_flPreSimulationTime) + 1;
		int iNewTickbase = TIME_TO_TICKS(flSimulationTime) + 1;
		int iTicksSinceLastSimulation = iNewTickbase - iOldTickbase;
		pThis->m_iTickBase = iNewTickbase;

		if (!previousRecord || animData->m_bIsDormant)
		{
			pThis->m_iTicksChoked = 0;
			pThis->m_iTicksSinceLastServerUpdate = 0;
			pThis->m_iServerTickCount = Interfaces::m_pClientState->m_ClockDriftMgr.m_nServerTick;
			pThis->m_flFirstCmdTickBaseTime = flSimulationTime;
			pThis->m_flNewestSimulationTime = flSimulationTime;
			pThis->m_iNewestTickBase = iNewTickbase;
		}
		else if (iNewTickbase != iOldTickbase || pThis->m_sPPDUData.m_bShotTimeDiffersFromRecord || pThis->m_sPPDUData.m_bOriginDiffersFromRecord || pThis->m_sPPDUData.m_bLayersDiffersFromRecord)
		{
			iTicksSinceLastUpdate = std::clamp((Interfaces::m_pClientState->m_ClockDriftMgr.m_nServerTick - pThis->m_iServerTickCount), 0, 18);

			if (iTicksSinceLastSimulation < 0)
			{
				pThis->m_bTickbaseShiftedBackwards = true;
				pThis->m_iTicksChoked = iTicksSinceLastUpdate - 1;
				pThis->m_flFirstCmdTickBaseTime = TICKS_TO_TIME(iNewTickbase - iTicksSinceLastUpdate - 1);
			}
			else if (iTicksSinceLastSimulation == 0)
			{
				pThis->m_iTicksChoked = iTicksSinceLastUpdate - 1;
				pThis->m_flFirstCmdTickBaseTime = TICKS_TO_TIME(iNewTickbase - iTicksSinceLastUpdate - 1);
			}
			else if (abs(iTicksSinceLastSimulation - iTicksSinceLastUpdate) > 0 && previousRecord->m_bTickbaseShiftedBackwards)
			{
				//did their tickbase shift forwards
				pThis->m_bTickbaseShiftedForwards = true; //m_bTickbaseShiftedBackwards
				pThis->m_iTicksChoked = iTicksSinceLastUpdate - 1;
				pThis->m_flFirstCmdTickBaseTime = TICKS_TO_TIME(iNewTickbase - iTicksSinceLastUpdate - 1);
			}
			else
			{
				pThis->m_iTicksChoked = iTicksSinceLastSimulation - 1;
				pThis->m_flFirstCmdTickBaseTime = TICKS_TO_TIME(iNewTickbase - iTicksSinceLastSimulation - 1);
			}

			pThis->m_iServerTickCount = Interfaces::m_pClientState->m_ClockDriftMgr.m_nServerTick;
			pThis->m_iTicksChoked = std::clamp(pThis->m_iTicksChoked, 0, 16);

			//now check for duplicates
			if (flSimulationTime <= pThis->m_flNewestSimulationTime)
				pThis->m_bIsDuplicateTickbase = true;
			else
			{
				for (auto& tick : pThis->m_History)
				{
					if (tick.m_iTickBase == iNewTickbase)
					{
						//we can't backtrack this new record we received :'(
						pThis->m_bIsDuplicateTickbase = true;
						break;
					}
				}
			}

			pThis->m_iTicksSinceLastServerUpdate = 0;
		}
		// nothing changed so we can just take the last value
		else
		{
			pThis->m_iTicksChoked = previousRecord->m_iLaggedTicks;
			++pThis->m_iTicksSinceLastServerUpdate;
			bCreateNewRecord = false;
		}

		pThis->m_iTicksChoked = std::clamp(pThis->m_iTicksChoked, 0, 16);

		C_LagRecord* record = nullptr;
		if (bCreateNewRecord)
		{
			// push back our current record.
			record = &pThis->m_History.emplace_front();

			// store record data
			record->Setup(player);
			record->m_flRealTime = Interfaces::m_pEngine->GetLastTimeStamp();
			record->m_flServerLatency = Engine::LagCompensation::Get()->m_flServerLatency;
			record->m_flDuckAmount = animRecord->m_flDuckAmount;
			record->m_flEyeYaw = animRecord->m_angEyeAngles.yaw;
			record->m_flEyePitch = animRecord->m_angEyeAngles.pitch;
			record->m_bIsShoting = animRecord->m_bIsShoting;
			record->m_bIsValid = !animRecord->m_bIsInvalid;
			record->m_bBonesCalculated = animData->m_bBonesCalculated;
			record->m_flAnimationVelocity = player->m_PlayerAnimState()->m_velocity_length_xy;
			record->m_bTeleporting = animRecord->m_bTeleporting;
			record->m_flAbsRotation = player->m_PlayerAnimState()->m_abs_yaw;
			record->m_iLaggedTicks = TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime());
			record->m_bResolved = animRecord->m_bResolved;
			record->m_iResolverMode = animRecord->m_iResolverMode;
			record->m_iServerTick = Interfaces::m_pEngine->GetServerTick();
			record->m_iTickBase = pThis->m_iTickBase;
			//record->m_iTickCount = Interfaces::m_pGlobalVars->tickcount;
			//record->m_flLowerBodyYawTarget = animRecord->m_flLowerBodyYawTarget;
			/*record->m_pPlayerAnimStateServer = animRecord->m_serverPlayerAnimState;*/

			std::memcpy(record->m_serverAnimationLayers, animRecord->m_serverAnimOverlays, sizeof(animRecord->m_serverAnimOverlays));
			std::memcpy(record->m_BoneMatrix, animData->m_Bones, player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

			auto weapon = (C_WeaponCSBaseGun*)player->m_hActiveWeapon().Get();
			if (weapon)
			{
				const float flLastShotTime = weapon->m_fLastShotTime();
				if (flLastShotTime >= pThis->m_flFirstCmdTickBaseTime)
				{
					record->m_bIsShoting = true;

					// TODO; eventually store another record here just for their on shot record and use that later for backtracking and aimbot
				}
				else
				{
					record->m_angEyeAnglesNotFiring = QAngle(record->m_flEyePitch, record->m_flEyeYaw, 0);
				}

				record->m_flLastShotTime = flLastShotTime;
			}

			record->m_bCreatedNewRecord = true;
			record->m_bCreatedNewRecordUsingPreviousData = false;
		}

		// we don't need to create a new record as we can use our old data.
		else
		{
			record = previousRecord;

			// what the fuck server send us our update
			if (pThis->m_bNetUpdateWasSilent)
			{
				// this players origin changed during this update.
				if (pThis->m_sPPDUData.m_bOriginDiffersFromRecord)
				{
					Vector newOrigin = pThis->m_sPPDUData.m_vecPostNetOrigin;
					player->SetAbsOrigin(newOrigin);
					record->m_vecOrigin = newOrigin;
				}

				// possibly set these variables to the entity's data here too like what we do with origin above.

				if (pThis->m_sPPDUData.m_bAbsAnglesDiffersFromRecord)
					record->m_angAngles = pThis->m_sPPDUData.m_angPostAbsAngles;

				if (pThis->m_sPPDUData.m_bEyeAnglesDiffersFromRecord)
				{
					record->m_flEyePitch = pThis->m_sPPDUData.m_angPostEyeAngles.x;
					record->m_flEyeYaw = pThis->m_sPPDUData.m_angPostEyeAngles.y;
				}
			}

			record->m_bCreatedNewRecord = false;
			record->m_bCreatedNewRecordUsingPreviousData = false;
		}

		// i know this looks stupid because i could've called this in the check above
		// but i wanted to make sure that we have accurately set our record, all of our record data, and updated properly before setting the rest of our data below,
		// so that's why i made another check and called it down here.
		if (bCreateNewRecord && previousRecord)
		{
			for (auto& tick : pThis->m_History)
			{
				if (&tick != record && !tick.m_bIsDuplicateTickbase && !tick.m_bTickbaseShiftedBackwards)
				{
					// possibly define this using record here??
					tick.m_bSkipDueToResolver = !tick.player->IsDormant() && (record->m_bTeleporting || (record->m_vecOrigin - tick.m_vecOrigin).LengthSquared() > 4096.f);
					break;
				}
				// possibly call a break right here so that way this loop isn't always running.
				break;
			}

			pThis->m_bSafeToRunLBYPrediction = false;

			float flServerTime = TICKS_TO_TIME(Interfaces::m_pClientState->m_ClockDriftMgr.m_nServerTick);

			const float flLowerBodyYaw = player->m_flLowerBodyYawTarget();
			if (flLowerBodyYaw != pThis->m_flLowerBodyYaw)
			{
				pThis->m_flLowerBodyYaw = flLowerBodyYaw;
				pThis->m_flLastLBYUpdateTime = flServerTime;
				pThis->m_bDidLBYChange = true;

				if (!previousRecord->player->IsDormant())
				{
					int iTotalNewCommands = pThis->m_iTicksChoked + 1;
					Vector vecAbsVelocitySlope = (record->m_vecAbsVelocity - previousRecord->m_vecAbsVelocity) / iTotalNewCommands;
					Vector vecAbsVelocity = previousRecord->m_vecAbsVelocity;
					Vector vecLastVelocity = Vector(previousRecord->m_vecVelocity.x, previousRecord->m_flAnimationVelocity, previousRecord->m_vecVelocity.z);
					float flSimTime = pThis->m_flFirstCmdTickBaseTime - TICKS_TO_TIME(1);
					bool bLBYTimerUpdated = false;

					for (int i = 0; i < iTotalNewCommands; ++i)
					{
						vecAbsVelocity += vecAbsVelocitySlope;
						flSimTime += TICKS_TO_TIME(1);
						Vector absVelocity = absVelocity;
						absVelocity.z = 0.0f;
						vecLastVelocity = Math::GetSmoothedVelocity(TICKS_TO_TIME(1) * 2000.f, absVelocity, vecLastVelocity);

						// possibly clamp this to max speed, but why the fuck would i do shit the proper way and not the shit hardcoded way?!??!
						float flSpeed = fminf(vecLastVelocity.Length(), 260.f);
						if (flSpeed > 0.1f || fabsf(vecAbsVelocity.z) > 100.f)
						{
							pThis->m_flNextLBYUpdateTime = flSimTime + 0.22f;
							bLBYTimerUpdated = true;
							break;
						}
						else if (flSimTime > pThis->m_flNextLBYUpdateTime)
						{
							pThis->m_flNextLBYUpdateTime = flSimTime + 1.1f;
							bLBYTimerUpdated = true;
							break;
						}
					}

					// force the lby timer to change, this might have to be reworked as i don't think this is the most accurate estimate of when the lby is going to change next.
					if (!bLBYTimerUpdated)
						pThis->m_flNextLBYUpdateTime = pThis->m_flFirstCmdTickBaseTime + TICKS_TO_TIME(std::max(0, record->m_iLaggedTicks - 1));
				}
			}

			pThis->m_bTriggeredBalanceAdjust = false;
			pThis->m_bFlickedToLBY = false;

			C_AnimationLayer& fireSequence = player->GetAnimLayer(ANIMATION_LAYER_ADJUST);
			C_AnimationLayer& previousFireSequence = previousRecord->m_serverAnimationLayers[ANIMATION_LAYER_ADJUST];
			auto act = player->GetSequenceActivity(fireSequence.m_nSequence);
			auto prevAct = player->GetSequenceActivity(previousFireSequence.m_nSequence);
			bool bRestartedBalanceAdjust = false;

			if (act == ACT_CSGO_IDLE_TURN_BALANCEADJUST || act == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING)
			{
				if (prevAct != act || fireSequence.m_flCycle < previousFireSequence.m_flCycle)
				{
					bRestartedBalanceAdjust = true;
					pThis->m_bTriggeredBalanceAdjust = true;
					pThis->m_flLastBalanceAdjustYaw = record->m_flEyeYaw;// eventually change this out for our proxied eye angles.

					float flOldTimeSInceLastBalanceAdjust = pThis->m_flTimeSinceLastBalanceAdjust;
					pThis->m_flTimeSinceLastBalanceAdjust = fmin(flServerTime - pThis->m_flLastBalanceAdjust, 200.f);
					pThis->m_flLastBalanceAdjust = flServerTime;

					// improved our dogass fakeflick check
					// testing this first condition.
					if (record->m_vecVelocity.Length() < 16.f
					&& (pThis->m_flTimeSinceLastBalanceAdjust >= 0.9f 
					&&  flOldTimeSInceLastBalanceAdjust > 0.9f 
					&&  flOldTimeSInceLastBalanceAdjust <= 2.5f 
					&&  pThis->m_flTimeSinceLastBalanceAdjust <= 2.5f))
						animRecord->m_bUnsafeVelocityTransition = true;
				}
			}

			if (pThis->m_flTimeSinceLastBalanceAdjust <= 0.2f || pThis->m_flTimeSinceLastBalanceAdjust > 3.0f)
				animRecord->m_bUnsafeVelocityTransition = false;

			pThis->m_flTimeSinceLastBalanceAdjust = fmin(flServerTime - pThis->m_flLastBalanceAdjust, 200.f);

			if (record->m_flSimulationTime > pThis->m_flNextLBYUpdateTime
				&& !pThis->m_bDidLBYChange && bRestartedBalanceAdjust)
			{
				pThis->m_bFlickedToLBY = true;
				pThis->m_bDidLBYBreakWithAHighDelta = true;
			}

			if (pThis->m_flTimeSinceLastBalanceAdjust > 1.5f)
				pThis->m_bDidLBYBreakWithAHighDelta = false;

			pThis->m_bSafeToRunLBYPrediction = true;

			//record->m_flLowerBodyYawTarget = pThis->m_flLowerBodyYaw;

			record->m_bCreatedNewRecordUsingPreviousData = true;
		}

		if (flSimTime > pThis->m_flNewestSimulationTime)
		{
			pThis->m_flNewestSimulationTime = flSimTime;
			pThis->m_iNewestTickBase = TIME_TO_TICKS(pThis->m_flNewestSimulationTime) + 1;
		}

		if (record->m_bCreatedNewRecord || record->m_bCreatedNewRecordUsingPreviousData)
			record->m_bShouldOverrideAnimationData = true;

		// reset values for next tick.
		pThis->m_bShouldNetUpdate = false;
		pThis->m_bNetUpdateWasSilent = false;
		pThis->m_sPPDUData.ResetData();
	}

	bool Engine::C_EntityLagData::DetectAutoDirerction(Encrypted_t< C_EntityLagData > pThis, C_CSPlayer* player) {
		if (!player || player->IsDead())
			return false;

		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || local->IsDead())
			return false;

		auto weapon = (C_WeaponCSBaseGun*)local->m_hActiveWeapon().Get();
		if (!weapon)
			return false;

		auto weaponData = weapon->GetCSWeaponData();
		if (!weaponData.IsValid())
			return false;

		auto eye_position = g_Vars.globals.m_vecFixedEyePosition;
		auto enemy_eye_position = player->GetEyePosition();

		auto delta = enemy_eye_position - eye_position;
		delta.Normalize();

		int iterator = 0;

		bool result = false;

		CTraceFilterWorldOnly filter;

		float edges[4] = { 0.0f };

		// TODO: find out, can we increase accuracy of this method?
		float directionRag = atan2(delta.y, delta.x);
		float atTargetYaw = RAD2DEG(directionRag);
		for (int i = 0; i < 4; ++i) {
			float radAngle = DEG2RAD((i * 90.0f) + atTargetYaw);
			float angleSin, angleCos;
			DirectX::XMScalarSinCos(&angleSin, &angleCos, radAngle);

			Vector pos;
			pos.x = enemy_eye_position.x + (angleCos * 16.0f);
			pos.y = enemy_eye_position.y + (angleSin * 16.0f);
			pos.z = enemy_eye_position.z;

			Vector extrapolated = eye_position;

			Autowall::C_FireBulletData data;

			data.m_bPenetration = true;
			data.m_Player = local;
			data.m_Weapon = weapon;
			data.m_WeaponData = weaponData.Xor();
			data.m_vecStart = extrapolated;
			data.m_bShouldIgnoreDistance = true;

			data.m_vecDirection = pos - extrapolated;
			data.m_flPenetrationDistance = data.m_vecDirection.Normalize();
			data.m_Filter = &filter;

			float dmg = Autowall::FireBullets(&data);

			if (dmg >= 1.0f)
				result = true;
			else if (dmg < 0.0f)
				dmg = 0.0f;

			edges[i] = dmg;
		}

		if (result
			&& (int(edges[0]) != int(edges[1])
				|| int(edges[0]) != int(edges[2])
				|| int(edges[0]) != int(edges[3]))) {
			pThis->m_flEdges[0] = edges[0];
			pThis->m_flEdges[1] = edges[1];
			pThis->m_flEdges[2] = edges[2];
			pThis->m_flEdges[3] = edges[3];

			pThis->m_flDirection = atTargetYaw;
		}
		else {
			result = false;
		}

		return result;
	}

	void Engine::C_EntityLagData::Clear() {
		this->m_History.clear();
		this->m_iTicksChokedHistory.clear();
		m_iUserID = -1;
		m_flLastScanTime = 0.0f;
		m_flLastUpdateTime = 0.0f;
	}

	float C_LagRecord::GetAbsYaw() {
		return this->m_angAngles.yaw;
	}

	matrix3x4_t* C_LagRecord::GetBoneMatrix() {
		if (!this->m_bBonesCalculated)
			return this->m_BoneMatrix;

		return this->m_BoneMatrix;
	}

	void Engine::C_LagRecord::Setup(C_CSPlayer* player) {
		auto collidable = player->m_Collision();
		this->m_vecMins = collidable->m_vecMins;
		this->m_vecMaxs = collidable->m_vecMaxs;

		this->m_vecOrigin = player->m_vecOrigin();
		this->m_angAngles = player->GetAbsAngles();

		this->m_vecVelocity = player->m_vecVelocity();
		this->m_vecAbsVelocity = player->GetAbsVelocity();

		this->m_flSimulationTime = player->m_flSimulationTime();

		this->m_iFlags = player->m_fFlags();

		std::memcpy(this->m_BoneMatrix, player->m_CachedBoneData().Base(),
			player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

		this->player = player;
	}

	void Engine::C_LagRecord::Apply(C_CSPlayer* player) {
		auto collidable = player->m_Collision();
		collidable->SetCollisionBounds(this->m_vecMins, this->m_vecMaxs);

		player->m_flSimulationTime() = this->m_flSimulationTime;

		QAngle absAngles = this->m_angAngles;
		absAngles.yaw = this->GetAbsYaw();

		player->m_fFlags() = this->m_iFlags;

		player->SetAbsAngles(absAngles);
		player->SetAbsOrigin(this->m_vecOrigin);

		matrix3x4_t* matrix = GetBoneMatrix();

		if (matrix) {
			std::memcpy(player->m_CachedBoneData().Base(), matrix,
				player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

			// force bone cache
			player->m_iMostRecentModelBoneCounter() = *(int*)Engine::Displacement.Data.m_uModelBoneCounter;
			player->m_BoneAccessor().m_ReadableBones = player->m_BoneAccessor().m_WritableBones = 0xFFFFFFFF;
			player->m_flLastBoneSetupTime() = FLT_MAX;
		}
	}

	void C_BaseLagRecord::Setup(C_CSPlayer* player) {
		auto collidable = player->m_Collision();
		this->m_vecMins = collidable->m_vecMins;
		this->m_vecMaxs = collidable->m_vecMaxs;

		this->m_flSimulationTime = player->m_flSimulationTime();

		this->m_angAngles = player->GetAbsAngles();
		this->m_vecOrigin = player->m_vecOrigin();

		if (player->m_PlayerAnimState() != nullptr)
			this->m_flAbsRotation = player->m_PlayerAnimState()->m_abs_yaw;

		std::memcpy(this->m_BoneMatrix, player->m_CachedBoneData().Base(),
			player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

		this->player = player;
	}

	void C_BaseLagRecord::Apply(C_CSPlayer* player) {
		auto collidable = player->m_Collision();
		collidable->SetCollisionBounds(this->m_vecMins, this->m_vecMaxs);

		player->m_flSimulationTime() = this->m_flSimulationTime;

		player->SetAbsAngles(this->m_angAngles);
		player->SetAbsOrigin(this->m_vecOrigin);

		if (player->m_PlayerAnimState() != nullptr)
			player->m_PlayerAnimState()->m_abs_yaw = this->m_flAbsRotation;

		std::memcpy(player->m_CachedBoneData().Base(), this->m_BoneMatrix,
			player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

		// force bone cache
		player->m_iMostRecentModelBoneCounter() = *(int*)Engine::Displacement.Data.m_uModelBoneCounter;
		player->m_BoneAccessor().m_ReadableBones = player->m_BoneAccessor().m_WritableBones = 0xFFFFFFFF;
		player->m_flLastBoneSetupTime() = FLT_MAX;
	}
}