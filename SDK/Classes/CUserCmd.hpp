#pragma once

class CUserCmd
{
public:
  CRC32_t GetChecksum( void );

  char pad_0x0000[0x4]; //0x0000	
  int     command_number;     // 0x04 For matching server and client commands for debugging
  int     tick_count;         // 0x08 the tick the client created this command
  QAngle  viewangles;         // 0x0C Player instantaneous view angles.
  Vector  aimdirection;       // 0x18
  float   forwardmove;        // 0x24
  float   sidemove;           // 0x28
  float   upmove;             // 0x2C
  int     buttons;            // 0x30 Attack button states
  char    impulse;            // 0x34
  int     weaponselect;       // 0x38 Current weapon id
  int     weaponsubtype;      // 0x3C
  int     random_seed;        // 0x40 For shared random functions
  short   mousedx;            // 0x44 mouse accum in x from create move
  short   mousedy;            // 0x46 mouse accum in y from create move
  bool    hasbeenpredicted;   // 0x48 Client only, tracks whether we've predicted this command at least once
  char    pad_0x4C[0x18];     // 0x4C Current sizeof( usercmd ) =  100  = 0x64
};

class CVerifiedUserCmd 
{
public:
	CUserCmd m_cmd = { };
	CRC32_t m_crc = 0u;
};

class ModifiableUserCmd
{
public:
	CUserCmd  cmd_backup; // original command before modification
	CUserCmd* cmd;
	DWORD* FramePointer;
	bool bFinalTick;
	bool bOriginalSendPacket;
	bool bSendPacket; // should we send a command this tick
	bool bShouldAutoScope;
	bool bShouldAutoStop;
private:
	bool* pbSendPacket;
public:
	int PressingJumpButton;
	bool bManuallyFiring;

	ModifiableUserCmd() : cmd_backup(), cmd(nullptr), FramePointer(nullptr), bFinalTick(false),
		bOriginalSendPacket(false),
		bSendPacket(false),
		bShouldAutoScope(false),
		bShouldAutoStop(false),
		pbSendPacket(nullptr),
		PressingJumpButton(0),
		bManuallyFiring(false)
	{
	}

	explicit ModifiableUserCmd(CUserCmd* newcmd) : cmd_backup(), cmd(newcmd), FramePointer(nullptr),
		bFinalTick(false),
		bOriginalSendPacket(false),
		bSendPacket(false),
		bShouldAutoScope(false),
		bShouldAutoStop(false),
		pbSendPacket(nullptr),
		PressingJumpButton(0),
		bManuallyFiring(false)
	{
	}

	~ModifiableUserCmd() {}

	void Reset(CUserCmd* newcmd);
	bool IsUserCmdAndPlayerNotMoving();

	BOOL IsManuallyAttacking() const { return cmd_backup.buttons & IN_ATTACK; }
	BOOL IsManuallySecondaryAttacking() const { return cmd_backup.buttons & IN_ATTACK2; }
	BOOL IsAttacking() const { return cmd->buttons & IN_ATTACK; }
	BOOL IsSecondaryAttacking() const { return cmd->buttons & IN_ATTACK2; }
	BOOL IsUsing() const { return cmd->buttons & IN_USE; }
	BOOL IsJumping() const { return cmd->buttons & IN_JUMP; }
	BOOL IsDucking() const { return cmd->buttons & IN_DUCK; }
	void OverrideSendPacket() { *pbSendPacket = bSendPacket; }
};
extern ModifiableUserCmd CurrentUserCmd;