#pragma once

class IPrediction
{
public:
	void Update( int startframe, bool validframe, int incoming_acknowledged, int outgoing_command );
	void SetupMove( C_BasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move );
	void FinishMove( C_BasePlayer* player, CUserCmd* ucmd, CMoveData* move );
	void SetLocalViewAngles( const QAngle& ang );
	int CheckMovingGround( C_BasePlayer * player, double unk );
	int& m_nCommandsPredicted( );
	bool InPrediction( );
	CGlobalVarsBase* GetUnpredictedGlobals( );
  
public:
	float GetIdealPitch(int nSlot) const
	{
		if (nSlot == -1) { Assert(0); return 0.0f; }
		return m_Split[nSlot].m_flIdealPitch;
	}

	bool			m_bInPrediction;
	bool			m_bOldCLPredictValue;
	bool			m_bEnginePaused;

	int				m_nPreviousStartFrame;
	int				m_nIncomingPacketNumber;

	float			m_flLastServerWorldTimeStamp;

	// last network origin for local player
	struct Split_t
	{
		Split_t()
		{
			m_bFirstTimePredicted = false;
			m_nCommandsPredicted = 0;
			m_nServerCommandsAcknowledged = 0;
			m_bPreviousAckHadErrors = false;
			m_flIdealPitch = 0.0f;
		}

		bool			m_bFirstTimePredicted;
		int				m_nCommandsPredicted;
		int				m_nServerCommandsAcknowledged;
		int				m_bPreviousAckHadErrors;
		float			m_flIdealPitch;
	};

	Split_t				m_Split[2];
};

class CMoveData;
class IMoveHelper;
class CPDumpPanel;
struct typedescription_t;

//#pragma pack(push, 1)
//#pragma pack(pop)

//-----------------------------------------------------------------------------
// Purpose: Implements prediction in the client .dll
//-----------------------------------------------------------------------------
//class CPrediction : public IPrediction
//{
//	// Construction
//public:
//	//DECLARE_CLASS_GAMEROOT(CPrediction, IPrediction);
//
//	CPrediction(void);
//	virtual			~CPrediction(void);
//
//	virtual void	Init(void);
//	virtual void	Shutdown(void);
//
//	// Implement IPrediction
//public:
//
//	virtual void	Update
//	(
//		int startframe,		// World update ( un-modded ) most recently received
//		bool validframe,		// Is frame data valid
//		int incoming_acknowledged, // Last command acknowledged to have been run by server (un-modded)
//		int outgoing_command	// Last command (most recent) sent to server (un-modded)
//	);
//
//	virtual void	PreEntityPacketReceived(int commands_acknowledged, int current_world_update_packet);
//	virtual void	PostEntityPacketReceived(void);
//	virtual void	PostNetworkDataReceived(int commands_acknowledged);
//	virtual void	OnReceivedUncompressedPacket(void);
//	// The engine needs to be able to access a few predicted values
//	virtual void	GetViewOrigin(Vector& org);
//	virtual void	SetViewOrigin(Vector& org);
//	virtual void	GetViewAngles(QAngle& ang);
//	virtual void	SetViewAngles(QAngle& ang);
//
//	virtual void	GetLocalViewAngles(QAngle& ang);
//	virtual void	SetLocalViewAngles(QAngle& ang);
//
//	virtual bool	InPrediction(void) const;
//	virtual bool	IsFirstTimePredicted(void) const;
//	virtual int GetLastAcknowledgedCommandNumber(void)const;
//
//#if !defined( NO_ENTITY_PREDICTION )
//	virtual int		GetIncomingPacketNumber(void) const;
//#endif
//
//	float			GetIdealPitch(int nSlot) const
//	{
//		if (nSlot == -1)
//		{
//			return 0.0f;
//		}
//		return 0.f;// m_Split[nSlot].m_flIdealPitch;
//	}
//
//	virtual void CheckMovingGround(C_BasePlayer* player, double frametime);
//
//	virtual void	RunCommand(C_BasePlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper);
//
//	// Internal
//public: //dylan changed from protected
//	virtual void	SetupMove(C_BasePlayer* player, CUserCmd* ucmd, IMoveHelper* pHelper, CMoveData* move);
//	virtual void	FinishMove(C_BasePlayer* player, CUserCmd* ucmd, CMoveData* move);
//	virtual void	SetIdealPitch(int nSlot, C_BasePlayer* player, const Vector& origin, const QAngle& angles, const Vector& viewheight);
//
//	void			CheckError(int nSlot, C_BasePlayer* player, int commands_acknowledged);
//
//	// Called before and after any movement processing
//	void			StartCommand(C_BasePlayer* player, CUserCmd* cmd);
//	void			FinishCommand(C_BasePlayer* player);
//
//	// Helpers to call pre and post think for player, and to call think if a think function is set
//	void			RunPreThink(C_BasePlayer* player);
//	void			RunThink(C_BasePlayer* ent, double frametime);
//	void			RunPostThink(C_BasePlayer* player);
//
//	void	_Update_New
//	(
//		int nSlot,
//		bool received_new_world_update,
//		bool validframe,		// Is frame data valid
//		int incoming_acknowledged, // Last command acknowledged to have been run by server (un-modded)
//		int outgoing_command	// Last command (most recent) sent to server (un-modded)
//	);
//
//private:
//	virtual void	_Update
//	(
//		int nSlot,
//		bool received_new_world_update,
//		bool validframe,		// Is frame data valid
//		int incoming_acknowledged, // Last command acknowledged to have been run by server (un-modded)
//		int outgoing_command	// Last command (most recent) sent to server (un-modded)
//	);
//
//	// Actually does the prediction work, returns false if an error occurred
//	bool			PerformPrediction(int nSlot, C_BasePlayer* localPlayer, bool received_new_world_update, int incoming_acknowledged, int outgoing_command);
//
//	void			ShiftIntermediateDataForward(int nSlot, int slots_to_remove, int previous_last_slot);
//	void			ShiftFirstPredictedIntermediateDataForward(int nSlot, int acknowledged);
//	void			RestoreEntityToPredictedFrame(int nSlot, int predicted_frame);
//	int				ComputeFirstCommandToExecute(int nSlot, bool received_new_world_update, int incoming_acknowledged, int outgoing_command);
//	int				ComputeFirstCommandToExecute_New(C_BaseEntity* pPlayer, int nSlot, bool received_new_world_update, int incoming_acknowledged, int outgoing_command);
//
//	void			DumpEntity(C_BaseEntity* ent, int commands_acknowledged);
//
//	void			ShutdownPredictables(void);
//	void			ReinitPredictables(void);
//
//	void			RemoveStalePredictedEntities(int nSlot, int last_command_packet);
//	void			RestoreOriginalEntityState(int nSlot);
//	void			RunSimulation(int current_command, float curtime, CUserCmd* cmd, C_BasePlayer* LocalPlayer);
//	void			Untouch(int nSlot);
//	void			StorePredictionResults(int nSlot, int predicted_frame);
//	bool			ShouldDumpEntity(C_BaseEntity* ent);
//
//	void			SmoothViewOnMovingPlatform(C_BasePlayer* pPlayer, Vector& offset);
//
//#if !defined( NO_ENTITY_PREDICTION )
//	// Data
//public:
//	// Last object the player was standing on
//	//CHandle2< C_BaseEntity > m_hLastGround; //0x4
//	bool m_bInPrediction; //0x8
//	bool m_bFirstTimePredicted; //0x9
//	bool m_bEnginePaused; //0x0A
//	bool m_bOldCLPredictValue; //0x0B
//	int m_nPreviousStartFrame; //0x0C
//	int m_nIncomingPacketNumber; //0x10
//	float m_flLastServerWorldTimeStamp; //0x14
//	// Last network origin for local player
//
////#pragma pack(push, 1)
////	struct Split_t
////	{
////		Split_t() //note: incomplete
////		{
////			m_bFirstTimePredicted = false;
////			m_nCommandsPredicted = 0;
////			m_nServerCommandsAcknowledged = 0;
////			m_bPreviousAckHadErrors = false;
////			//m_flIdealPitch = 0.0f;
////
////		}
////
////		bool			m_bFirstTimePredicted; //0x18
////		char pad[3];
////		int				m_nCommandsPredicted; //0x1C
////		int				m_nServerCommandsAcknowledged; //0x20  set in PostNetworkDataReceived
////		int				m_bPreviousAckHadErrors; //0x24 set in PostNetworkDataReceived
////		int pad2; //0x28
////		int m_nIncomingAcknowledged; //0x2C
////		char m_bUnknown; //0x30 set in PostNetworkDataReceived
////		char pad3[3]; //0x31
////
////		CUtlVector<EHANDLE> m_vecPredictionHandles; //0x34
////		bool m_bUnknown2;
////		char pad4[3];
////		//char pad4[12]; //0x34
////		//int pad5; //0x40 set in PostNetworkDataReceived
////		//int pad6[2]; //0x44
////
////		//end 0x4C
////	};//size 0x34 bytes
////#pragma pack(pop)
////
////	Split_t				m_Split[1]; //0x18 //MAX_SPLITSCREEN_PLAYERS
//	CGlobalVars     m_SavedVars; //0x4C
//	bool			m_bPlayerOriginTypedescriptionSearched;
//	//CUtlVector< const typedescription_t* >	m_PlayerOriginTypeDescription; // A vector in cases where the .x, .y, and .z are separately listed
//
//	CPDumpPanel* m_pPDumpPanel;
//
//#endif
//};