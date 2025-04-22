#include "../sdk.hpp"
#include "Player.hpp"
#include "weapon.hpp"

CRC32_t CUserCmd::GetChecksum( void ) {
  CRC32_t crc;
  CRC32_Init( &crc );

  CRC32_ProcessBuffer( &crc, &command_number, sizeof( command_number ) );
  CRC32_ProcessBuffer( &crc, &tick_count, sizeof( tick_count ) );
  CRC32_ProcessBuffer( &crc, &viewangles, sizeof( viewangles ) );
  CRC32_ProcessBuffer( &crc, &aimdirection, sizeof( aimdirection ) );
  CRC32_ProcessBuffer( &crc, &forwardmove, sizeof( forwardmove ) );
  CRC32_ProcessBuffer( &crc, &sidemove, sizeof( sidemove ) );
  CRC32_ProcessBuffer( &crc, &upmove, sizeof( upmove ) );
  CRC32_ProcessBuffer( &crc, &buttons, sizeof( buttons ) );
  CRC32_ProcessBuffer( &crc, &impulse, sizeof( impulse ) );
  CRC32_ProcessBuffer( &crc, &weaponselect, sizeof( weaponselect ) );
  CRC32_ProcessBuffer( &crc, &weaponsubtype, sizeof( weaponsubtype ) );
  CRC32_ProcessBuffer( &crc, &random_seed, sizeof( random_seed ) );
  CRC32_ProcessBuffer( &crc, &mousedx, sizeof( mousedx ) );
  CRC32_ProcessBuffer( &crc, &mousedy, sizeof( mousedy ) );

  CRC32_Final( &crc );
  return crc;
}

void ModifiableUserCmd::Reset(CUserCmd* newcmd)
{
	auto local = C_CSPlayer::GetLocalPlayer();
	if (!local)
		return;

	auto weapon = (C_WeaponCSBaseGun*)local->m_hActiveWeapon().Get();
	if (!weapon)
		return;

	cmd = newcmd;
	PressingJumpButton = IsJumping();
	bManuallyFiring = weapon && (cmd_backup.buttons & IN_ATTACK || (cmd_backup.buttons & IN_ATTACK2 && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER));
	bFinalTick = *(bool*)(*FramePointer - 0x1B);
	pbSendPacket = (bool*)(*FramePointer - 0x1C);
	bOriginalSendPacket = *pbSendPacket;
	bSendPacket = *pbSendPacket;
}

bool ModifiableUserCmd::IsUserCmdAndPlayerNotMoving()
{
	auto local = C_CSPlayer::GetLocalPlayer();

	if (cmd->buttons & IN_USE)
		int i = 0;

	return !local || !g_Vars.globals.HackIsReady || (cmd->weaponselect == 0 && cmd->forwardmove == 0.0f && cmd->sidemove == 0.0f && (cmd->buttons & ~(IN_DUCK | IN_BULLRUSH)) == 0 && local->m_vecVelocity().Length() < 0.1f);
}