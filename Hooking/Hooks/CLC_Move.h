#pragma once
#include "../../SDK/Classes/Exploits.hpp"

extern bool DisableCLC_Move;

#pragma pack(push, 1)
struct CLC_Move
{
	unsigned int INetMessage_vtable; //0x58
	unsigned int CCLCMsg_Move_vtable; //0x54
	unsigned int unknown; //0x50
	int	m_nBackupCommands; //0x4C
	int	m_nNewCommands; //0x48
	void* allocatedmemory; //0x44
	unsigned int someint3; //0x40
	unsigned int flags;  //0x3C
	unsigned char somebyte1; //0x38
	unsigned char somebyte1_pad[3]; //0x37
	unsigned char somebyte2; //0x34
	unsigned char somebyte2_pad[3]; //0x33
	unsigned int unknownpad[3]; //0x30
	unsigned int someint2; //0x24
	unsigned int someint; //0x20
	bf_write m_DataOut; //0x1C
	void* m_Data; // 0x10

	CLC_Move();
	~CLC_Move();
	void FinishProtobuf();
};
#pragma pack(pop)

template < typename T >
class CNetMessagePB : public INetMessage, public T {};
using CCLCMsg_Move_t = CNetMessagePB< CLC_Move >;