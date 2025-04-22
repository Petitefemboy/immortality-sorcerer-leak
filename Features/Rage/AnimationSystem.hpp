#pragma once
#include "../../SDK/sdk.hpp"

#include "../../SDK/Classes/Weapon.hpp"
#include "../../SDK/Classes/Player.hpp"

#include <map>
#include <deque>

extern bool m_bShouldDelayShot;

namespace Engine
{
	class C_SideAnimation {
	public:
		alignas( 16 ) matrix3x4_t m_Bones[ 128 ]{ };
		float m_flAbsRotation;

		void Update( C_CSPlayer* player ) {
			if( !player )
				return;

			auto animState = player->m_PlayerAnimState( );
			if( animState )
				m_flAbsRotation = animState->m_abs_yaw;
		}
	};

	class C_SimulationInfo {
	public:
		bool bOnGround = false;
		float m_flTime = 0.f;
		float m_flDuckAmount = 0.f;
		float m_flLowerBodyYawTarget = 0.f;
		Vector m_vecOrigin{ };
		Vector m_vecVelocity{ };
		QAngle m_angEyeAngles{ };
	};

	class C_AnimationRecord {
	public:
		bool m_bIsInvalid;
		bool m_bIsShoting;
		bool m_bTeleporting;
		bool m_bShiftingTickbase;
		bool m_bResolved;
		bool m_bIsDormant;
		bool m_bSkipDueToResolver;
		bool m_bNoFakeAngles;
		bool m_bFakeWalking;
		bool m_bUnsafeVelocityTransition;
		bool m_bUnsafeLBY;

		Vector m_vecOrigin;
		Vector m_vecVelocity;
		Vector m_vecAnimationVelocity;
		QAngle m_angEyeAngles, m_angDirWhenFirstStartedMoving;

		int m_fFlags;
		int m_iChokeTicks;

		float m_flChokeTime;
		float m_flSimulationTime;
		float m_flShotTime;
		float m_flDuckAmount;
		float m_flDelta;
		float m_flLowerBodyYawTarget, m_flOldLowerBodyYaw;
		float m_flAbsRotation;
		float m_flAnimationSpeed;

		float m_flFeetYawRate;
		float m_flFeetCycle;

		int m_iResolverMode;
		int m_iStandingResolverMode = -4;
		std::string m_sResolverStrings;

		float m_flPoseParameters[20];
		CCSGOPlayerAnimState* m_serverPlayerAnimState;
		C_AnimationLayer m_serverAnimOverlays[ 13 ];
	};

	class C_AnimationData {
	public:
		void Update( );
		void Collect( C_CSPlayer* player );

		void SimulateAnimations( Encrypted_t<Engine::C_AnimationRecord> current, Encrypted_t<Engine::C_AnimationRecord> previous );

		C_CSPlayer* player;
		int ent_index;

		float m_flSpawnTime;
		float m_flSimulationTime;
		float m_flOldSimulationTime;

		Vector m_vecOrigin;

		bool m_bIsDormant = false;
		bool m_bBonesCalculated = false;
		bool m_bUpdated = false;
		bool m_bForceVelocity = false;
		bool m_bDidForceVelocity = false;

		bool m_bResolved = false;
		bool m_bIsAlive = false;
		bool m_bInvertedSide = false;
		bool m_bForceFake = false;
		float m_flLastScannedYaw = 0.0f;

		int m_iCurrentTickCount = 0;
		int m_iOldTickCount = 0;
		int m_iTicksAfterDormancy = 0;
		int m_iTicksUnknown = 0;
		int m_iLastLowDeltaTick = 0;

		Vector m_vecOldVelocity;

		std::deque<C_AnimationRecord> m_AnimationRecord;
		std::vector<C_SimulationInfo> m_vecSimulationData;

		alignas( 16 ) matrix3x4_t m_Bones[ 128 ]{ };
		float m_flAbsRotation;
	};

	class __declspec( novtable ) AnimationSystem : public NonCopyable {
	public:
		static Encrypted_t<AnimationSystem> Get( );

		virtual void CollectData( ) = 0;
		virtual void Update( ) = 0;
		virtual C_AnimationData* GetAnimationData( int index ) = 0;

	protected:
		AnimationSystem( ) { };
		virtual ~AnimationSystem( ) { };
	};
}
