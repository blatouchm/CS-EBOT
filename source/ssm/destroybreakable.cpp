#include "../../include/core.h"
ConVar ebot_kill_breakables("ebot_kill_breakables", "0");

void Bot::DestroyBreakableStart(void)
{
	m_breakableAttackStep = 0;
	m_breakableAttackTime = 0.0f;
	SelectBestWeapon();
}

void Bot::DestroyBreakableUpdate(void)
{
	if (!DestroyBreakableReq())
	{
		FinishCurrentProcess("sucsessfully destroyed a breakable");
		return;
	}

	if (ebot_kill_breakables.GetBool())
		m_breakableEntity->v.health = -1.0f;

	const float time = engine->GetTime();
	Vector lookTarget = m_breakableOrigin;
	if (m_breakableAttackStep >= 3)
	{
		Vector directionToGoal = m_destOrigin - pev->origin;
		if (directionToGoal.GetLengthSquared2D() > squaredf(1.0f))
		{
			directionToGoal = directionToGoal.Normalize2D();
			lookTarget = EyePosition() + directionToGoal * 256.0f;
			lookTarget.z = EyePosition().z;
		}
	}

	LookAt(lookTarget);
	if (pev->origin.z > m_breakableOrigin.z)
		m_duckTime = time + 1.0f;
	else if (!IsVisible(m_breakableOrigin, m_myself))
		m_duckTime = time + 1.0f;

	if (!m_isZombieBot && m_currentWeapon != Weapon::Knife)
		FireWeapon((pev->origin - m_breakableOrigin).GetLengthSquared());
	else
	{
		MoveTo(m_breakableOrigin, true);
		SelectBestWeapon();

		if (time >= m_breakableAttackTime)
		{
			m_buttons |= IN_ATTACK;
			m_breakableAttackStep = (m_breakableAttackStep + 1) % 6;
			m_breakableAttackTime = time + 0.22f;
		}
		else
			m_buttons &= ~IN_ATTACK;
	}

	IgnoreCollisionShortly();
	m_pauseTime = time + crandomfloat(2.0f, 7.0f);
}

void Bot::DestroyBreakableEnd(void)
{
	if (!FNullEnt(m_breakableEntity) && m_breakableEntity->v.health > 0.0f)
		m_ignoreEntity = m_breakableEntity;

	m_breakableEntity = nullptr;
	m_breakableAttackStep = 0;
	m_breakableAttackTime = 0.0f;
}

bool Bot::DestroyBreakableReq(void)
{
	if (FNullEnt(m_breakableEntity))
		return false;

	if (m_breakableEntity->v.health <= 0.0f)
		return false;

	if (m_breakableEntity->v.takedamage == DAMAGE_NO)
		return false;

	if (m_ignoreEntity == m_breakableEntity)
		return false;

	return true;
}
