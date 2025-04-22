#pragma once

#include "../../SDK/sdk.hpp"
#include <deque>

class C_CSPlayer;

namespace Engine {
	enum ENetvarsCompressed : size_t {
		m_aimPunchAngle = 0,
		m_aimPunchAngleVel,
		m_vecBaseVelocity,
		m_vecViewOffset,
		m_flVelocityModifier,
		m_flFallVelocity,
		m_flDuckAmount,
		m_flDuckSpeed,
		m_surfaceFriction,
		m_fAccuracyPenalty,
		
	};

	struct RestoreData {
		void Reset( ) {
			m_aimPunchAngle.Set( );
			m_aimPunchAngleVel.Set( );
			m_viewPunchAngle.Set( );

			m_vecViewOffset.Set( );
			m_vecBaseVelocity.Set( );
			m_vecVelocity.Set( );
			m_vecOrigin.Set( );

			m_flFallVelocity = 0.0f;
			m_flVelocityModifier = 0.0f;
			m_flDuckAmount = 0.0f;
			m_flDuckSpeed = 0.0f;
			m_surfaceFriction = 0.0f;

			m_fAccuracyPenalty = 0.0f;
			m_flRecoilIndex = 0.f;

			m_hGroundEntity = 0;
			m_nMoveType = 0;
			m_nFlags = 0;
			m_nTickBase = 0;
		}

		void Setup( C_CSPlayer* player );

		void Apply( C_CSPlayer* player );

		CMoveData m_MoveData;

		QAngle m_aimPunchAngle = { };
		QAngle m_aimPunchAngleVel = { };
		QAngle m_viewPunchAngle = { };

		Vector m_vecViewOffset = { };
		Vector m_vecBaseVelocity = { };
		Vector m_vecVelocity = { };
		Vector m_vecOrigin = { };
		Vector m_vecNetworkOrigin = {};

		float m_flFallVelocity = 0.0f;
		float m_flVelocityModifier = 0.0f;
		float m_flDuckAmount = 0.0f;
		float m_flDuckSpeed = 0.0f;
		float m_surfaceFriction = 0.0f;

		float m_fAccuracyPenalty = 0.0f;
		float m_flRecoilIndex = 0;

		int m_hGroundEntity = 0;
		int m_nMoveType = 0;
		int m_nFlags = 0;
		int m_nTickBase = 0;

		bool is_filled = false;
	};

	// the extremely ghetto way lmao
	struct PlayerData {
		int m_nTickbase = 0;
		int m_nCommandNumber = 0;
		
		QAngle m_aimPunchAngle = { }; // 0
		QAngle m_aimPunchAngleVel = { }; // 1
		
		Vector m_vecBaseVelocity = { }; // 2
		Vector m_vecViewOffset = { }; // 3

		float m_flVelocityModifier = 0.0f; // 4
		float m_flFallVelocity = 0.0f; // 5
		float m_flDuckAmount = 0.0f; // 6
		float m_flDuckSpeed = 0.0f; // 7
		float m_surfaceFriction = 0.0f; // 8
		// weapon values; there should be a LOT more.
		float m_fAccuracyPenalty = 0.0f; // 9
		float m_flRecoilIndex = 0.f; // 10

		QAngle m_viewPunchAngle = {}; // 11
		Vector m_vecVelocity = {}; // 12
		Vector m_vecNetworkOrigin = {}; // 13

		float m_flNextPrimaryAttack = 0.0f; // 14
		float m_flNextSecondaryAttack = 0.0f; // 15
		float m_flPostponeFireReadyTime = 0.0f; // 16

		Vector m_vecOrigin = {}; // 17
	};

	struct LastPredData {
		QAngle m_aimPunchAngle = { };
		QAngle m_aimPunchAngleVel = { };
		Vector m_vecBaseVelocity = { };
		Vector m_vecViewOffset = { };
		Vector m_vecOrigin = { };
		float m_flFallVelocity = 0.0f;
		float m_flDuckAmount = 0.0f;
		float m_flDuckSpeed = 0.0f;
		float m_flVelocityModifier = 0.0f;
		int m_nTickBase = 0;
	};

	struct CorrectionData {
		int command_nr;
		int tickbase;
		int tickbase_shift;
		int tickcount;
		int chokedcommands;
	};

	struct OutgoingData {
		int command_nr;
		int prev_command_nr;

		bool is_outgoing;
		bool is_used;
	};

	struct PredictionData {
		Encrypted_t<CUserCmd> m_pCmd = nullptr;
		bool* m_pSendPacket = nullptr;

		C_CSPlayer* m_pPlayer = nullptr;
		C_WeaponCSBaseGun* m_pWeapon = nullptr;
		Encrypted_t<CCSWeaponInfo> m_pWeaponInfo = nullptr;

		int m_nTickBase = 0;
		int m_fFlags = 0;
		Vector m_vecVelocity{ };

		bool m_bInPrediction = false;
		bool m_bFirstTimePrediction = false;

		float m_flCurrentTime = 0.0f;
		float m_flFrameTime = 0.0f;

		CMoveData m_MoveData = { };
		RestoreData m_RestoreData;

		PlayerData m_Data[ 150 ] = { };

		std::deque<CorrectionData> m_CorrectionData;
		std::vector<OutgoingData> m_OutgoingCommands;
		std::vector<int> m_ChokedNr;

		bool m_bInitDatamap = false;
		int m_Offset_nImpulse;
		int m_Offset_nButtons;
		int m_Offset_afButtonLast;
		int m_Offset_afButtonPressed;
		int m_Offset_afButtonReleased;
		int m_Offset_afButtonForced;
	};

	class Prediction : public Core::Singleton<Prediction> {
	public:
		void Begin( Encrypted_t<CUserCmd> cmd, bool* send_packet, int command_number);
		void Repredict( );
		void End( );
		void Invalidate( );
		void RunGamePrediction( );

		int GetFlags( );
		Vector GetVelocity( );

		Encrypted_t<CUserCmd> GetCmd( );

		float GetFrametime( );
		float GetCurtime( );
		float GetSpread( );
		float GetInaccuracy( );
		float GetWeaponRange();

		void ApplyCompressionFix();
		void OnFrameStageNotify( ClientFrameStage_t stage );
		void OnRunCommand( C_CSPlayer* player, CUserCmd* cmd );

		void PostEntityThink( C_CSPlayer* player );

		bool ShouldSimulate( int command_number );
		void PacketCorrection( uintptr_t cl_state );
		void KeepCommunication( bool* bSendPacket, int command_number );


		void StoreNetvarCompression(CUserCmd* cmd);
		void RestoreNetvarCompression(CUserCmd* cmd);

		bool InPrediction( ) {
			return m_bInPrediction;
		}

		Encrypted_t<PredictionData> predictionData;

		bool m_bGetNetvarCompressionData = false;

		// ghetto fucking way but hey it works
		bool m_bNetvarCompressionFixHasBeenApplied[18] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false }; // the last one is used to say that all values are good.
		bool m_bNetvarCompressionWasntApplied = false;
		std::vector<std::pair<int, bool>> m_aNetvarCompressionFixHasBeenApplied;

	private:
		Prediction( );
		~Prediction( );

		LastPredData m_LastPredictedData;
		CUserCmd* m_pLastCmd;

		friend class Core::Singleton<Prediction>;
		bool m_bInPrediction = false;

		int m_iSeqDiff = 0;

		float m_flSpread = 0.f;
		float m_flInaccuracy = 0.f;
		float m_flWeaponRange = 0.f;

		bool m_bFixSendDataGram = false;
		bool m_bNeedStoreNetvarsForFixingNetvarCompresion = false;
	};
}