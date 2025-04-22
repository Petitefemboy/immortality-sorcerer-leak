#include "../sdk.hpp"
#include "../../source.hpp"
#include "../Displacement.hpp"

void IPrediction::Update( int startframe, bool validframe, int incoming_acknowledged, int outgoing_command ) {
	using Fn = void( __thiscall* )( void*, int, bool, int, int );
	return Memory::VCall<Fn>( this, Index::IPrediction::Update )( this, startframe, validframe, incoming_acknowledged, outgoing_command );
}

void IPrediction::SetupMove( C_BasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move ) {
	using Fn = void( __thiscall* )( void*, C_BasePlayer*, CUserCmd*, IMoveHelper*, CMoveData* );
	return Memory::VCall<Fn>( this, Index::IPrediction::SetupMove )( this, player, ucmd, pHelper, move );
}

void IPrediction::FinishMove( C_BasePlayer* player, CUserCmd* ucmd, CMoveData* move ) {
	using Fn = void( __thiscall* )( void*, C_BasePlayer*, CUserCmd*, CMoveData* );
	return Memory::VCall<Fn>( this, Index::IPrediction::FinishMove )( this, player, ucmd, move );
}

void IPrediction::SetLocalViewAngles( const QAngle& ang ) {
	return Memory::VCall< void( __thiscall* )( decltype( this ), const QAngle& ) >( this, 13 )( this, ang );
}

int IPrediction::CheckMovingGround( C_BasePlayer* player, double unk ) {
	using Fn = int( __thiscall* )( void*, C_BasePlayer*, double );
	return Memory::VCall<Fn>( this, Index::IPrediction::CheckMovingGround )( this, player, unk );
}

int& IPrediction::m_nCommandsPredicted( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.IPrediction.m_nCommandsPredicted );
}

bool IPrediction::InPrediction( ) {
	using Fn = bool( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, Index::IPrediction::InPrediction )( this );
}

CGlobalVarsBase* IPrediction::GetUnpredictedGlobals( ) {
	if( InPrediction( ) )
		return reinterpret_cast< CGlobalVarsBase* >( uint32_t( this ) + 0x4c );

	return Interfaces::m_pGlobalVars.Xor( );
}

//int CPrediction::ComputeFirstCommandToExecute_New(C_BaseEntity* localPlayer, int nSlot, bool received_new_world_update, int incoming_acknowledged, int outgoing_command)
//{
//	//return StaticOffsets.GetOffsetValueByType<int(__thiscall*)(CPrediction*, int, bool, int, int)>(_ComputeFirstCommandToExecute)(this, nSlot, received_new_world_update, incoming_acknowledged, outgoing_command);
//
//	int destination_slot = 1;
//	int skipahead = 0;
//
//	Split_t& split = m_Split[nSlot];
//
//	// If we didn't receive a new update ( or we received an update that didn't ack any new CUserCmds -- 
//	//  so for the player it should be just like receiving no update ), just jump right up to the very 
//	//  last command we created for this very frame since we probably wouldn't have had any errors without 
//	//  being notified by the server of such a case.
//	// NOTE:  received_new_world_update only gets set to false if cl_pred_optimize >= 1
//	if (!received_new_world_update || !split.m_nServerCommandsAcknowledged)
//	{
//		if (!split.m_bUnknown2) //csgo specific
//		{
//			if (!localPlayer->IsLocalPlayer() || g_Vars.cl_pred_optimize->GetBool()) //dylan added
//			{
//				// this is where we would normally start
//				int start = incoming_acknowledged + 1;
//				// outgoing_command is where we really want to start
//				skipahead = std::max(0, (outgoing_command - start));
//				// Don't start past the last predicted command, though, or we'll get prediction errors
//				skipahead = std::min(skipahead, split.m_nCommandsPredicted);
//
//				// Always restore since otherwise we might start prediction using an "interpolated" value instead of a purely predicted value
//				RestoreEntityToPredictedFrame(nSlot, skipahead - 1);
//
//				//Msg( "%i/%i no world, skip to %i restore from slot %i\n", 
//				//	gpGlobals->framecount,
//				//	gpGlobals->tickcount,
//				//	skipahead,
//				//	skipahead - 1 );
//			}
//		}
//	}
//	else
//	{
//		// Otherwise, there is a second optimization, wherein if we did receive an update, but no
//		//  values differed (or were outside their epsilon) and the server actually acknowledged running
//		//  one or more commands, then we can revert the entity to the predicted state from last frame, 
//		//  shift the # of commands worth of intermediate state off of front the intermediate state array, and
//		//  only predict the usercmd from the latest render frame.
//		if (g_Vars.cl_pred_optimize->GetInt() >= 2 &&
//			!split.m_bPreviousAckHadErrors &&
//			split.m_nCommandsPredicted > 0 &&
//			split.m_nServerCommandsAcknowledged <= split.m_nCommandsPredicted
//			&& !split.m_bUnknown2) //< csgo specific
//		{
//			// Copy all of the previously predicted data back into entity so we can skip repredicting it
//			// This is the final slot that we previously predicted
//			RestoreEntityToPredictedFrame(nSlot, split.m_nCommandsPredicted - 1);
//
//			// Shift intermediate state blocks down by # of commands ack'd
//			ShiftIntermediateDataForward(nSlot, split.m_nServerCommandsAcknowledged, split.m_nCommandsPredicted);
//
//			// Only predict new commands (note, this should be the same number that we could compute
//			//  above based on outgoing_command - incoming_acknowledged - 1
//			skipahead = (split.m_nCommandsPredicted - split.m_nServerCommandsAcknowledged);
//
//			//Msg( "%i/%i optimize2, skip to %i restore from slot %i\n", 
//			//	gpGlobals->framecount,
//			//	gpGlobals->tickcount,
//			//	skipahead,
//			//	split.m_nCommandsPredicted - 1 );
//		}
//		else
//		{
//			//csgo specific
//			if (split.m_bPreviousAckHadErrors)
//			{
//				CPredictableList* predictables = GetPredictables(nSlot);
//				for (int i = 0; i < predictables->GetPredictableCount(); ++i)
//				{
//					C_BaseEntity* predictable = predictables->GetPredictable(i);
//					if (predictable)
//					{
//						predictable->SetUnknownEntityPredictionBool(true);
//					}
//				}
//			}
//
//			if ((split.m_bPreviousAckHadErrors && g_Vars.cl_pred_doresetlatch->GetBool()) ||
//				g_Vars.cl_pred_doresetlatch->GetInt() == 2)
//			{
//				// Both players should have == time base, etc.
//				C_CSPlayer* pLocalPlayer = C_CSPlayer::GetLocalPlayer();//C_BasePlayer::GetLocalPlayer(nSlot);
//
//				// If an entity gets a prediction error, then we want to clear out its interpolated variables
//				// so we don't mix different samples at the same timestamps. We subtract 1 tick interval here because
//				// if we don't, we'll have 3 interpolation entries with the same timestamp as this predicted
//				// frame, so we won't be able to interpolate (which leads to jerky movement in the player when
//				// ANY entity like your gun gets a prediction error).
//				float flPrev = Interfaces::m_pGlobalVars->curtime;
//				Interfaces::m_pGlobalVars->curtime = TICKS_TO_TIME(pLocalPlayer->GetTickBase()) - Interfaces::m_pGlobalVars->interval_per_tick;
//
//				if (split.m_bUnknown || g_Vars.cl_pred_doresetlatch->GetInt() == 2) 	//csgo specific
//				{
//					CPredictableList* predictables = GetPredictables(nSlot);
//					for (int i = 0; i < predictables->GetPredictableCount(); ++i)
//					{
//						C_BaseEntity* entity = predictables->GetPredictable(i);
//						if (entity)
//						{
//							entity->ResetLatched();
//						}
//					}
//
//					Interfaces::m_pGlobalVars->curtime = flPrev;
//				}
//				else //csgo specific
//				{
//					for (int i = 0; i < split.m_vecPredictionHandles.Count(); ++i)
//					{
//						C_CSPlayer* pEnt = (C_CSPlayer*)Interfaces::m_pEntList->GetClientEntityFromHandle(split.m_vecPredictionHandles[i]);
//						if (pEnt)
//						{
//							CPredictableList* predictables = GetPredictables(nSlot);
//							for (int j = 0; j < predictables->GetPredictableCount(); j++)
//							{
//								C_CSPlayer* pPredictable = predictables->GetPredictable(j);
//								if (pPredictable == pEnt)
//								{
//									pEnt->ResetLatched();
//									break;
//								}
//							}
//						}
//					}
//
//					Interfaces::m_pGlobalVars->curtime = flPrev;
//				}
//			}
//		}
//	}
//
//	destination_slot += skipahead;
//
//	//csgo specific
//	ShiftFirstPredictedIntermediateDataForward(nSlot, split.m_nServerCommandsAcknowledged);
//
//	// Always reset these values now that we handled them
//	split.m_nCommandsPredicted = 0;
//	split.m_bPreviousAckHadErrors = false;
//	split.m_nServerCommandsAcknowledged = 0;
//	split.m_bUnknown2 = 0;
//
//	return destination_slot;
//}
//
//bool CPrediction::PerformPrediction(int nSlot, C_BasePlayer* localPlayer, bool received_new_world_update, int incoming_acknowledged, int outgoing_command)
//{
//	Interfaces::m_pMDLCache->BeginLock();
//
//	//Dylan added this to fix crash when resuming from being paused
//	static float LastPredictTime = QPCTime();
//	float Time = QPCTime();
//	auto m_VarMap = localPlayer->GetVarMapping();
//	if (Time - LastPredictTime > 1.0f)
//	{
//		int c = m_VarMap->m_Entries.Count();
//		for (int i = 0; i < c; i++)
//		{
//			VarMapEntry_t* e = &m_VarMap->m_Entries[i];
//			IInterpolatedVar* watcher = e->watcher;
//			watcher->Reset(Interfaces::m_pGlobalVars->curtime);
//		}
//	}
//	LastPredictTime = Time;
//
//	Assert(localPlayer);
//	// This makes sure that we are allowed to sample the world when it may not be ready to be sampled
//	Assert(C_BaseEntity::IsAbsQueriesValid());
//	Assert(C_BaseEntity::IsAbsRecomputationsEnabled());
//
//	// Start at command after last one server has processed and 
//	//  go until we get to targettime or we run out of new commands
//	int i = ComputeFirstCommandToExecute_New(localPlayer, nSlot, received_new_world_update, incoming_acknowledged, outgoing_command);
//
//	Assert(i >= 1);
//
//	// This is a hack to get the CTriggerAutoGameMovement auto duck triggers to correctly deal with prediction.
//	// Here we just untouch any triggers the player was touching (since we might have teleported the origin 
//	// backward from it's previous position) and then re-touch any triggers it's currently in
//
//	localPlayer->SetCheckUntouch(true);
//	localPlayer->PhysicsCheckForEntityUntouch();
//
//	localPlayer->PhysicsTouchTriggers();
//
//	//csgo specific
//	if (localPlayer->IsLocalPlayer())
//		localPlayer->DoLocalPlayerPrePrediction();
//
//	// undo interpolation changes for entities we stand on
//	C_BaseEntity* ground = localPlayer->GetGroundEntity();
//
//	while (ground && ground->entindex() > 0)
//	{
//		ground->MoveToLastReceivedPosition();
//		ground = ground->GetMoveParent();
//	}
//
//	Split_t& split = m_Split[nSlot];
//
//	//csgo specific
//	physenv()->SetPredictionCommandNum(incoming_acknowledged - 1);
//
//	bool bTooMany = outgoing_command - incoming_acknowledged >= MULTIPLAYER_BACKUP;
//
//	int maxfuturetick = Interfaces::m_pGlobalVars->tickcount + g_Vars.sv_max_usercmd_future_ticks->GetInt();
//	bool didshift = false;
//	CUserCmd* lastcmd = nullptr; //custom by dylan
//
//	if (!bTooMany && g_Tickbase.m_iDummyCommandsToProcess)
//	{
//		int last_processed_cmd_index = incoming_acknowledged;
//		CUserCmd* last_processed_cmd;
//		while (last_processed_cmd = Interfaces::m_pInput->GetUserCmd(nSlot, last_processed_cmd_index), last_processed_cmd&& last_processed_cmd->tick_count - Interfaces::m_pGlobalVars->tickcount > MULTIPLAYER_BACKUP)
//		{
//			--last_processed_cmd_index;
//		}
//
//		if (last_processed_cmd)
//		{
//			int backup_tick_count = last_processed_cmd->tick_count;
//			QAngle backup_eyeangles = last_processed_cmd->viewangles;
//
//			Math::NormalizeAngles(last_processed_cmd->viewangles);
//			last_processed_cmd->tick_count = Interfaces::m_pGlobalVars->tickcount;
//
//			int old_tickbase = localPlayer->GetTickBase();
//
//			for (int i = 0; i < g_Tickbase.m_iDummyCommandsToProcess; ++i)
//			{
//				float curtime = (localPlayer->GetTickBase()) * TICK_INTERVAL;
//
//				split.m_bFirstTimePredicted = !last_processed_cmd->hasbeenpredicted;
//				RunSimulation(incoming_acknowledged, curtime, last_processed_cmd, localPlayer);
//
//				Interfaces::m_pGlobalVars->curtime = curtime;
//				Interfaces::m_pGlobalVars->frametime = m_bEnginePaused ? 0 : TICK_INTERVAL;
//
//				// Call untouch on any entities no longer predicted to be touching
//				Untouch(nSlot);
//
//				// Store intermediate data into appropriate slot
//				StorePredictionResults(nSlot, i - 1); // Note that I starts at 1
//			}
//
//			int added_ticks = localPlayer->GetTickBase() - old_tickbase;
//
//			last_processed_cmd->tick_count = backup_tick_count;
//			last_processed_cmd->viewangles = backup_eyeangles;
//
//			CVerifiedUserCmd* verifiedcmd = Interfaces::m_pInput->GetVerifiedUserCmd(last_processed_cmd_index);
//
//			if (verifiedcmd)
//				verifiedcmd->m_crc = last_processed_cmd->GetChecksum();
//
//			if (added_ticks != 0)
//			{
//				for (int f = last_processed_cmd_index + 1; f <= outgoing_command; ++f)
//				{
//					CUserCmd* cmd = Interfaces::m_pInput->GetUserCmd(nSlot, f);
//					if (!cmd)
//						break;
//
//					if (g_Tickbase.m_iCustomTickbase[cmd->command_number % 150][0] != 0)
//						g_Tickbase.m_iCustomTickbase[cmd->command_number % 150][0] += added_ticks;
//				}
//			}
//		}
//	}
//
//	while (!bTooMany)
//	{
//		// Incoming_acknowledged is the last usercmd the server acknowledged having acted upon
//		int current_command = incoming_acknowledged + i;
//
//		//printf("current command: %i\n", current_command);
//
//		// We've caught up to the current command.
//		if (current_command > outgoing_command)
//			break;
//
//		CUserCmd* cmd = Interfaces::m_pInput->GetUserCmd(nSlot, current_command);
//		if (!cmd)
//		{
//			bTooMany = true;
//			break;
//		}
//
//		//fix tickbase when we are incrementing the max # of processable ticks
//		//if (lastcmd && lastcmd->tick_count > maxfuturetick)
//		{
//			//localPlayer->SetTickBase(localPlayer->GetTickBase() + 1);
//		}
//
//		//fix tickbase when we are incrementing the max # of processable ticks
//		//if (cmd->tick_count > maxfuturetick)
//		//	localPlayer->SetTickBase(localPlayer->GetTickBase() - 1);
//
//		//if the first command in a batch needs the tickbase shifted, do so now
//		int targetTickbase = g_Tickbase.m_iCustomTickbase[cmd->command_number % 150][0];
//
//#ifdef _DEBUG
//		if (cmd->command_number != current_command)
//			printf("WARNING: PredictionUpdate -> FIX THIS\n");
//#endif
//		if (targetTickbase != 0)
//		{
//			localPlayer->SetTickBase(targetTickbase);
//#ifdef _DEBUG
//			if (g_Tickbase.m_bEnableShiftPrinting)
//				printf("PredictionUpdate: first cmd curtime %f tickbase %i\n", TICKS_TO_TIME(targetTickbase), targetTickbase);
//#endif
//			didshift = true;
//		}
//
//#ifdef _DEBUG
//		if (cmd->buttons & IN_ATTACK)
//		{
//			printf("PredictionUpdate: attack curtime %f\n", TICKS_TO_TIME(localPlayer->GetTickBase()));
//		}
//#endif
//
//		Assert(i < MULTIPLAYER_BACKUP);
//
//		// Is this the first time predicting this
//		split.m_bFirstTimePredicted = !cmd->hasbeenpredicted;
//
//		// Set globals appropriately
//		float curtime = (localPlayer->GetTickBase()) * TICK_INTERVAL;
//
//		//csgo specific
//		if (physenv()->IsPredicted())
//		{
//			physenv()->SetPredictionCommandNum(current_command);
//			//only do this on the first predicted command
//			if (!split.m_nCommandsPredicted && i == 1)
//			{
//				if (!split.m_bFirstTimePredicted)
//				{
//					auto predictables = GetPredictables(nSlot);
//					for (auto i = 0; i < predictables->GetPredictableCount(); ++i)
//					{
//						C_BaseEntity* predictable = predictables->GetPredictable(i);
//						if (predictable->GetUnknownEntityPredictionBool())
//						{
//							Interfaces::Globals->curtime = curtime;
//							predictable->VPhysicsCompensateForPredictionErrors(predictable->GetFirstPredictedFrame());
//						}
//					}
//				}
//			}
//		}
//
//		//csgo specific
//		if (!cmd->hasbeenpredicted)
//		{
//			auto predictables = GetPredictables(nSlot);
//			for (auto i = 0; i < predictables->GetPredictableCount(); ++i)
//			{
//				C_BaseEntity* predictable = predictables->GetPredictable(i);
//				if (predictable)
//					predictable->SetUnknownEntityPredictionBool(false);
//			}
//		}
//
//		RunSimulation(current_command, curtime, cmd, localPlayer);
//
//		Interfaces::m_pGlobalVars->curtime = curtime;
//		Interfaces::m_pGlobalVars->frametime = m_bEnginePaused ? 0 : TICK_INTERVAL;
//
//		// Call untouch on any entities no longer predicted to be touching
//		Untouch(nSlot);
//
//		// Store intermediate data into appropriate slot
//		StorePredictionResults(nSlot, i - 1); // Note that I starts at 1
//
//		split.m_nCommandsPredicted = i;
//
//		if (current_command == outgoing_command)
//			localPlayer->SetFinalPredictedTick(localPlayer->GetTickBase());
//
//		// Mark that we issued any needed sounds, of not done already
//		cmd->hasbeenpredicted = true;
//
//		lastcmd = cmd; //custom by dylan
//
//		// Copy the state over.
//		i++;
//	}
//
//#ifdef _DEBUG
//	if (didshift && g_Tickbase.m_bEnableShiftPrinting)
//	{
//		printf("PredictionUpdate: curtime after prediction %f tickbase %i\n", TICKS_TO_TIME(localPlayer->GetTickBase()), localPlayer->GetTickBase());
//	}
//#endif
//
//	// Somehow we looped past the end of the list (severe lag), don't predict at all
//	Interfaces::m_pMDLCache->EndLock();
//	return !bTooMany;
//}
//
//void CPrediction::_Update_New(int nSlot, bool received_new_world_update, bool validframe, int incoming_acknowledged, int outgoing_command)
//{
//	QAngle viewangles;
//
//	C_CSPlayer* localPlayer = C_CSPlayer::GetLocalPlayer();
//	if (!localPlayer)
//		return;
//
//	//csgo specific
//	m_Split[nSlot].m_nIncomingAcknowledged = incoming_acknowledged;
//
//	// Always using current view angles no matter what
//	// NOTE: ViewAngles are always interpreted as being *relative* to the player
//	Interfaces::m_pEngine->GetViewAngles(viewangles);
//	localPlayer->SetLocalAngles(viewangles);
//
//	if (!validframe)
//	{
//		return;
//	}
//
//	// If we are not doing prediction, copy authoritative value into velocity and angle.
//	if (!g_Vars.cl_predict->GetInt())
//	{
//		// When not predicting, we at least must make sure the player
//		// view angles match the view angles...
//		/*localPlayer->*/SetLocalViewAngles(viewangles);
//		return;
//	}
//
//	// This is cheesy, but if we have entities that are parented to attachments on other entities, then 
//	// it'll wind up needing to get a bone transform.
//	{
//		C_BaseEntity::AutoAllowBoneAccess boneaccess(true, true);
//
//		// Invalidate bone cache on predictables
//		int c = GetPredictables(nSlot)->GetPredictableCount();
//		for (int i = 0; i < c; ++i)
//		{
//			C_BaseEntity* ent = GetPredictables(nSlot)->GetPredictable(i);
//			if (!ent ||
//				!ent->GetBaseAnimating() ||
//				!ent->GetPredictable())
//				continue;
//			ent->InvalidateBoneCache();
//		}
//
//		// Remove any purely client predicted entities that were left "dangling" because the 
//		//  server didn't acknowledge them or which can now safely be removed
//		//RemoveStalePredictedEntities(nSlot, incoming_acknowledged);
//
//		//csgo specific: removed in csgo ^
//
//
//		// Restore objects back to "pristine" state from last network/world state update
//		if (received_new_world_update)
//		{
//			RestoreOriginalEntityState(nSlot);
//			if (localPlayer->IsLocalPlayer()) //dylan added
//			{
//				auto& state = LocalPlayer.real_playerbackups[incoming_acknowledged % 150];
//				if (!state.ReceivedGameServerAck)
//				{
//					state.TickBase = TIME_TO_TICKS(localPlayer->GetSimulationTime());
//					state.SimulationTime = localPlayer->GetSimulationTime();
//					Vector neworg = localPlayer->GetNetworkOrigin();
//					state.AbsOrigin = neworg;
//					state.NetworkOrigin = localPlayer->GetNetworkOrigin();
//					state.ReceivedGameServerAck = true;
//					state.ServerTickWhenAcked = g_ClientState->command_ack_servertickcount;
//				}
//			}
//		}
//
//		m_bInPrediction = true;
//		bool bValid = PerformPrediction(nSlot, localPlayer, received_new_world_update, incoming_acknowledged, outgoing_command);
//		m_bInPrediction = false;
//		if (!bValid)
//		{
//			return;
//		}
//	}
//
//	// Overwrite predicted angles with the actual view angles
//	localPlayer->SetLocalAngles(viewangles);
//
//	// This allows us to sample the world when it may not be ready to be sampled
//	Assert(C_BaseEntity::IsAbsQueriesValid());
//}
//
//void CPrediction::RestoreOriginalEntityState(int nSlot)
//{
//	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CPrediction*, int)>(_RestoreOriginalEntityState)(this, nSlot);
//	CBaseEntity* Entity = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer());
//	Entity->SetVelocityModifier(Entity->ToPlayerRecord()->m_flPristineVelocityModifier);
//}
//
//void CPrediction::RunSimulation(int current_command, float curtime, CUserCmd* cmd, C_BasePlayer* localPlayer)
//{
//	__asm movss xmm2, curtime
//	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CPrediction*, int, CUserCmd*, C_BaseEntity*)>(_RunSimulation)(this, current_command, cmd, (C_BaseEntity*)localPlayer);
//}
//
//void CPrediction::StorePredictionResults(int nSlot, int predicted_frame)
//{
//	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CPrediction*, int, int)>(_StorePredictionResults)(this, nSlot, predicted_frame);
//}
//
//void CPrediction::Untouch(int nSlot)
//{
//	int numpredictables = GetPredictables(nSlot)->GetPredictableCount();
//
//	// Loop through all entities again, checking their untouch if flagged to do so
//	int i;
//	for (i = 0; i < numpredictables; i++)
//	{
//		C_BaseEntity* entity = GetPredictables(nSlot)->GetPredictable(i);
//		if (!entity)
//			continue;
//
//		if (!entity->GetCheckUntouch())
//			continue;
//
//		entity->PhysicsCheckForEntityUntouch();
//	}
//}
//
//int CPrediction::ComputeFirstCommandToExecute(int nSlot, bool received_new_world_update, int incoming_acknowledged, int outgoing_command)
//{
//	int destination_slot = 1;
//	int skipahead = 0;
//
//	Split_t& split = m_Split[nSlot];
//
//	// If we didn't receive a new update ( or we received an update that didn't ack any new CUserCmds -- 
//	//  so for the player it should be just like receiving no update ), just jump right up to the very 
//	//  last command we created for this very frame since we probably wouldn't have had any errors without 
//	//  being notified by the server of such a case.
//	// NOTE:  received_new_world_update only gets set to false if cl_pred_optimize >= 1
//	if (!received_new_world_update || !split.m_nServerCommandsAcknowledged)
//	{
//		if (!split.m_bUnknown2) //csgo specific
//		{
//			// this is where we would normally start
//			int start = incoming_acknowledged + 1;
//			// outgoing_command is where we really want to start
//			skipahead = std::max(0, (outgoing_command - start));
//			// Don't start past the last predicted command, though, or we'll get prediction errors
//			skipahead = std::min(skipahead, split.m_nCommandsPredicted);
//
//			// Always restore since otherwise we might start prediction using an "interpolated" value instead of a purely predicted value
//			RestoreEntityToPredictedFrame(nSlot, skipahead - 1);
//
//			//Msg( "%i/%i no world, skip to %i restore from slot %i\n", 
//			//	gpGlobals->framecount,
//			//	gpGlobals->tickcount,
//			//	skipahead,
//			//	skipahead - 1 );
//		}
//	}
//	else
//	{
//		// Otherwise, there is a second optimization, wherein if we did receive an update, but no
//		//  values differed (or were outside their epsilon) and the server actually acknowledged running
//		//  one or more commands, then we can revert the entity to the predicted state from last frame, 
//		//  shift the # of commands worth of intermediate state off of front the intermediate state array, and
//		//  only predict the usercmd from the latest render frame.
//		if (g_Vars.cl_pred_optimize->GetInt() >= 2 &&
//			!split.m_bPreviousAckHadErrors &&
//			split.m_nCommandsPredicted > 0 &&
//			split.m_nServerCommandsAcknowledged <= split.m_nCommandsPredicted
//			&& !split.m_bUnknown2) //< csgo specific
//		{
//			// Copy all of the previously predicted data back into entity so we can skip repredicting it
//			// This is the final slot that we previously predicted
//			RestoreEntityToPredictedFrame(nSlot, split.m_nCommandsPredicted - 1);
//
//			// Shift intermediate state blocks down by # of commands ack'd
//			ShiftIntermediateDataForward(nSlot, split.m_nServerCommandsAcknowledged, split.m_nCommandsPredicted);
//
//			// Only predict new commands (note, this should be the same number that we could compute
//			//  above based on outgoing_command - incoming_acknowledged - 1
//			skipahead = (split.m_nCommandsPredicted - split.m_nServerCommandsAcknowledged);
//
//			//Msg( "%i/%i optimize2, skip to %i restore from slot %i\n", 
//			//	gpGlobals->framecount,
//			//	gpGlobals->tickcount,
//			//	skipahead,
//			//	split.m_nCommandsPredicted - 1 );
//		}
//		else
//		{
//			//csgo specific
//			if (split.m_bPreviousAckHadErrors)
//			{
//				CPredictableList* predictables = GetPredictables(nSlot);
//				for (int i = 0; i < predictables->GetPredictableCount(); ++i)
//				{
//					C_BaseEntity* predictable = predictables->GetPredictable(i);
//					if (predictable)
//					{
//						predictable->SetUnknownEntityPredictionBool(true);
//					}
//				}
//			}
//
//			if ((split.m_bPreviousAckHadErrors && g_Vars.cl_pred_doresetlatch->GetBool()) ||
//				g_Vars.cl_pred_doresetlatch->GetInt() == 2)
//			{
//				// Both players should have == time base, etc.
//				C_CSPlayer* pLocalPlayer = C_CSPlayer::GetLocalPlayer();
//
//				// If an entity gets a prediction error, then we want to clear out its interpolated variables
//				// so we don't mix different samples at the same timestamps. We subtract 1 tick interval here because
//				// if we don't, we'll have 3 interpolation entries with the same timestamp as this predicted
//				// frame, so we won't be able to interpolate (which leads to jerky movement in the player when
//				// ANY entity like your gun gets a prediction error).
//				float flPrev = Interfaces::m_pGlobalVars->curtime;
//				Interfaces::m_pGlobalVars->curtime = TICKS_TO_TIME(pLocalPlayer->GetTickBase()) - TICK_INTERVAL;
//
//				if (split.m_bUnknown || g_Vars.cl_pred_doresetlatch->GetInt() == 2) 	//csgo specific
//				{
//					CPredictableList* predictables = GetPredictables(nSlot);
//					for (int i = 0; i < predictables->GetPredictableCount(); i++)
//					{
//						C_BaseEntity* entity = predictables->GetPredictable(i);
//						if (entity)
//						{
//							entity->ResetLatched();
//						}
//					}
//
//					Interfaces::m_pGlobalVars->curtime = flPrev;
//				}
//				else //csgo specific
//				{
//					for (int i = 0; i < split.m_vecPredictionHandles.Count(); ++i)
//					{
//						CBaseEntity* pEnt = Interfaces::ClientEntList->GetBaseEntityFromHandle(split.m_vecPredictionHandles[i]);
//						if (pEnt)
//						{
//							CPredictableList* predictables = GetPredictables(nSlot);
//							for (int j = 0; j < predictables->GetPredictableCount(); j++)
//							{
//								C_BaseEntity* pPredictable = predictables->GetPredictable(j);
//								if (pPredictable == pEnt)
//								{
//									pPredictable->ResetLatched();
//									break;
//								}
//							}
//						}
//					}
//
//					Interfaces::m_pGlobalVars->curtime = flPrev;
//				}
//			}
//		}
//	}
//
//	destination_slot += skipahead;
//
//	//csgo specific
//	ShiftFirstPredictedIntermediateDataForward(nSlot, split.m_nServerCommandsAcknowledged);
//
//	// Always reset these values now that we handled them
//	split.m_nCommandsPredicted = 0;
//	split.m_bPreviousAckHadErrors = false;
//	split.m_nServerCommandsAcknowledged = 0;
//	split.m_bUnknown2 = 0;
//
//	return destination_slot;
//}
//
//void CPrediction::ShiftFirstPredictedIntermediateDataForward(int nSlot, int acknowledged)
//{
//	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CPrediction*, int, int)>(_ShiftFirstPredictedIntermediateDataForward)(this, nSlot, acknowledged);
//}
//
//void CPrediction::RestoreEntityToPredictedFrame(int nSlot, int predicted_frame)
//{
//	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CPrediction*, int, int)>(_RestoreEntityToPredictedFrame)(this, nSlot, predicted_frame);
//}
//
//void CPrediction::ShiftIntermediateDataForward(int nSlot, int slots_to_remove, int previous_last_slot)
//{
//	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CPrediction*, int, int, int)>(_ShiftIntermediateDataForward)(this, nSlot, slots_to_remove, previous_last_slot);
//}