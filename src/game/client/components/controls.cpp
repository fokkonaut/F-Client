/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>

#include "controls.h"

CControls::CControls()
{
	mem_zero(&m_LastData, sizeof(m_LastData));
	m_LastDummy = 0;
	m_OtherFire = 0;
}

void CControls::OnReset()
{
	ResetInput(CLIENT_MAIN);
	ResetInput(CLIENT_DUMMY);
}

void CControls::ResetInput(int Dummy)
{
	m_LastData[Dummy].m_Direction = 0;
	//m_LastData[Dummy].m_Hook = 0;
	// simulate releasing the fire button
	if((m_LastData[Dummy].m_Fire&1) != 0)
		m_LastData[Dummy].m_Fire++;
	m_LastData[Dummy].m_Fire &= INPUT_STATE_MASK;
	m_LastData[Dummy].m_Jump = 0;
	m_InputData[Dummy] = m_LastData[Dummy];

	m_InputDirectionLeft[Dummy] = 0;
	m_InputDirectionRight[Dummy] = 0;
}

void CControls::OnRelease()
{
	OnReset();
}

void CControls::OnPlayerDeath()
{
	if(!m_pClient->m_Snap.m_pGameDataRace || !(m_pClient->m_Snap.m_pGameDataRace->m_RaceFlags&RACEFLAG_KEEP_WANTED_WEAPON))
		m_LastData[Config()->m_ClDummy].m_WantedWeapon = m_InputData[Config()->m_ClDummy].m_WantedWeapon = 0;
}

struct CInputState
{
	CControls *m_pControls;
	int *m_pVariable[NUM_CLIENTS];
};

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	CInputState *pState = (CInputState *)pUserData;
	*pState->m_pVariable[pState->m_pControls->GameClient()->Config()->m_ClDummy] = pResult->GetInteger(0);
}

static void ConKeyInputCounter(IConsole::IResult *pResult, void *pUserData)
{
	CInputState *pState = (CInputState *)pUserData;

	int *v = pState->m_pVariable[pState->m_pControls->GameClient()->Config()->m_ClDummy];
	if(((*v)&1) != pResult->GetInteger(0))
		(*v)++;
	*v &= INPUT_STATE_MASK;
}

struct CInputSet
{
	CControls *m_pControls;
	int *m_pVariable[NUM_CLIENTS];
	int m_Value;
};

static void ConKeyInputSet(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	if(pResult->GetInteger(0))
		*pSet->m_pVariable[pSet->m_pControls->GameClient()->Config()->m_ClDummy] = pSet->m_Value;
}

static void ConKeyInputNextPrevWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	ConKeyInputCounter(pResult, pSet);
	pSet->m_pControls->m_InputData[pSet->m_pControls->GameClient()->Config()->m_ClDummy].m_WantedWeapon = 0;
}

void CControls::OnConsoleInit()
{
	// game commands
	{ static CInputState s_State = {this, &m_InputDirectionLeft[CLIENT_MAIN], &m_InputDirectionLeft[CLIENT_DUMMY]}; Console()->Register("+left", "", CFGFLAG_CLIENT, ConKeyInputState, (void *)&s_State, "Move left"); }
	{ static CInputState s_State = {this, &m_InputDirectionRight[CLIENT_MAIN], &m_InputDirectionRight[CLIENT_DUMMY]}; Console()->Register("+right", "", CFGFLAG_CLIENT, ConKeyInputState, (void *)&s_State, "Move right"); }
	{ static CInputState s_State = {this, &m_InputData[CLIENT_MAIN].m_Jump, &m_InputData[CLIENT_DUMMY].m_Jump}; Console()->Register("+jump", "", CFGFLAG_CLIENT, ConKeyInputState, (void *)&s_State, "Jump"); }
	{ static CInputState s_State = {this, &m_InputData[CLIENT_MAIN].m_Hook, &m_InputData[CLIENT_DUMMY].m_Hook}; Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, (void *)&s_State, "Hook"); }
	{ static CInputState s_State = {this, &m_InputData[CLIENT_MAIN].m_Fire, &m_InputData[CLIENT_DUMMY].m_Fire}; Console()->Register("+fire", "", CFGFLAG_CLIENT, ConKeyInputCounter, (void *)&s_State, "Fire"); }

	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_WantedWeapon, &m_InputData[CLIENT_DUMMY].m_WantedWeapon, 1}; Console()->Register("+weapon1", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to hammer"); }
	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_WantedWeapon, &m_InputData[CLIENT_DUMMY].m_WantedWeapon, 2}; Console()->Register("+weapon2", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to gun"); }
	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_WantedWeapon, &m_InputData[CLIENT_DUMMY].m_WantedWeapon, 3}; Console()->Register("+weapon3", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to shotgun"); }
	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_WantedWeapon, &m_InputData[CLIENT_DUMMY].m_WantedWeapon, 4}; Console()->Register("+weapon4", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to grenade"); }
	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_WantedWeapon, &m_InputData[CLIENT_DUMMY].m_WantedWeapon, 5}; Console()->Register("+weapon5", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to laser"); }

	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_NextWeapon, &m_InputData[CLIENT_DUMMY].m_NextWeapon, 0}; Console()->Register("+nextweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to next weapon"); }
	{ static CInputSet s_Set = {this, &m_InputData[CLIENT_MAIN].m_PrevWeapon, &m_InputData[CLIENT_DUMMY].m_PrevWeapon, 0}; Console()->Register("+prevweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to previous weapon"); }
}

void CControls::OnMessage(int Msg, void *pRawMsg)
{
	if(Msg == NETMSGTYPE_SV_WEAPONPICKUP)
	{
		CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)pRawMsg;
		if(Config()->m_ClAutoswitchWeapons)
			m_InputData[Config()->m_ClDummy].m_WantedWeapon = pMsg->m_Weapon+1;
	}
}

int CControls::SnapInput(int *pData)
{
	static int64 LastSendTime = 0;
	bool Send = false;

	// update player state
	if(m_pClient->m_pChat->IsActive())
		m_InputData[Config()->m_ClDummy].m_PlayerFlags = PLAYERFLAG_CHATTING;
	else
		m_InputData[Config()->m_ClDummy].m_PlayerFlags = 0;

	if(m_pClient->m_pScoreboard->IsActive())
		m_InputData[Config()->m_ClDummy].m_PlayerFlags |= PLAYERFLAG_SCOREBOARD;

	if(m_LastData[Config()->m_ClDummy].m_PlayerFlags != m_InputData[Config()->m_ClDummy].m_PlayerFlags)
		Send = true;

	m_LastData[Config()->m_ClDummy].m_PlayerFlags = m_InputData[Config()->m_ClDummy].m_PlayerFlags;

	// we freeze the input if chat or menu is activated
	if(m_pClient->m_pChat->IsActive() || m_pClient->m_pMenus->IsActive())
	{
		OnReset();

		mem_copy(pData, &m_InputData[Config()->m_ClDummy], sizeof(m_InputData[CLIENT_MAIN]));

		// send once a second just to be sure
		if(time_get() > LastSendTime + time_freq())
			Send = true;
	}
	else
	{
		m_InputData[Config()->m_ClDummy].m_TargetX = (int)m_MousePos[Config()->m_ClDummy].x;
		m_InputData[Config()->m_ClDummy].m_TargetY = (int)m_MousePos[Config()->m_ClDummy].y;
		if(!m_InputData[Config()->m_ClDummy].m_TargetX && !m_InputData[Config()->m_ClDummy].m_TargetY)
		{
			m_InputData[Config()->m_ClDummy].m_TargetX = 1;
			m_MousePos[Config()->m_ClDummy].x = 1;
		}

		// set direction
		m_InputData[Config()->m_ClDummy].m_Direction = 0;
		if(m_InputDirectionLeft[Config()->m_ClDummy] && !m_InputDirectionRight[Config()->m_ClDummy])
			m_InputData[Config()->m_ClDummy].m_Direction = -1;
		if(!m_InputDirectionLeft[Config()->m_ClDummy] && m_InputDirectionRight[Config()->m_ClDummy])
			m_InputData[Config()->m_ClDummy].m_Direction = 1;

		// dummy copy moves
		if(Config()->m_ClDummyCopyMoves)
		{
			CNetObj_PlayerInput *pDummyInput = &m_pClient->m_DummyInput;
			pDummyInput->m_Direction = m_InputData[Config()->m_ClDummy].m_Direction;
			pDummyInput->m_Hook = m_InputData[Config()->m_ClDummy].m_Hook;
			pDummyInput->m_Jump = m_InputData[Config()->m_ClDummy].m_Jump;
			pDummyInput->m_PlayerFlags = m_InputData[Config()->m_ClDummy].m_PlayerFlags;
			pDummyInput->m_TargetX = m_InputData[Config()->m_ClDummy].m_TargetX;
			pDummyInput->m_TargetY = m_InputData[Config()->m_ClDummy].m_TargetY;
			pDummyInput->m_WantedWeapon = m_InputData[Config()->m_ClDummy].m_WantedWeapon;

			pDummyInput->m_Fire += m_InputData[Config()->m_ClDummy].m_Fire - m_LastData[Config()->m_ClDummy].m_Fire;
			pDummyInput->m_NextWeapon += m_InputData[Config()->m_ClDummy].m_NextWeapon - m_LastData[Config()->m_ClDummy].m_NextWeapon;
			pDummyInput->m_PrevWeapon += m_InputData[Config()->m_ClDummy].m_PrevWeapon - m_LastData[Config()->m_ClDummy].m_PrevWeapon;

			m_InputData[!Config()->m_ClDummy] = *pDummyInput;
		}

		if(Config()->m_ClDummyControl)
		{
			CNetObj_PlayerInput *pDummyInput = &m_pClient->m_DummyInput;
			pDummyInput->m_Jump = Config()->m_ClDummyJump;
			pDummyInput->m_Fire = Config()->m_ClDummyFire;
			pDummyInput->m_Hook = Config()->m_ClDummyHook;
		}

		// stress testing
		if(Config()->m_DbgStress)
		{
			float t = Client()->LocalTime();
			mem_zero(&m_InputData[Config()->m_ClDummy], sizeof(m_InputData));

			m_InputData[Config()->m_ClDummy].m_Direction = ((int)t/2)%3-1;
			m_InputData[Config()->m_ClDummy].m_Jump = ((int)t)&1;
			m_InputData[Config()->m_ClDummy].m_Fire = ((int)(t*10));
			m_InputData[Config()->m_ClDummy].m_Hook = ((int)(t*2))&1;
			m_InputData[Config()->m_ClDummy].m_WantedWeapon = ((int)t)%NUM_WEAPONS;
			m_InputData[Config()->m_ClDummy].m_TargetX = (int)(sinf(t*3)*100.0f);
			m_InputData[Config()->m_ClDummy].m_TargetY = (int)(cosf(t*3)*100.0f);
		}

		// check if we need to send input
		if(m_InputData[Config()->m_ClDummy].m_Direction != m_LastData[Config()->m_ClDummy].m_Direction) Send = true;
		else if(m_InputData[Config()->m_ClDummy].m_Jump != m_LastData[Config()->m_ClDummy].m_Jump) Send = true;
		else if(m_InputData[Config()->m_ClDummy].m_Fire != m_LastData[Config()->m_ClDummy].m_Fire) Send = true;
		else if(m_InputData[Config()->m_ClDummy].m_Hook != m_LastData[Config()->m_ClDummy].m_Hook) Send = true;
		else if(m_InputData[Config()->m_ClDummy].m_WantedWeapon != m_LastData[Config()->m_ClDummy].m_WantedWeapon) Send = true;
		else if(m_InputData[Config()->m_ClDummy].m_NextWeapon != m_LastData[Config()->m_ClDummy].m_NextWeapon) Send = true;
		else if(m_InputData[Config()->m_ClDummy].m_PrevWeapon != m_LastData[Config()->m_ClDummy].m_PrevWeapon) Send = true;

		// send at at least 10hz
		if(time_get() > LastSendTime + time_freq()/25)
			Send = true;
	}

	// copy and return size
	m_LastData[Config()->m_ClDummy] = m_InputData[Config()->m_ClDummy];

	if(!Send)
		return 0;

	LastSendTime = time_get();
	mem_copy(pData, &m_InputData[Config()->m_ClDummy], sizeof(m_InputData[CLIENT_MAIN]));
	return sizeof(m_InputData[CLIENT_MAIN]);
}

void CControls::OnRender()
{
	ClampMousePos();

	// update target pos
	if(m_pClient->m_Snap.m_pGameData && !m_pClient->m_Snap.m_SpecInfo.m_Active)
		m_TargetPos[Config()->m_ClDummy] = m_pClient->m_LocalCharacterPos + m_MousePos[Config()->m_ClDummy];
	else if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
		m_TargetPos[Config()->m_ClDummy] = m_pClient->m_Snap.m_SpecInfo.m_Position + m_MousePos[Config()->m_ClDummy];
	else
		m_TargetPos[Config()->m_ClDummy] = m_MousePos[Config()->m_ClDummy];
}

bool CControls::OnMouseMove(float x, float y)
{
	if((m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER)) ||
		(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_pChat->IsActive()))
		return false;

	m_MousePos[Config()->m_ClDummy] += vec2(x, y); // TODO: ugly

	return true;
}

void CControls::ClampMousePos()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		m_MousePos[Config()->m_ClDummy].x = clamp(m_MousePos[Config()->m_ClDummy].x, 200.0f, Collision()->GetWidth()*32-200.0f);
		m_MousePos[Config()->m_ClDummy].y = clamp(m_MousePos[Config()->m_ClDummy].y, 200.0f, Collision()->GetHeight()*32-200.0f);
	}
	else
	{
		float MouseMax;
		if(Config()->m_ClDynamicCamera)
		{
			float CameraMaxDistance = 200.0f;
			float FollowFactor = Config()->m_ClMouseFollowfactor/100.0f;
			MouseMax = min(CameraMaxDistance/FollowFactor + Config()->m_ClMouseDeadzone, (float)Config()->m_ClMouseMaxDistanceDynamic);
		}
		else
			MouseMax = (float)Config()->m_ClMouseMaxDistanceStatic;

		if(length(m_MousePos[Config()->m_ClDummy]) > MouseMax)
			m_MousePos[Config()->m_ClDummy] = normalize(m_MousePos[Config()->m_ClDummy])*MouseMax;
	}
}
