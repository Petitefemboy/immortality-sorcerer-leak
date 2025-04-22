#pragma once
#include "../../SDK/sdk.hpp"

#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Classes/Player.hpp"

#include <map>
#include <deque>
#include "AnimationSystem.hpp"

#define RESOLVER_SIDE_FAKE 0
#define RESOLVER_SIDE_LEFT 1
#define RESOLVER_SIDE_RIGHT -1

#define ANIMATION_RESOLVER_FAKE 1
#define ANIMATION_RESOLVER_LEFT 2
#define ANIMATION_RESOLVER_RIGHT 4

namespace Engine
{
	// base lag record, generally used for backup and restore
	class C_BaseLagRecord {
	public:
		C_CSPlayer* player = nullptr;
		Vector m_vecMins, m_vecMaxs;
		Vector m_vecOrigin;
		QAngle m_angAngles;
		float m_flAbsRotation;

		float m_flSimulationTime;

		alignas(16) matrix3x4_t m_BoneMatrix[128];

		void Setup(C_CSPlayer* player);
		void Apply(C_CSPlayer* player);
	};

	// full lag record, will be stored in lag compensation history
	class C_LagRecord : public C_BaseLagRecord {
	public:
		float m_flServerLatency;
		float m_flRealTime;
		float m_flLastShotTime;
		float m_flEyeYaw;
		float m_flEyePitch;
		float m_flAnimationVelocity;
		float m_flLowerBodyYawTarget;

		// this is later used in our animation fix & tying our recorded data into that.
		bool m_bIsValid;
		bool m_bCreatedNewRecord;
		bool m_bCreatedNewRecordUsingPreviousData;
		bool m_bShouldOverrideAnimationData;

		QAngle m_angEyeAnglesNotFiring;

		bool m_bIsShoting;
		bool m_bBonesCalculated;
		bool m_bExtrapolated;
		bool m_bResolved;
		bool m_bIsWalking;
		bool m_bIsRunning;
		bool m_bIsSideways;

		float m_flAbsRotation;

		int m_iFlags;
		int m_iLaggedTicks = 0;
		int m_iResolverMode;
		int m_iStandingResolverMode;
		int m_iServerTick;
		int m_iTickCount;

		float m_flInterpolateTime = 0.f;

		// for sorting
		Vector m_vecVelocity;
		Vector m_vecAbsVelocity;
		float m_flDuckAmount;

		bool m_bSkipDueToResolver = false; // skip record in hitscan
		bool m_bTeleporting = false; // teleport distance was broken
		
		// new record shit
		bool m_bIsDuplicateTickbase;
		bool m_bTickbaseShiftedBackwards;
		bool m_bTickbaseShiftedForwards;

		int m_iTickBase;

		/*CCSGOPlayerAnimState* m_pPlayerAnimStateServer;*/

		C_AnimationLayer m_serverAnimationLayers[13];

		float GetAbsYaw();
		matrix3x4_t* GetBoneMatrix();
		void Setup(C_CSPlayer* player);
		void Apply( C_CSPlayer* player);
	};

	class C_EntityLagData {
	public:
		C_EntityLagData();
		static void UpdateRecordData(Encrypted_t< C_EntityLagData > pThis, C_LagRecord* previousRecord, C_CSPlayer* player, const player_info_t& info, int updateType);

		void Clear();

		// TODO; proxy everything in here so we are using the most pristine data!
		struct sPPDUData {
			float m_flPrePoseParams[20];

			C_AnimationLayer m_PreAnimLayers[13];
			C_AnimationLayer m_PostAnimLayers[13];
			bool m_bLayersDiffersFromRecord = false;

			float  m_flPreSimulationTime;
			bool m_bSimulationTimeDiffersFromRecord = false;

			Vector m_vecPreNetOrigin;
			Vector m_vecPostNetOrigin;
			bool m_bOriginDiffersFromRecord = false;

			QAngle m_angPreAbsAngles;
			QAngle m_angPostAbsAngles;
			bool m_bAbsAnglesDiffersFromRecord = false;

			QAngle m_angPreEyeAngles;
			QAngle m_angPostEyeAngles;
			bool m_bEyeAnglesDiffersFromRecord = false;

			float  m_flPreVelocityModifier;
			float  m_flPostVelocityModifier;
			bool m_bVelocityModifierDiffersFromRecord = false;

			float m_flPreLowerBodyYaw;
			float m_flPostLowerBodyYaw;
			float m_flLowerBodyUpdateTime;
			float m_flRecordedLBYUpdateTime;
			bool m_bLowerBodyYawDiffersFromRecord = false;

			float  m_flPreShotTime;
			float  m_flPostShotTime;
			bool m_bShotTimeDiffersFromRecord = false;

			//C_AnimationLayer m_LastOutputAnimLayers[13];

			bool m_bShouldUseServerData = false;

			void ResetData()
			{
				m_bLayersDiffersFromRecord = m_bSimulationTimeDiffersFromRecord = m_bOriginDiffersFromRecord = 
				m_bAbsAnglesDiffersFromRecord = m_bEyeAnglesDiffersFromRecord = m_bVelocityModifierDiffersFromRecord = 
				m_bLowerBodyYawDiffersFromRecord = m_bShotTimeDiffersFromRecord = false;
			}
		} m_sPPDUData;

		bool m_bShouldNetUpdate = false, m_bNetUpdateWasSilent = false;

		struct sProxyData {
			float m_flSimulationTime;
			bool m_bRecievedSimTime = false;

			float m_flEyeYawAngle;
			bool m_bRecievedYawAngle = false;

			// will be later used for flick prediction.
			float m_flLowerBodyYaw;
			int m_iTickRecievedLBYUpdate;
			bool m_bLBYUpdated = false, m_bRecievedLBY = false;
		} m_sProxyData;

		std::deque<Engine::C_LagRecord> m_History = {};
		int m_iUserID = -1;

		float m_flLastUpdateTime = 0.0f;
		float m_flRate = 0.0f;

		int m_iMissedShots = 0;
		int m_iMissedShotsLBY = 0;

		bool m_bHitLastMove = false;
		std::deque< float > m_flLastMoveYaw;

		bool m_bGotAbsYaw = false;
		bool m_bGotAbsYawShot = false;
		bool m_bNotResolveIfShooting = false;
		bool m_bRateCheck = false;
		float m_flAbsYawHandled = 0.f;

		// ragebot scan data
		float m_flLastSpread, m_flLastInaccuracy;
		float m_flLastScanTime;
		Vector m_vecLastScanPos;

		// autowall resolver stuff
		float m_flEdges[4];
		float m_flDirection;

		// new record shit
		bool m_bIsDuplicateTickbase = false;
		bool m_bTickbaseShiftedBackwards = false;
		bool m_bTickbaseShiftedForwards = false;

		int m_iTicksChoked{};
		int m_iTickBase{};
		int m_iNewestTickBase{};
		int m_iServerTickCount{};
		int m_iTicksSinceLastServerUpdate{};
		int m_iValidRecordCount{};
		int m_iRecordCount{};

		float m_flTotalInvalidRecordTime{};
		float m_flFirstCmdTickBaseTime{};
		float m_flNewestSimulationTime{};

		std::deque<int> m_iTicksChokedHistory;

		// new lby prediction shit
		float m_flLowerBodyYaw{};
		float m_flOldLowerBodyYaw{};
		float m_flLastLBYUpdateTime{};
		float m_flNextLBYUpdateTime{};
		float m_flTimeSinceLastBalanceAdjust{};
		float m_flLastBalanceAdjustYaw{};
		float m_flLastBalanceAdjust{};

		bool m_bSafeToRunLBYPrediction = false;
		bool m_bDidLBYChange = false;
		bool m_bPredictedLBY = false;
		bool m_bFlickedToLBY = false;
		bool m_bTriggeredBalanceAdjust = false;
		bool m_bDidLBYBreakWithAHighDelta = false;

		bool m_bUpdatedRecordData = false;

		// player prediction, need many improvments
		static bool DetectAutoDirerction( Encrypted_t< C_EntityLagData > pThis, C_CSPlayer* player );
	};

	class __declspec(novtable) LagCompensation : public NonCopyable {
	public:
		static LagCompensation* Get();

		virtual void Update() = 0;
		virtual bool IsRecordOutOfBounds(const Engine::C_LagRecord& record, float target_time = 0.2f, int tickbase_shift = -1, bool tick_count_check = true) const = 0;;
		virtual float GetLerp() const = 0;

		virtual Encrypted_t<C_EntityLagData> GetLagData(int entindex) = 0;

		virtual void ClearLagData() = 0;

		float m_flOutLatency;
		float m_flServerLatency;
	protected:
		LagCompensation() { };
		virtual ~LagCompensation() { };
	};
}
