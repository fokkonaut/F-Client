/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <base/math.h>
#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include "camera.h"
#include "controls.h"

#include <engine/serverbrowser.h>

const float ZoomStep = 0.866025f;

CCamera::CCamera()
{
	m_CamType = CAMTYPE_UNDEFINED;
	m_RotationCenter = vec2(0.0f, 0.0f);
	m_AnimationStartPos = vec2(0.0f, 0.0f);
	m_Center = vec2(0.0f, 0.0f);
	m_PrevCenter = vec2(0.0f, 0.0f);
	m_MenuCenter = vec2(0.0f, 0.0f);

	m_Positions[POS_START] = vec2(500.0f, 500.0f);
	m_Positions[POS_INTERNET] = vec2(1000.0f, 1000.0f);
	m_Positions[POS_LAN] = vec2(1100.0f, 1000.0f);
	m_Positions[POS_DEMOS] = vec2(1500.0f, 500.0f);
	m_Positions[POS_SETTINGS_GENERAL] = vec2(500.0f, 1000.0f);
	m_Positions[POS_SETTINGS_PLAYER] = vec2(600.0f, 1000.0f);
	m_Positions[POS_SETTINGS_TBD] = vec2(700.0f, 1000.0f);
	m_Positions[POS_SETTINGS_CONTROLS] = vec2(800.0f, 1000.0f);
	m_Positions[POS_SETTINGS_GRAPHICS] = vec2(900.0f, 1000.0f);
	m_Positions[POS_SETTINGS_SOUND] = vec2(1000.0f, 1000.0f);
	m_Positions[POS_SETTINGS_FCLIENT] = vec2(1100.0f, 1000.0f);

	m_CurrentPosition = -1;
	m_MoveTime = 0.0f;

	m_ZoomSet = false;
	m_Zooming = false;
}

float CCamera::ZoomProgress(float CurrentTime) const
{
	return (CurrentTime - m_ZoomSmoothingStart) / (m_ZoomSmoothingEnd - m_ZoomSmoothingStart);
}

void CCamera::ScaleZoom(float Factor)
{
	float CurrentTarget = m_Zooming ? m_ZoomSmoothingTarget : m_Zoom;
	ChangeZoom(CurrentTarget * Factor);
}

void CCamera::ChangeZoom(float Target)
{
	if(Target >= 30)
	{
		return;
	}

	float Now = Client()->LocalTime();
	float Current = m_Zoom;
	float Derivative = 0.0f;
	if(m_Zooming)
	{
		float Progress = ZoomProgress(Now);
		Current = m_ZoomSmoothing.Evaluate(Progress);
		Derivative = m_ZoomSmoothing.Derivative(Progress);
	}

	m_ZoomSmoothingTarget = Target;
	m_ZoomSmoothing = CCubicBezier::With(Current, Derivative, 0, m_ZoomSmoothingTarget);
	m_ZoomSmoothingStart = Now;
	m_ZoomSmoothingEnd = Now + (float)Config()->m_ClSmoothZoomTime / 1000;

	m_Zooming = true;
}

void CCamera::OnRender()
{
	if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		if(m_Zooming)
		{
			float Time = Client()->LocalTime();
			if(Time >= m_ZoomSmoothingEnd)
			{
				m_Zoom = m_ZoomSmoothingTarget;
				m_Zooming = false;
			}
			else
			{
				m_Zoom = m_ZoomSmoothing.Evaluate(ZoomProgress(Time));
			}
		}

		CServerInfo Info;
		Client()->GetServerInfo(&Info);
		if(!(m_pClient->m_Snap.m_SpecInfo.m_Active || IsRace(&Info) || Client()->State() == IClient::STATE_DEMOPLAYBACK))
		{
			m_ZoomSet = false;
			m_Zoom = 1.0f;
			m_Zooming = false;
		}
		else if(!m_ZoomSet)
		{
			m_ZoomSet = true;
			OnReset();
		}

		// update camera center
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition &&
			(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FREEVIEW || (m_pClient->m_Snap.m_pLocalInfo && !(m_pClient->m_Snap.m_pLocalInfo->m_PlayerFlags&PLAYERFLAG_DEAD))))
		{
			if(m_CamType != CAMTYPE_SPEC)
			{
				m_pClient->m_pControls->m_MousePos[Config()->m_ClDummy] = m_PrevCenter;
				m_pClient->m_pControls->ClampMousePos();
				m_CamType = CAMTYPE_SPEC;
			}
			m_Center = m_pClient->m_pControls->m_MousePos[Config()->m_ClDummy];
		}
		else
		{
			if(m_CamType != CAMTYPE_PLAYER)
			{
				m_pClient->m_pControls->ClampMousePos();
				m_CamType = CAMTYPE_PLAYER;
			}

			vec2 CameraOffset(0, 0);

			float l = length(m_pClient->m_pControls->m_MousePos[Config()->m_ClDummy]);
			if(Config()->m_ClDynamicCamera && l > 0.0001f) // make sure that this isn't 0
			{
				float DeadZone = Config()->m_ClMouseDeadzone;
				float FollowFactor = Config()->m_ClMouseFollowfactor/100.0f;
				float OffsetAmount = max(l-DeadZone, 0.0f) * FollowFactor;

				CameraOffset = normalize(m_pClient->m_pControls->m_MousePos[Config()->m_ClDummy])*OffsetAmount;
			}

			if(m_pClient->m_Snap.m_SpecInfo.m_Active)
				m_Center = m_pClient->m_Snap.m_SpecInfo.m_Position + CameraOffset;
			else
				m_Center = m_pClient->m_LocalCharacterPos + CameraOffset;
		}
	}
	else
	{
		m_Zoom = 0.7f;

		static vec2 Dir = vec2(1.0f, 0.0f);

		if(distance(m_Center, m_RotationCenter) <= (float)Config()->m_ClRotationRadius+0.5f)
		{
			// do little rotation
			float RotPerTick = 360.0f/(float)Config()->m_ClRotationSpeed * Client()->RenderFrameTime();
			Dir = rotate(Dir, RotPerTick);
			m_Center = m_RotationCenter+Dir*(float)Config()->m_ClRotationRadius;
		}
		else
		{
			// positions for the animation
			Dir = normalize(m_AnimationStartPos - m_RotationCenter);
			vec2 TargetPos = m_RotationCenter + Dir * (float)Config()->m_ClRotationRadius;
			float Distance = distance(m_AnimationStartPos, TargetPos);

			// move time
			m_MoveTime += Client()->RenderFrameTime()*Config()->m_ClCameraSpeed / 10.0f;
			float XVal = 1 - m_MoveTime;
			XVal = pow(XVal, 7.0f);

			m_Center = TargetPos + Dir * (XVal*Distance);
		}
	}

	m_PrevCenter = m_Center;
}

void CCamera::ChangePosition(int PositionNumber)
{
	if(PositionNumber < 0 || PositionNumber > NUM_POS-1)
		return;
	m_AnimationStartPos = m_Center;
	m_RotationCenter = m_Positions[PositionNumber];
	m_CurrentPosition = PositionNumber;
	m_MoveTime = 0.0f;
}

int CCamera::GetCurrentPosition()
{
	return m_CurrentPosition;
}

void CCamera::ConSetPosition(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	int PositionNumber = clamp(pResult->GetInteger(0), 0, NUM_POS-1);
	vec2 Position = vec2(pResult->GetInteger(1)*32.0f+16.0f, pResult->GetInteger(2)*32.0f+16.0f);
	pSelf->m_Positions[PositionNumber] = Position;

	// update
	if(pSelf->GetCurrentPosition() == PositionNumber)
		pSelf->ChangePosition(PositionNumber);
}

void CCamera::OnConsoleInit()
{
	Console()->Register("set_position", "iii", CFGFLAG_CLIENT, ConSetPosition, this, "Sets the rotation position");

	Console()->Register("zoom+", "", CFGFLAG_CLIENT, ConZoomPlus, this, "Zoom increase");
	Console()->Register("zoom-", "", CFGFLAG_CLIENT, ConZoomMinus, this, "Zoom decrease");
	Console()->Register("zoom", "", CFGFLAG_CLIENT, ConZoomReset, this, "Zoom reset");
}

void CCamera::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_OFFLINE)
		m_MenuCenter = m_Center;
	else if(NewState != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		m_Center = m_MenuCenter;

	if (NewState == IClient::STATE_ONLINE)
		m_ZoomSet = false;
}

void CCamera::OnReset()
{
	m_Zoom = pow(ZoomStep, Config()->m_ClDefaultZoom - 10);
	m_Zooming = false;
}

void CCamera::ConZoomPlus(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	CServerInfo Info;
	pSelf->Client()->GetServerInfo(&Info);
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || IsRace(&Info) || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		pSelf->ScaleZoom(ZoomStep);
}
void CCamera::ConZoomMinus(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	CServerInfo Info;
	pSelf->Client()->GetServerInfo(&Info);
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || IsRace(&Info) || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		pSelf->ScaleZoom(1 / ZoomStep);
}
void CCamera::ConZoomReset(IConsole::IResult *pResult, void *pUserData)
{
	((CCamera *)pUserData)->ChangeZoom(pow(ZoomStep, ((CCamera *)pUserData)->Config()->m_ClDefaultZoom - 10));
}
