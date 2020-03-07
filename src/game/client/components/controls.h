/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CONTROLS_H
#define GAME_CLIENT_COMPONENTS_CONTROLS_H
#include <base/vmath.h>
#include <game/client/component.h>

class CControls : public CComponent
{
public:
	vec2 m_MousePos[NUM_CLIENTS];
	vec2 m_TargetPos[NUM_CLIENTS];

	CNetObj_PlayerInput m_InputData[NUM_CLIENTS];
	CNetObj_PlayerInput m_LastData[NUM_CLIENTS];
	int m_InputDirectionLeft[NUM_CLIENTS];
	int m_InputDirectionRight[NUM_CLIENTS];
	int m_LastDummy;
	int m_OtherFire;

	CControls();

	virtual void OnReset();
	virtual void OnRelease();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnConsoleInit();
	virtual void OnPlayerDeath();

	int SnapInput(int *pData);
	void ClampMousePos();
	void ResetInput(int Dummy);

	class CGameClient *GameClient() const { return m_pClient; }
};
#endif
