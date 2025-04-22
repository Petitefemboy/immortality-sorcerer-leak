#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"

#include "../../SDK/sdk.hpp"
#include "../../SDK/Classes/Player.hpp"

//void WriteUsercmdD( void* buf, CUserCmd* incmd, CUserCmd* outcmd ) {
//	__asm
//	{
//		mov     ecx, buf
//		mov     edx, incmd
//		push    outcmd
//		call    Engine::Displacement.Function.m_WriteUsercmd
//		add     esp, 4
//	}
//}
//
//bool __fastcall Hooked::WriteUsercmdDeltaToBuffer( void* ecx, void* edx, int slot, void* buf, int from, int to, bool isnewcommand ) {
//	if (Interfaces::m_pEngine->IsInGame() && Interfaces::m_pEngine->IsConnected()) {
//		auto local = C_CSPlayer::GetLocalPlayer();
//		if (local && !local->IsDead()) {
//			if (g_Vars.globals.m_iTicksShifted <= 0) {
//				g_Vars.globals.m_iTicksShifted = 0;
//				return oWriteUsercmdDeltaToBuffer(ecx, slot, buf, from, to, isnewcommand);
//			}
//
//			if (from != -1)
//				return true;
//
//			int m_nTickbase = g_Vars.globals.m_iTicksShifted;
//			g_Vars.globals.m_iTicksShifted = 0;
//
//			*(int*)((uintptr_t)buf - 0x30) = 0;
//			int* m_pnNewCmds = (int*)((uintptr_t)buf - 0x2C);
//
//			int m_nNewCmds = *m_pnNewCmds;
//			int m_nNextCmd = Interfaces::m_pClientState->m_nLastOutgoingCommand() + Interfaces::m_pClientState->m_nChokedCommands() + 1;
//			int m_nTotalNewCmds = std::min(m_nNewCmds + abs(m_nTickbase), 62);
//
//			*m_pnNewCmds = m_nTotalNewCmds;
//
//			for (to = m_nNextCmd - m_nNewCmds + 1; to <= m_nNextCmd; to++) {
//				if (!oWriteUsercmdDeltaToBuffer(ecx, slot, buf, from, to, true))
//					return false;
//
//				from = to;
//			}
//
//			CUserCmd* m_pCmd = Interfaces::m_pInput->GetUserCmd(slot, from);
//			if (!m_pCmd)
//				return true;
//
//			CUserCmd m_FromCmd = *m_pCmd, m_ToCmd = *m_pCmd;
//			m_ToCmd.command_number++;
//			m_ToCmd.tick_count += 3 * (int)std::round(1.f / Interfaces::m_pGlobalVars->interval_per_tick);
//
//			for (int i = m_nNewCmds; i <= m_nTotalNewCmds; i++) {
//				WriteUsercmdD(buf, &m_ToCmd, &m_FromCmd);
//				m_FromCmd = m_ToCmd;
//
//				m_ToCmd.command_number++;
//				m_ToCmd.tick_count++;
//			}
//
//			return true;
//		}
//	}
//}