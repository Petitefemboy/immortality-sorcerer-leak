#include "AntiAim.hpp"
#include "../../SDK/CVariables.hpp"
#include "../Miscellaneous/Movement.hpp"
#include "../../source.hpp"
#include "../../Utils/InputSys.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "LagCompensation.hpp"
#include "Autowall.h"
#include "../Game/SimulationContext.hpp"
#include "../../SDK/displacement.hpp"
#include "../../Renderer/Render.hpp"

#include "../Visuals/ESP.hpp"
#include <random>
#include "FakeLag.hpp"

int GetFPS()
{
	static int fps = 0;
	static int count = 0;
	using namespace std::chrono;
	auto now = high_resolution_clock::now();
	static auto last = high_resolution_clock::now();
	count++;

	if (duration_cast<milliseconds>(now - last).count() > 1000) {
		fps = count;
		count = 0;
		last = now;
	}

	return fps;
}

namespace Interfaces
{
	class C_AntiAimbot : public AntiAimbot {
	public:
		void Main(bool* bSendPacket, bool* bFinalPacket, Encrypted_t<CUserCmd> cmd, bool ragebot) override;
		void PrePrediction(bool* bSendPacket, Encrypted_t<CUserCmd> cmd) override;
		void ImposterBreaker(bool* bSendPacket, Encrypted_t<CUserCmd> cmd) override;

		float m_flLowerBodyUpdateYaw = FLT_MAX;
	private:
		virtual float GetAntiAimX(Encrypted_t<CVariables::ANTIAIM_STATE> settings);
		virtual float GetAntiAimY(Encrypted_t<CVariables::ANTIAIM_STATE> settings, Encrypted_t<CUserCmd> cmd);
		virtual void Distort(Encrypted_t<CUserCmd> cmd, bool fakeYaw);

		enum class Directions : int {
			YAW_RIGHT = -1,
			YAW_BACK,
			YAW_LEFT,
			YAW_NONE,
		};
		virtual Directions HandleDirection(Encrypted_t<CUserCmd> cmd);

		virtual bool IsEnabled(Encrypted_t<CUserCmd> cmd, Encrypted_t<CVariables::ANTIAIM_STATE> settings);

		bool m_bUsingManual = false;

		bool m_bNegate = false;
		float m_flLowerBodyUpdateTime = 0.f;

		float  m_auto;
		float  m_auto_dist;
		float  m_auto_last;
		float  m_view;
		float  m_auto_time;
	};

	bool C_AntiAimbot::IsEnabled(Encrypted_t<CUserCmd> cmd, Encrypted_t<CVariables::ANTIAIM_STATE> settings) {
		C_CSPlayer* LocalPlayer = C_CSPlayer::GetLocalPlayer();
		if (!LocalPlayer || LocalPlayer->IsDead())
			return false;

		//if (!(g_Vars.antiaim.bomb_activity && g_Vars.globals.BobmActivityIndex == LocalPlayer->EntIndex()) || !g_Vars.antiaim.bomb_activity)
		if ((cmd->buttons & IN_USE) || LocalPlayer->m_bIsDefusing())
			return false;

		if (LocalPlayer->m_MoveType() == MOVETYPE_NOCLIP)
			return false;

		static auto g_GameRules = *(uintptr_t**)(Engine::Displacement.Data.m_GameRules);
		if (g_GameRules && *(bool*)(*(uintptr_t*)g_GameRules + 0x20) || (LocalPlayer->m_fFlags() & FL_FROZEN))
			return false;

		C_WeaponCSBaseGun* Weapon = (C_WeaponCSBaseGun*)LocalPlayer->m_hActiveWeapon().Get();

		if (!Weapon)
			return false;

		auto WeaponInfo = Weapon->GetCSWeaponData();
		if (!WeaponInfo.IsValid())
			return false;

		if (WeaponInfo->m_iWeaponType == WEAPONTYPE_GRENADE) {
			if (!Weapon->m_bPinPulled() || (cmd->buttons & (IN_ATTACK | IN_ATTACK2))) {
				float throwTime = Weapon->m_fThrowTime();
				if (throwTime > 0.f)
					return false;
			}
		}
		else {
			if ((WeaponInfo->m_iWeaponType == WEAPONTYPE_KNIFE && cmd->buttons & (IN_ATTACK | IN_ATTACK2)) || cmd->buttons & IN_ATTACK) {
				if (LocalPlayer->CanShoot())
					return false;
			}
		}

		if (LocalPlayer->m_MoveType() == MOVETYPE_LADDER)
			return false;

		return true;
	}

	bool IsSafeAngle(const float yaw, CUserCmd* cmd) {
		auto localPlayer = C_CSPlayer::GetLocalPlayer();
		if (!localPlayer || localPlayer->IsDead())
			return false;

		// best target.
		struct AutoTarget_t { float fov; C_CSPlayer* player; };
		AutoTarget_t target{ 180.f + 1.f, nullptr };

		// iterate players, for closest distance.
		for (int i{ 1 }; i <= Interfaces::m_pGlobalVars->maxClients; ++i) {
			auto player = C_CSPlayer::GetPlayerByIndex(i);
			if (!player || player->IsDead())
				continue;

			if (player->IsDormant())
				continue;

			bool is_team = player->IsTeammate(localPlayer);
			if (is_team)
				continue;

			auto lag_data = Engine::LagCompensation::Get()->GetLagData(player->m_entIndex);
			if (!lag_data.IsValid())
				continue;

			// get best target based on fov.
			Vector origin = player->m_vecOrigin();

			auto AngleDistance = [&](QAngle& angles, const Vector& start, const Vector& end) -> float {
				auto direction = end - start;
				auto aimAngles = direction.ToEulerAngles();
				auto delta = aimAngles - angles;
				delta.Normalize();

				return sqrtf(delta.x * delta.x + delta.y * delta.y);
			};

			float fov = AngleDistance(cmd->viewangles, g_Vars.globals.m_vecFixedEyePosition, origin);

			if (fov < target.fov) {
				target.fov = fov;
				target.player = player;
			}
		}

		// get best player.
		const auto player = target.player;
		if (!player)
			return true;

		const auto yaw_rad = DEG2RAD(yaw);

		const auto head_pos_flick = g_Vars.globals.m_vecFixedEyePosition + Vector(std::cos(yaw_rad) * 25.f, std::sin(yaw_rad) * 25.f, 0);

		return Autowall::GetDamage(target.player, localPlayer, head_pos_flick) < 10.f;
	}

	Encrypted_t<AntiAimbot> AntiAimbot::Get() {
		static C_AntiAimbot instance;
		return &instance;
	}

	std::random_device random;
	std::mt19937 generator(random());
	void C_AntiAimbot::Main(bool* bSendPacket, bool* bFinalPacket, Encrypted_t<CUserCmd> cmd, bool ragebot) {
		C_CSPlayer* LocalPlayer = C_CSPlayer::GetLocalPlayer();

		if (!LocalPlayer || LocalPlayer->IsDead())
			return;

		auto animState = LocalPlayer->m_PlayerAnimState();
		if (!animState)
			return;

		if (!g_Vars.antiaim.enabled)
			return;

		Encrypted_t<CVariables::ANTIAIM_STATE> settings(&g_Vars.antiaim_stand);

		C_WeaponCSBaseGun* Weapon = (C_WeaponCSBaseGun*)LocalPlayer->m_hActiveWeapon().Get();

		if (!Weapon)
			return;

		auto WeaponInfo = Weapon->GetCSWeaponData();
		if (!WeaponInfo.IsValid())
			return;

		if (!IsEnabled(cmd, settings))
			return;

		if (LocalPlayer->m_MoveType() == MOVETYPE_LADDER) {
			auto eye_pos = LocalPlayer->GetEyePosition();

			CTraceFilterWorldAndPropsOnly filter;
			CGameTrace tr;
			Ray_t ray;
			float angle = 0.0f;
			while (true) {
				float cosa, sina;
				DirectX::XMScalarSinCos(&cosa, &sina, angle);

				Vector pos;
				pos.x = (cosa * 32.0f) + eye_pos.x;
				pos.y = (sina * 32.0f) + eye_pos.y;
				pos.z = eye_pos.z;

				ray.Init(eye_pos, pos,
					Vector(-1.0f, -1.0f, -4.0f),
					Vector(1.0f, 1.0f, 4.0f));
				Interfaces::m_pEngineTrace->TraceRay(ray, MASK_SOLID, &filter, &tr);
				if (tr.fraction < 1.0f)
					break;

				angle += DirectX::XM_PIDIV2;
				if (angle >= DirectX::XM_2PI) {
					return;
				}
			}

			float v23 = atan2(tr.plane.normal.x, std::fabsf(tr.plane.normal.y));
			float v24 = RAD2DEG(v23) + 90.0f;
			cmd->viewangles.pitch = 89.0f;
			if (v24 <= 180.0f) {
				if (v24 < -180.0f) {
					v24 = v24 + 360.0f;
				}
				cmd->viewangles.yaw = v24;
			}
			else {
				cmd->viewangles.yaw = v24 - 360.0f;
			}

			if (cmd->buttons & IN_BACK) {
				cmd->buttons |= IN_FORWARD;
				cmd->buttons &= ~IN_BACK;
			}
			else  if (cmd->buttons & IN_FORWARD) {
				cmd->buttons |= IN_BACK;
				cmd->buttons &= ~IN_FORWARD;
			}

			return;
		}

		bool move = LocalPlayer->m_vecVelocity().Length2D() > 0.1f && !g_Vars.globals.Fakewalking;

		// save view, depending if locked or not.
		//if ((g_Vars.antiaim.freestand_lock && move) || !g_Vars.antiaim.freestand_lock)
		//	m_view = cmd->viewangles.y;

		cmd->viewangles.x = g_Vars.globals.RegularAngles.x = GetAntiAimX(settings);
		float flYaw = GetAntiAimY(settings, cmd);

		// https://github.com/VSES/SourceEngine2007/blob/master/se2007/engine/cl_main.cpp#L1877-L1881
		if (!*bSendPacket || !*bFinalPacket) {
			cmd->viewangles.y = flYaw;
			//Distort(cmd, false);
		}

		else {
			std::uniform_int_distribution random(-90, 90);
			std::uniform_int_distribution randomjew(-settings->jitterrange, settings->jitterrange);

			switch (settings->fakeyaw)
			{
			case 1: // default
				cmd->viewangles.y = Math::AngleNormalize(flYaw + 180 + random(generator));
				break;

			case 2: // relative
				cmd->viewangles.y = Math::AngleNormalize(flYaw + 180 + settings->relativeamount);
				break;

			case 3: // jitter
				cmd->viewangles.y = Math::AngleNormalize(flYaw + 180 + randomjew(generator));
				break;

			case 4: // rotate
				cmd->viewangles.y = std::fmod(Interfaces::m_pGlobalVars->curtime * (settings->rotationspeed * 100.f), settings->rotationrange * 360.f);
				break;
			default: [[fallthrough]];
			}
		}

		static bool bNegative = false;
		auto bSwitch = std::fabs(Interfaces::m_pGlobalVars->curtime - g_Vars.globals.m_flBodyPred) < Interfaces::m_pGlobalVars->interval_per_tick;
		auto bSwap = std::fabs(Interfaces::m_pGlobalVars->curtime - g_Vars.globals.m_flBodyPred) > 1.1 - (Interfaces::m_pGlobalVars->interval_per_tick * 5);

		// for the Z fuckery LMFAO
		const auto jitterSpeed = 15.f + Interfaces::m_pClientState->m_nChokedCommands();
		static int lastTick = 0;
		static auto returnValue = 0.f;
		bool bJitterUpdate = false;
		if (lastTick + jitterSpeed < LocalPlayer->m_nTickBase() || lastTick > LocalPlayer->m_nTickBase())
		{
			lastTick = LocalPlayer->m_nTickBase();
			bJitterUpdate = !bJitterUpdate;
		}

		if (!Interfaces::m_pClientState->m_nChokedCommands()
			&& Interfaces::m_pGlobalVars->curtime >= g_Vars.globals.m_flBodyPred
			&& LocalPlayer->m_fFlags() & FL_ONGROUND && g_Vars.globals.m_bUpdate) {
			// fake yaw.
			switch (settings->yaw) {
			case 1: // dynamic
				bSwitch ? cmd->viewangles.y += 87.f : cmd->viewangles.y -= 92.f;
				bSwitch = !bSwitch;
				break;
			case 2: // sway 
				bNegative ? cmd->viewangles.y += 117.f : cmd->viewangles.y -= 107.f;
				break;
			case 3: // Z
				//	cmd->viewangles.y += bJitterUpdate ? -actualFuckery(generator) : actualFuckery(generator);
				//	break;
				//case 4: // static
					// logic behind this *should* work lmao
				if (g_Vars.antiaim.safebreak)
				{
					if (IsSafeAngle(g_Vars.antiaim.break_lby, cmd.Xor()))
						cmd->viewangles.y += g_Vars.antiaim.break_lby;
					else
						break;
				}
				else
					cmd->viewangles.y += g_Vars.antiaim.break_lby;
				break;
			default:
				break;
			}

			if (g_Vars.antiaim.funnymode) {
				cmd->viewangles.x = animState->m_aim_pitch_min;
				cmd->viewangles.z = 30.f;
			}

			m_flLowerBodyUpdateYaw = LocalPlayer->m_flLowerBodyYawTarget();
		}
	}

	void C_AntiAimbot::PrePrediction(bool* bSendPacket, Encrypted_t<CUserCmd> cmd) {
		if (!g_Vars.antiaim.enabled)
			return;

		C_CSPlayer* local = C_CSPlayer::GetLocalPlayer();
		if (!local || local->IsDead())
			return;

		Encrypted_t<CVariables::ANTIAIM_STATE> settings(&g_Vars.antiaim_stand);

		//g_Vars.globals.m_bInverted = g_Vars.antiaim.desync_flip_bind.enabled;

		if (!IsEnabled(cmd, settings)) {
			g_Vars.globals.RegularAngles = cmd->viewangles;
			return;
		}
	}

	float C_AntiAimbot::GetAntiAimX(Encrypted_t<CVariables::ANTIAIM_STATE> settings) {
		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || local->IsDead())
			return FLT_MAX;

		auto animState = local->m_PlayerAnimState();
		if (!animState)
			return FLT_MAX;

		switch (settings->pitch) {
		case 1: // down
			return 89.f;
		case 2: // up 
			return -89.f;
		case 3: // zero
			return 0.f;
		case 4:
			return animState->m_aim_pitch_min;
		default:
			return FLT_MAX;
			break;
		}
	}

	bool DoEdgeAntiAim(C_BasePlayer* player, QAngle& out) {
		CGameTrace trace;

		if (player->m_MoveType() == MOVETYPE_LADDER)
			return false;

		// skip this player in our traces.
		static CTraceFilterSkipEntity filter(player);

		// get player bounds.
		Vector mins = player->OBBMins();
		Vector maxs = player->OBBMaxs();

		// make player bounds bigger.
		mins.x -= 20.f;
		mins.y -= 20.f;
		maxs.x += 20.f;
		maxs.y += 20.f;

		// get player origin.
		Vector start = player->GetAbsOrigin();

		// offset the view.
		start.z += 56.f;

		Interfaces::m_pEngineTrace->TraceRay(Ray_t(start, start, mins, maxs), CONTENTS_SOLID, (ITraceFilter*)&filter, &trace);
		if (!trace.startsolid)
			return false;

		float  smallest = 1.f;
		Vector plane;

		// trace around us in a circle, in 20 steps (anti-degree conversion).
		// find the closest object.
		for (float step{ }; step <= (M_PI * 2.f); step += (M_PI / 10.f)) {
			// extend endpoint x units.
			Vector end = start;

			// set end point based on range and step.
			end.x += std::cos(step) * 32.f;
			end.y += std::sin(step) * 32.f;

			Interfaces::m_pEngineTrace->TraceRay(Ray_t(start, end, { -1.f, -1.f, -8.f }, { 1.f, 1.f, 8.f }), CONTENTS_SOLID, (ITraceFilter*)&filter, &trace);

			// we found an object closer, then the previouly found object.
			if (trace.fraction < smallest) {
				// save the normal of the object.
				plane = trace.plane.normal;
				smallest = trace.fraction;
			}
		}

		// no valid object was found.
		if (smallest == 1.f || plane.z >= 0.1f)
			return false;

		// invert the normal of this object
		// this will give us the direction/angle to this object.
		Vector inv = -plane;
		Vector dir = inv;
		dir.Normalize();

		// extend point into object by 24 units.
		Vector point = start;
		point.x += (dir.x * 24.f);
		point.y += (dir.y * 24.f);

		// check if we can stick our head into the wall.
		if (Interfaces::m_pEngineTrace->GetPointContents(point, CONTENTS_SOLID) & CONTENTS_SOLID) {
			// trace from 72 units till 56 units to see if we are standing behind something.
			Interfaces::m_pEngineTrace->TraceRay(Ray_t(point + Vector{ 0.f, 0.f, 16.f }, point), CONTENTS_SOLID, (ITraceFilter*)&filter, &trace);

			// we didnt start in a solid, so we started in air.
			// and we are not in the ground.
			if (trace.fraction < 1.f && !trace.startsolid && trace.plane.normal.z > 0.7f) {
				// mean we are standing behind a solid object.
				// set our angle to the inversed normal of this object.
				out.y = RAD2DEG(std::atan2(inv.y, inv.x));
				return true;
			}
		}

		// if we arrived here that mean we could not stick our head into the wall.
		// we can still see if we can stick our head behind/asides the wall.

		// adjust bounds for traces.
		mins = { (dir.x * -3.f) - 1.f, (dir.y * -3.f) - 1.f, -1.f };
		maxs = { (dir.x * 3.f) + 1.f, (dir.y * 3.f) + 1.f, 1.f };

		// move this point 48 units to the left 
		// relative to our wall/base point.
		Vector left = start;
		left.x = point.x - (inv.y * 48.f);
		left.y = point.y - (inv.x * -48.f);

		Interfaces::m_pEngineTrace->TraceRay(Ray_t(left, point, mins, maxs), CONTENTS_SOLID, (ITraceFilter*)&filter, &trace);
		float l = trace.startsolid ? 0.f : trace.fraction;

		// move this point 48 units to the right 
		// relative to our wall/base point.
		Vector right = start;
		right.x = point.x + (inv.y * 48.f);
		right.y = point.y + (inv.x * -48.f);

		Interfaces::m_pEngineTrace->TraceRay(Ray_t(right, point, mins, maxs), CONTENTS_SOLID, (ITraceFilter*)&filter, &trace);
		float r = trace.startsolid ? 0.f : trace.fraction;

		// both are solid, no edge.
		if (l == 0.f && r == 0.f)
			return false;

		// set out to inversed normal.
		out.y = RAD2DEG(std::atan2(inv.y, inv.x));

		// left started solid.
		// set angle to the left.
		if (l == 0.f) {
			out.y += 90.f;
			return true;
		}

		// right started solid.
		// set angle to the right.
		if (r == 0.f) {
			out.y -= 90.f;
			return true;
		}

		return false;
	}

	float C_AntiAimbot::GetAntiAimY(Encrypted_t<CVariables::ANTIAIM_STATE> settings, Encrypted_t<CUserCmd> cmd) {
		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || local->IsDead())
			return FLT_MAX;

		float freestandingReturnYaw = std::numeric_limits< float >::max();
		float flViewAngle = cmd->viewangles.y;

		auto GetTargetYaw = [local, flViewAngle]() -> float
		{
			float_t flBestDistance = FLT_MAX;

			C_CSPlayer* pFinalPlayer = nullptr;
			for (int32_t i = 1; i < 65; i++)
			{
				C_CSPlayer* pPlayer = C_CSPlayer::GetPlayerByIndex(i);
				if (!pPlayer || !pPlayer->IsPlayer() || pPlayer->IsDead() || pPlayer->m_iTeamNum() == local->m_iTeamNum() || pPlayer->IsDormant())
					continue;

				if (pPlayer->m_fFlags() & FL_FROZEN)
					continue;

				float_t flDistanceToPlayer = local->m_vecOrigin().Distance(pPlayer->m_vecOrigin());
				if (flDistanceToPlayer > flBestDistance)
					continue;

				if (flDistanceToPlayer > 1250.0f)
					continue;

				flBestDistance = flDistanceToPlayer;
				pFinalPlayer = pPlayer;
			}

			if (!pFinalPlayer)
				return flViewAngle + 180.0f;

			return Math::CalcAngle(local->GetAbsOrigin() + local->m_vecViewOffset(), pFinalPlayer->GetAbsOrigin()).yaw + 180.0f;
		};

		float flRetValue = (settings->at_targets ? GetTargetYaw() : flViewAngle) + 180.f;

		bool bUsingManualAA = g_Vars.globals.manual_aa >= 0;
		if (bUsingManualAA) {
			m_bUsingManual = true;
			switch (g_Vars.globals.manual_aa) {
			case 0:
				flRetValue = flViewAngle + 90.f;
				break;
			case 1:
				flRetValue = flViewAngle + 180.f;
				break;
			case 2:
				flRetValue = flViewAngle - 90.f;
				break;
			}
			goto RETURN;
		}
		else m_bUsingManual = false;

		bool bStahlhelmEnabled = false, bFreestandingEnabled = false;

		QAngle returnAngle;
		if (g_Vars.antiaim.stahlhelm && local->m_fFlags() & FL_ONGROUND && DoEdgeAntiAim(local, returnAngle) && !bUsingManualAA)
		{
			bStahlhelmEnabled = true;
			flRetValue = returnAngle.y;
			goto RETURN;
		}

		// this is inside of an else because iron helmet takes priority over freestanding.
		else
		{
			bStahlhelmEnabled = false;

			if (g_Vars.antiaim.freestanding_mode == 0)
			{
				if (local->m_vecVelocity().Length() > 3.f && local->m_fFlags() & FL_ONGROUND && g_Vars.antiaim.freestand_move && !bUsingManualAA)
				{
					const C_AntiAimbot::Directions Direction = HandleDirection(cmd);
					switch (Direction) {
					case Directions::YAW_BACK:
						// backwards yaw.
						bFreestandingEnabled = true;
						flRetValue = flViewAngle + 180.f;
						goto RETURN;
						break; // we m ight not need this break after the goto return.
					case Directions::YAW_LEFT:
						// left yaw.
						bFreestandingEnabled = true;
						flRetValue = flViewAngle + 90.f;
						goto RETURN;
						break; // we m ight not need this break after the goto return.
					case Directions::YAW_RIGHT:
						// right yaw.
						bFreestandingEnabled = true;
						flRetValue = flViewAngle - 90.f;
						goto RETURN;
						break; // we m ight not need this break after the goto return.
					case Directions::YAW_NONE:
						// break because we already have an angle!.
						bFreestandingEnabled = false;
						break;
					}
				}

				if (local->m_vecVelocity().Length() < 3.f && g_Vars.antiaim.freestand_stand && !bUsingManualAA)
				{
					const C_AntiAimbot::Directions Direction = HandleDirection(cmd);
					switch (Direction) {
					case Directions::YAW_BACK:
						// backwards yaw.
						bFreestandingEnabled = true;
						flRetValue = flViewAngle + 180.f;
						goto RETURN;
						break; // we m ight not need this break after the goto return.
					case Directions::YAW_LEFT:
						// left yaw.
						bFreestandingEnabled = true;
						flRetValue = flViewAngle + 90.f;
						goto RETURN;
						break; // we m ight not need this break after the goto return.
					case Directions::YAW_RIGHT:
						// right yaw.
						bFreestandingEnabled = true;
						flRetValue = flViewAngle - 90.f;
						goto RETURN;
						break; // we m ight not need this break after the goto return.
					case Directions::YAW_NONE:
						// break because we already have an angle!.
						bFreestandingEnabled = false;
						break;
					}
				}
			}
		}

		static int Ticks = 0;
		if (!bFreestandingEnabled && !bStahlhelmEnabled && !bUsingManualAA)
		{
			std::uniform_int_distribution randomcockhead(-settings->randomrange, settings->randomrange);
			bool bJitterSwitch = false;

			switch (settings->base_yaw)
			{
			case 1: // backwards.
				flRetValue = settings->at_targets ? GetTargetYaw() : flViewAngle + 180.f;
				break;
			case 2: // jitter
				flRetValue = bJitterSwitch ? (settings->at_targets ? GetTargetYaw() : flViewAngle) + -settings->realjitterrange : (settings->at_targets ? GetTargetYaw() : flViewAngle) + settings->realjitterrange;
				bJitterSwitch = !bJitterSwitch;
				break;
			case 3: // spin
				flRetValue = std::fmod(Interfaces::m_pGlobalVars->curtime * (settings->RotationSpeed * 100.f), settings->RotationRange * 360.f); break;
				break;
			case 4: // random
				flRetValue = Math::AngleNormalize((settings->at_targets ? GetTargetYaw() : flViewAngle) + randomcockhead(generator));
				break;
			case 5: // static
				flRetValue = Math::AngleNormalize((settings->at_targets ? GetTargetYaw() : flViewAngle) + settings->staticangle);
				break;
			case 6: // 180z
				flRetValue -= Ticks;
				Ticks += 2;

				if (Ticks > 240)
					Ticks = 120;
				break;
			default: [[fallthrough]];
			}

			goto RETURN;
		}

	RETURN:
		g_Vars.globals.RegularAngles.y = flRetValue;
		return flRetValue;
	}

	void C_AntiAimbot::Distort(Encrypted_t<CUserCmd> cmd, bool usingFakeYaw) {
		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || local->IsDead())
			return;

		if (!g_Vars.antiaim.distort_tggl)
			return;

		bool bDoDistort;
		if (g_Vars.antiaim.distort_disable_fakewalk && g_Vars.globals.Fakewalking)
			bDoDistort = false;
		else if (g_Vars.antiaim.distort_disable_air && !g_Vars.globals.m_bGround)
			bDoDistort = false;
		else
			bDoDistort = true;

		static float flLastMoveTime = FLT_MAX;
		static float flLastMoveYaw = FLT_MAX;
		static bool bGenerate = true;
		static float flGenerated = 0.f;

		if (local->m_PlayerAnimState()->m_velocity_length_xy > 0.1f && g_Vars.globals.m_bGround && !g_Vars.globals.Fakewalking) {
			flLastMoveTime = Interfaces::m_pGlobalVars->realtime;
			flLastMoveYaw = local->m_flLowerBodyYawTarget();

			if (g_Vars.antiaim.distort_disable_run)
				bDoDistort = false;
		}

		if (m_bUsingManual)
			bDoDistort = false;

		if (flLastMoveTime == FLT_MAX)
			return;

		if (flLastMoveYaw == FLT_MAX)
			return;

		if (bDoDistort) {

			// twist = force turn ;)

			if (usingFakeYaw)
			{
				if (g_Vars.antiaim.fake_distort_twist) {

					float max = 180 * (g_Vars.antiaim.fake_distort_range * (0.01f * g_Vars.antiaim.fake_distort_speed));
					float min = 0;
					float speed = 100 - (g_Vars.antiaim.fake_distort_max_time);

					cmd->viewangles.y = std::min(std::max(floor(abs(cos(Interfaces::m_pGlobalVars->curtime / speed * 100)) * max), min), max) - max / 2;
					return;
				}

				float max = 180 * (g_Vars.antiaim.fake_distort_range * 0.01f);
				float min = 0;
				float speed = 100 - (g_Vars.antiaim.fake_distort_max_time);

				cmd->viewangles.y = std::min(std::max(floor(abs(cos(Interfaces::m_pGlobalVars->curtime / speed * 100)) * max), min), max) - max / 2;
			}

			// normal distortion
			else
			{
				if (g_Vars.antiaim.distort_force_turn) {

					float max = 180 * (g_Vars.antiaim.distort_rmg * (0.01f * g_Vars.antiaim.distort_force_turn_spd));
					float min = 0;
					float speed = 100 - (g_Vars.antiaim.distort_spd);

					cmd->viewangles.y = 180 + std::min(std::max(floor(abs(cos(Interfaces::m_pGlobalVars->curtime / speed * 100)) * max), min), max) - max / 2;
					return;
				}

				float max = 180 * (g_Vars.antiaim.distort_rmg * 0.01f);
				float min = 0;
				float speed = 100 - (g_Vars.antiaim.distort_spd);

				cmd->viewangles.y = 180 + std::min(std::max(floor(abs(cos(Interfaces::m_pGlobalVars->curtime / speed * 100)) * max), min), max) - max / 2;
			}
		}
	}

	void C_AntiAimbot::ImposterBreaker(bool* bSendPacket, Encrypted_t<CUserCmd> cmd) {
		auto pLocal = C_CSPlayer::GetLocalPlayer();
		if (!pLocal)
			return;

		bool bCrouching = pLocal->m_PlayerAnimState()->m_anim_duck_amount > 0.f;
		float flVelocity = (bCrouching ? 3.25f : 1.01f);
		static int iUpdates = 0;

		if (!(g_Vars.globals.m_pCmd->buttons & IN_FORWARD) && !(g_Vars.globals.m_pCmd->buttons & IN_BACK) && !(g_Vars.globals.m_pCmd->buttons & IN_MOVELEFT) && !(g_Vars.globals.m_pCmd->buttons & IN_MOVERIGHT) && !(g_Vars.globals.m_pCmd->buttons & IN_JUMP))
		{
			if (Interfaces::m_pClientState->m_nChokedCommands() == 2)
			{
				cmd->forwardmove = iUpdates % 2 ? -450 : 450;
				++iUpdates;
			}
		}
	}

	C_AntiAimbot::Directions C_AntiAimbot::HandleDirection(Encrypted_t<CUserCmd> cmd) {
		m_auto_dist = -1;

		const auto pLocal = C_CSPlayer::GetLocalPlayer();
		if (!pLocal)
			return Directions::YAW_NONE;

		// best target.
		struct AutoTarget_t { float fov; C_CSPlayer* player; };
		AutoTarget_t target{ 180.f + 1.f, nullptr };

		// iterate players, for closest distance.
		for (int i{ 1 }; i <= Interfaces::m_pGlobalVars->maxClients; ++i) {
			auto player = C_CSPlayer::GetPlayerByIndex(i);
			if (!player || player->IsDead())
				continue;

			if (player->IsDormant())
				continue;

			bool is_team = player->IsTeammate(pLocal);
			if (is_team)
				continue;

			auto lag_data = Engine::LagCompensation::Get()->GetLagData(player->m_entIndex);
			if (!lag_data.IsValid())
				continue;

			// get best target based on fov.
			Vector origin = player->m_vecOrigin();

			auto AngleDistance = [&](QAngle& angles, const Vector& start, const Vector& end) -> float {
				auto direction = end - start;
				auto aimAngles = direction.ToEulerAngles();
				auto delta = aimAngles - angles;
				delta.Normalize();

				return sqrtf(delta.x * delta.x + delta.y * delta.y);
			};

			float fov = AngleDistance(cmd->viewangles, g_Vars.globals.m_vecFixedEyePosition, origin);

			if (fov < target.fov) {
				target.fov = fov;
				target.player = player;
			}
		}

		// get best player.
		const auto player = target.player;
		if (!player)
			return Directions::YAW_NONE;

		Vector& bestOrigin = player->m_vecOrigin();

		// calculate direction from bestOrigin to our origin
		const auto yaw = Math::CalcAngle(bestOrigin, pLocal->m_vecOrigin());

		Vector forward, right, up;
		Math::AngleVectors(yaw, forward, right, up);

		Vector vecStart = pLocal->GetEyePosition();
		Vector vecEnd = vecStart + forward * 100.0f;

		Ray_t rightRay(vecStart + right * 35.0f, vecEnd + right * 35.0f), leftRay(vecStart - right * 35.0f, vecEnd - right * 35.0f);

		// setup trace filter
		CTraceFilter filter{ };
		filter.pSkip = pLocal;

		CGameTrace tr{ };

		m_pEngineTrace->TraceRay(rightRay, MASK_SOLID, &filter, &tr);
		float rightLength = (tr.endpos - tr.startpos).Length();

		m_pEngineTrace->TraceRay(leftRay, MASK_SOLID, &filter, &tr);
		float leftLength = (tr.endpos - tr.startpos).Length();

		static auto leftTicks = 0;
		static auto rightTicks = 0;
		static auto backTicks = 0;

		if (rightLength - leftLength > 20.0f) {
			leftTicks++;
			m_auto_dist = leftLength;
		}
		else
			leftTicks = 0;

		if (leftLength - rightLength > 20.0f) {
			rightTicks++;
			m_auto_dist = rightLength;
		}
		else
			rightTicks = 0;

		if (fabs(rightLength - leftLength) <= 20.0f) {
			backTicks++;
		}
		else
			backTicks = 0;

		Directions direction = Directions::YAW_NONE;

		if (rightTicks > 10) {
			direction = Directions::YAW_RIGHT;
			m_auto = -90;
		}
		else {
			if (leftTicks > 10) {
				direction = Directions::YAW_LEFT;
				m_auto = 90;
			}
			else {
				if (backTicks > 10)
					direction = Directions::YAW_BACK;
				m_auto = 180;
			}
		}

		return direction;
	}
}