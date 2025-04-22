#pragma once

#include "../../SDK/sdk.hpp"

struct PostSimulationContextData_t
{
	bool walking;
	int buttons;
	int meme1;
	float Origin;
	Vector m_vecOrigin;
	Vector pMins, pMaxs;
	float someFloat;
	Vector m_vecVelocity;
	int simulationTicks;
	int flags;
	CGameTrace trace;
	float flMaxSpeed;
	ITraceFilter* filter;
	C_CSPlayer* player;
};

class SimulationContext : public Core::Singleton<SimulationContext> {
private: // variables called in class
	bool walking;
	int buttons;
	int meme1;
	float Origin;
	Vector m_vecOrigin;
	Vector pMins, pMaxs;
	float someFloat;
	Vector m_vecVelocity;
	int simulationTicks;
	int flags;
	CGameTrace trace;
	float gravity;
	float sv_jump_impulse;
	float stepsize;
	float flMaxSpeed;
	ITraceFilter* filter;
	C_CSPlayer* player;

	bool m_bRebuiltGameMovement = false;

private: // functions called in class
	void TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, CGameTrace& pm);
	void TryPlayerMove();
	void RebuildGameMovement(CUserCmd* ucmd);

public: // variables called outside of class
	bool m_bRebuiltGameMovementDataWritten = false;
	PostSimulationContextData_t sPostSimulationContextData;

public: // functions called out of class
	void InitSimulationContext(C_CSPlayer* player);
	void ExtrapolatePlayer(float yaw);

	// test function
	void WriteSimulateContextData()
	{
		if (!m_bRebuiltGameMovement)
			return;

		sPostSimulationContextData.walking = walking;
		sPostSimulationContextData.buttons = buttons;
		sPostSimulationContextData.meme1 = meme1;
		sPostSimulationContextData.Origin = Origin;
		sPostSimulationContextData.m_vecOrigin = m_vecOrigin;
		sPostSimulationContextData.pMins = pMins;
		sPostSimulationContextData.pMaxs = pMaxs;
		sPostSimulationContextData.someFloat = someFloat;
		sPostSimulationContextData.m_vecVelocity = m_vecVelocity;
		sPostSimulationContextData.simulationTicks = simulationTicks;
		sPostSimulationContextData.flags = flags;
		sPostSimulationContextData.trace = trace;
		sPostSimulationContextData.flMaxSpeed = flMaxSpeed;
		sPostSimulationContextData.filter = filter;
		sPostSimulationContextData.player = player;

		m_bRebuiltGameMovementDataWritten = true;
	}
};