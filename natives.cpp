/*
 * Half-Life Weapon Mod
 * Copyright (c) 2012 AGHL.RU Dev Team
 *
 *
 *    This program is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include "weaponmod.h"

enum e_Offsets
{
	// Weapon
	Offset_flStartThrow,
	Offset_flReleaseThrow,
	Offset_iChargeReady,
	Offset_iInAttack,
	Offset_iFireState,
	Offset_iFireOnEmpty,				// true when the gun is empty and the player is still holding down the attack key(s)
	Offset_flPumpTime,
	Offset_iInSpecialReload,			// Are we in the middle of a reload for the shotguns
	Offset_flNextPrimaryAttack,			// soonest time ItemPostFrame will call PrimaryAttack
	Offset_flNextSecondaryAttack,		// soonest time ItemPostFrame will call SecondaryAttack
	Offset_flTimeWeaponIdle,			// soonest time ItemPostFrame will call WeaponIdle
	Offset_iPrimaryAmmoType,			// "primary" ammo index into players m_rgAmmo[]
	Offset_iSecondaryAmmoType,			// "secondary" ammo index into players m_rgAmmo[]
	Offset_iClip,						// number of shots left in the primary weapon clip, -1 it not used
	Offset_iInReload,					// are we in the middle of a reload;
	Offset_iDefaultAmmo,				// how much ammo you get when you pick up this weapon as placed by a level designer.
	
	// Player
	Offset_flNextAttack,				// cannot attack again until this time
	Offset_iWeaponVolume,				// how loud the player's weapon is right now
	Offset_iWeaponFlash,				// brightness of the weapon flash
	
	Offset_End
};

int PvDataOffsets[Offset_End] =
{
	m_flStartThrow,
	m_flReleaseThrow,
	m_chargeReady,
	m_fInAttack,
	m_fireState,
	m_fFireOnEmpty,
	m_flPumpTime,
	m_fInSpecialReload,
	m_flNextPrimaryAttack,
	m_flNextSecondaryAttack,
	m_flTimeWeaponIdle,
	m_iPrimaryAmmoType,
	m_iSecondaryAmmoType,
	m_iClip,
	m_fInReload,
	m_iDefaultAmmo,
	m_flNextAttack,
	m_iWeaponVolume,
	m_iWeaponFlash
};


#define CHECK_ENTITY(x) \
	if (x != 0 && (FNullEnt(INDEXENT2(x)) || x < 0 || x > gpGlobals->maxEntities)) \
	{ \
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity."); \
		return 0; \
	}\

#define CHECK_OFFSET(x) \
	if ( x < 0 || x >= Offset_End) \
	{ \
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d  Max: %d.", x, Offset_End - 1); \
		return 0; \
	}\

#define CHECK_PARAMS(x) \
	cell count = params[0] / sizeof(cell); \
	if (count != x) \
	{ \
		MF_LogError(amx, AMX_ERR_NATIVE, "Expected %d parameters, got %d.", x, count); \
		return 0; \
	}\


static cell AMX_NATIVE_CALL wpnmod_register_weapon(AMX *amx, cell *params)
{
	CHECK_PARAMS(10)

	if (g_iWeaponIndex >= MAX_WEAPONS)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Weapon limit reached.");
		return 0;
	}

	g_iWeaponIndex++;

	WeaponInfoArray[g_iWeaponIndex].ItemData.pszName = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[1], 0, NULL)));
	WeaponInfoArray[g_iWeaponIndex].ItemData.iSlot = params[2] - 1;
	WeaponInfoArray[g_iWeaponIndex].ItemData.iPosition = params[3] - 1;
	WeaponInfoArray[g_iWeaponIndex].ItemData.pszAmmo1 = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[4], 0, NULL)));
	WeaponInfoArray[g_iWeaponIndex].ItemData.iMaxAmmo1 = params[5];
	WeaponInfoArray[g_iWeaponIndex].ItemData.pszAmmo2 = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[6], 0, NULL)));
	WeaponInfoArray[g_iWeaponIndex].ItemData.iMaxAmmo2 = params[7];
	WeaponInfoArray[g_iWeaponIndex].ItemData.iMaxClip = params[8];
	WeaponInfoArray[g_iWeaponIndex].ItemData.iFlags = params[9];
	WeaponInfoArray[g_iWeaponIndex].ItemData.iWeight = params[10];

	if (!g_HooksEnabled)
	{
		g_HooksEnabled = TRUE;
		MakeVirtualHooks();
	}

	g_InitWeapon = TRUE;

	FN_PrecacheOtherWeapon PrecacheOtherWeapon = (FN_PrecacheOtherWeapon)((DWORD)pDbase + ADDRESS_PRECAHE_OTHER_WEAPON);
	PrecacheOtherWeapon("weapon_crowbar");

	return g_iWeaponIndex;
}

static cell AMX_NATIVE_CALL wpnmod_register_forward(AMX *amx, cell *params)
{
	int iId = params[1];

	if (iId < LIMITER_WEAPON || iId >= MAX_WEAPONS)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id provided. Got: %d  Valid: 16 up to %d.", iId, MAX_WEAPONS - 1);
		return 0;
	}

	int Fwd = params[2];

	if (Fwd < 0 || Fwd >= Fwd_End)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d  Max: %d.", iId, Fwd_End - 1);
		return 0;
	}

	const char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

	WeaponInfoArray[iId].iForward[Fwd] = MF_RegisterSPForwardByName
	(
		amx, 
		funcname, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_DONE
	);

	if (WeaponInfoArray[iId].iForward[Fwd] == -1)
	{
		WeaponInfoArray[iId].iForward[Fwd] = 0;
		MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (%d, \"%s\").", Fwd, funcname);
		return 0;
	}

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_send_weapon_anim(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iAnim = params[2];

	CHECK_ENTITY(iEntity)

	edict_t *pWeapon = INDEXENT(iEntity);
	edict_t *pPlayer = GetPrivateCbase(pWeapon, m_pPlayer);

	if (!IsValidPev(pPlayer))
	{
		return 0;
	}

	pPlayer->v.weaponanim = iAnim;

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, pPlayer);
		WRITE_BYTE(iAnim);
		WRITE_BYTE(pWeapon->v.body);
	MESSAGE_END();

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_set_player_anim(AMX *amx, cell *params)
{
	int iPlayer = params[1];
	int iPlayerAnim = params[2];

	CHECK_ENTITY(iPlayer)

	edict_t *pPlayer = INDEXENT2(iPlayer);

	FN_SetAnimation SetAnimation = (FN_SetAnimation)((DWORD)pDbase + ADDRESS_SET_ANIMATION);

#ifdef _WIN32
	reinterpret_cast<void (__fastcall *)(void *, int, int)>(SetAnimation)((void*)pPlayer->pvPrivateData, 0, iPlayerAnim);
#elif __linux__
	reinterpret_cast<void (*)(void *, int)>(SetAnimation)((void*)pPlayer->pvPrivateData, PLAYER_ATTACK1);
#endif

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_set_offset_int(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];
	int iValue = params[3];

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)
	
	*((int *)INDEXENT2(iEntity)->pvPrivateData + PvDataOffsets[iOffset]) = iValue;
	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_set_offset_float(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];

	float flValue = amx_ctof(params[3]);

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)
	
	*((float *)INDEXENT2(iEntity)->pvPrivateData + PvDataOffsets[iOffset]) = flValue;
	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_get_offset_int(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)

	return *((int *)INDEXENT2(iEntity)->pvPrivateData + PvDataOffsets[iOffset]);
}

static cell AMX_NATIVE_CALL wpnmod_get_offset_float(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)

	return amx_ftoc(*((float *)INDEXENT2(iEntity)->pvPrivateData + PvDataOffsets[iOffset]));
}

static cell AMX_NATIVE_CALL wpnmod_default_deploy(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iAnim = params[4];

	const char *szViewModel = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))); 
	const char *szWeaponModel = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[3], 0, NULL)));
	const char *szAnimExt = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[5], 0, NULL)));

	CHECK_ENTITY(iEntity)

	edict_t* pItem = INDEXENT2(iEntity);
	edict_t* pPlayer = GetPrivateCbase(pItem, m_pPlayer);

	if (!IsValidPev(pPlayer))
	{
		return 0;
	}
	/*
	if (!CanDeploy( ))
	{
		return 0;
	}*/

	pPlayer->v.viewmodel = MAKE_STRING(szViewModel);
	pPlayer->v.weaponmodel = MAKE_STRING(szWeaponModel);

	char *szData = (char *)pPlayer->pvPrivateData + m_szAnimExtention * 4;
		
	if (!IsBadWritePtr(szData, 1))
	{
		strcpy(szData, szAnimExt);
	}

	pPlayer->v.weaponanim = iAnim;

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, pPlayer);
		WRITE_BYTE(iAnim);
		WRITE_BYTE(pItem->v.body);
	MESSAGE_END();

	*((float *)pPlayer->pvPrivateData + m_flNextAttack) = 0.5;
	*((float *)pItem->pvPrivateData + m_flTimeWeaponIdle) = 1.0;

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_default_reload(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iClipSize = params[2];
	int iAnim = params[3];

	float flDelay = amx_ctof(params[4]);

	CHECK_ENTITY(iEntity)

	edict_t* pItem = INDEXENT2(iEntity);
	edict_t* pPlayer = GetPrivateCbase(pItem, m_pPlayer);

	if (!IsValidPev(pPlayer))
	{
		return 0;
	}

	int iAmmo = Player_AmmoInventory(pPlayer, pItem, TRUE);

	if (!iAmmo)
	{
		return 0;
	}

	int j = min(iClipSize - *((int *)pItem->pvPrivateData + m_iClip), iAmmo);

	if (!j)
	{
		return 0;
	}

	*((int *)pItem->pvPrivateData + m_fInReload) = TRUE;	
	*((float *)pPlayer->pvPrivateData + m_flNextAttack) = flDelay;
	*((float *)pItem->pvPrivateData + m_flTimeWeaponIdle) = flDelay;

	pPlayer->v.weaponanim = iAnim;

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, pPlayer);
		WRITE_BYTE(iAnim);
		WRITE_BYTE(pItem->v.body);
	MESSAGE_END();

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_set_think(AMX *amx, cell *params)
{
	int iEntity = params[1];

	CHECK_ENTITY(iEntity)

	char *funcname = MF_GetAmxString(amx, params[2], 0, NULL);

	int iForward = MF_RegisterSPForwardByName
	(
		amx, 
		funcname, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_DONE
	);

	if (iForward == -1)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (\"%s\").", funcname);
		return 0;
	}

	g_EntData.Set_Think(iEntity, iForward);

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_fire_bullets(AMX *amx, cell *params)
{
	int iPlayer = params[1];
	int iAttacker = params[2];

	CHECK_ENTITY(iPlayer)
	CHECK_ENTITY(iAttacker)

	Vector vecSpread;

	cell *vSpread = MF_GetAmxAddr(amx, params[4]);

	vecSpread.x = amx_ctof(vSpread[0]);
	vecSpread.y = amx_ctof(vSpread[1]);
	vecSpread.z = amx_ctof(vSpread[2]);

	FireBulletsPlayer(INDEXENT2(iPlayer), INDEXENT2(iAttacker), params[3], vecSpread, amx_ctof(params[5]), amx_ctof(params[6]), params[7], params[8]);

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_radius_damage(AMX *amx, cell *params)
{
	Vector vecSrc;
	cell *vSrc = MF_GetAmxAddr(amx, params[1]);

	vecSrc.x = amx_ctof(vSrc[0]);
	vecSrc.y = amx_ctof(vSrc[1]);
	vecSrc.z = amx_ctof(vSrc[2]);

	int iInflictor = params[2];
	int iAttacker = params[3];

	CHECK_ENTITY(iInflictor)
	CHECK_ENTITY(iAttacker)

	FN_RadiusDamage RadiusDamage = (FN_RadiusDamage)((DWORD)pDbase + ADDRESS_RADIUS_DAMAGE);
	RadiusDamage(vecSrc, &INDEXENT2(iInflictor)->v, &INDEXENT2(iAttacker)->v, amx_ctof(params[4]), amx_ctof(params[5]), params[6], params[7]);

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_eject_brass(AMX *amx, cell *params)
{
	int iPlayer = params[1];
	int iShell = params[2];
	int iSoundType = params[3];

	CHECK_ENTITY(iPlayer)

	float flFwdScale = amx_ctof(params[4]);
	float flUpScale = amx_ctof(params[5]);
	float flRightScale = amx_ctof(params[6]);
	
	Vector vecViewOfs;
	Vector vecShellVelocity;
	Vector vecShellOrigin;

	float fR = RANDOM_FLOAT(50, 70);
	float fU = RANDOM_FLOAT(100, 150);

	Vector vecUp = gpGlobals->v_up;
	Vector vecRight = gpGlobals->v_right;
	Vector vecForward = gpGlobals->v_forward;
	
	edict_t* pPlayer = INDEXENT2(iPlayer);

	vecViewOfs.z = FBitSet(pPlayer->v.flags, FL_DUCKING) ? 12 : DEFAULT_VIEWHEIGHT;

	for (int i = 0; i < 3; i++ )
	{
		vecShellVelocity[i] = pPlayer->v.velocity[i] + vecRight[i] * fR + vecUp[i] * fU + vecForward[i] * 25;
		vecShellOrigin[i]   = pPlayer->v.origin[i] + vecViewOfs[i] + vecUp[i] * flUpScale + vecForward[i] * flFwdScale + vecRight[i] * flRightScale;
	}

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecShellOrigin );
		WRITE_BYTE( TE_MODEL);
		WRITE_COORD( vecShellOrigin.x );
		WRITE_COORD( vecShellOrigin.y );
		WRITE_COORD( vecShellOrigin.z );
		WRITE_COORD( vecShellVelocity.x );
		WRITE_COORD( vecShellVelocity.y );
		WRITE_COORD( vecShellVelocity.z );
		WRITE_ANGLE( pPlayer->v.angles.z );
		WRITE_SHORT( iShell );
		WRITE_BYTE ( iSoundType );
		WRITE_BYTE ( 25 );// 2.5 seconds
	MESSAGE_END();

	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_reset_empty_sound(AMX *amx, cell *params)
{
	int iEntity = params[1];

	CHECK_ENTITY(iEntity)

	*((int *)INDEXENT2(iEntity)->pvPrivateData + m_iPlayEmptySound) = 1;
	return 1;
}

static cell AMX_NATIVE_CALL wpnmod_play_empty_sound(AMX *amx, cell *params)
{
	int iEntity = params[1];

	CHECK_ENTITY(iEntity)

	if (*((int *)INDEXENT2(iEntity)->pvPrivateData + m_iPlayEmptySound))
	{
		edict_t* pPlayer = GetPrivateCbase(INDEXENT2(iEntity), m_pPlayer);

		if (IsValidPev(pPlayer))
		{
			EMIT_SOUND_DYN2(pPlayer, CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM, 0, PITCH_NORM);
			*((int *)INDEXENT2(iEntity)->pvPrivateData + m_iPlayEmptySound) = 0;
			
			return 1;
		}
	}

	return 0;
}

static cell AMX_NATIVE_CALL wpnmod_spawn_weapon_by_id(AMX *amx, cell *params)
{
	int iId = params[1];

	if (iId < LIMITER_WEAPON || iId >= MAX_WEAPONS)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id provided. Got: %d  Valid: 16 up to %d.", iId, MAX_WEAPONS - 1);
		return 0;
	}

	Vector vecOrigin;
	cell *vOrigin = MF_GetAmxAddr(amx, params[2]);

	vecOrigin.x = amx_ctof(vOrigin[0]);
	vecOrigin.y = amx_ctof(vOrigin[1]);
	vecOrigin.z = amx_ctof(vOrigin[2]);

	Vector vecAngles;
	cell *vAngles = MF_GetAmxAddr(amx, params[3]);

	vecAngles.x = amx_ctof(vAngles[0]);
	vecAngles.y = amx_ctof(vAngles[1]);
	vecAngles.z = amx_ctof(vAngles[2]);

	edict_t* iItem = Weapon_Spawn(iId, vecOrigin, vecAngles);

	if (IsValidPev(iItem))
	{
		return ENTINDEX(iItem);
	}

	return 0;
}

static cell AMX_NATIVE_CALL wpnmod_spawn_weapon_by_name(AMX *amx, cell *params)
{
	char *wpnname = MF_GetAmxString(amx, params[1], 0, NULL);

	Vector vecOrigin;
	cell *vOrigin = MF_GetAmxAddr(amx, params[2]);

	vecOrigin.x = amx_ctof(vOrigin[0]);
	vecOrigin.y = amx_ctof(vOrigin[1]);
	vecOrigin.z = amx_ctof(vOrigin[2]);

	Vector vecAngles;
	cell *vAngles = MF_GetAmxAddr(amx, params[3]);

	vecAngles.x = amx_ctof(vAngles[0]);
	vecAngles.y = amx_ctof(vAngles[1]);
	vecAngles.z = amx_ctof(vAngles[2]);

	for (int i = LIMITER_WEAPON + 1; i <= g_iWeaponIndex; i++)
	{
		if (!strcmpi(pszName(i), wpnname))
		{
			edict_t* iItem = Weapon_Spawn(i, vecOrigin, vecAngles);

			if (IsValidPev(iItem))
			{
				return ENTINDEX(iItem);
			}
		}
	}

	return 0;
}

AMX_NATIVE_INFO wpnmod_Natives[] = 
{
	{ "wpnmod_register_weapon", wpnmod_register_weapon},
	{ "wpnmod_register_forward", wpnmod_register_forward},
	{ "wpnmod_send_weapon_anim", wpnmod_send_weapon_anim},
	{ "wpnmod_set_player_anim", wpnmod_set_player_anim},
	{ "wpnmod_set_think", wpnmod_set_think},
	{ "wpnmod_set_offset_int", wpnmod_set_offset_int},
	{ "wpnmod_set_offset_float", wpnmod_set_offset_float},
	{ "wpnmod_get_offset_int", wpnmod_get_offset_int},
	{ "wpnmod_get_offset_float", wpnmod_get_offset_float},
	{ "wpnmod_default_deploy", wpnmod_default_deploy},
	{ "wpnmod_default_reload", wpnmod_default_reload},
	{ "wpnmod_fire_bullets", wpnmod_fire_bullets},
	{ "wpnmod_radius_damage", wpnmod_radius_damage},
	{ "wpnmod_eject_brass", wpnmod_eject_brass},
	{ "wpnmod_reset_empty_sound", wpnmod_reset_empty_sound},
	{ "wpnmod_play_empty_sound", wpnmod_play_empty_sound},
	{ "wpnmod_spawn_weapon_by_id", wpnmod_spawn_weapon_by_id},
	{ "wpnmod_spawn_weapon_by_name", wpnmod_spawn_weapon_by_name},

	{ NULL, NULL }
};