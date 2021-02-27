/*
 * ============================================================================
 *
 *  [THC RPG] Total HardCore RPG
 *
 *  File:          regen_ammo.inc
 *  Type:          Upgrade
 *  Description:   Gives you the ability to steadily regain ammo for the active gun until it is fully replenished
 *  Author:        ArsiRC
 *
 *  Copyright (C) 2009-2011  ArsiRC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 */

/**
 * This module's identifier.
 */
new Module:g_moduleregen_ammo;

new g_regen_ammoUpgradeConfigCache[UpgradeConfigs];

new Handle:g_hRegenAmmoTimers[MAXPLAYERS+1] = {INVALID_HANDLE, ...};

/**
 * Register this module.
 */
regen_ammo_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "regen_ammo");
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "RegenerateAmmo");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Gives you the ability to steadily regain ammo for the active gun until it is fully replenished");

    new Module:dependencies[MM_DATA_DEPENDENCIES];
    dependencies[0] = g_moduleCore;
    dependencies[1] = INVALID_MODULE;
    moduledata[ModuleData_Dependencies] = dependencies;

    new Module:interlocks[MM_DATA_INTERLOCKS];
    interlocks[0] = INVALID_MODULE;
    moduledata[ModuleData_Interlocks] = interlocks;

    moduledata[ModuleData_TeamLock] = 0;
    moduledata[ModuleData_MaxLevel] = 5;
    moduledata[ModuleData_Cost] = 5;
    moduledata[ModuleData_iCost] = 10;

    // Send this array of data to the module manager.
    g_moduleregen_ammo = ModuleMgr_Register(moduledata);

    // Now register the events we're going to use.
    #if defined EVENT_MANAGER
        EventMgr_RegisterEvent(g_moduleregen_ammo, "Event_OnEventsRegister", "regen_ammo_OnEventsRegister");
    #endif  
  
    // Register config file(s) that this module will use.
    #if defined CONFIG_MANAGER
        ConfigMgr_Register(g_moduleregen_ammo, "regen_ammo_OnConfigReload", "configs/thc_rpg/regen_ammo.txt");
    #endif
}

/**
 * Register all events here.
 */
public regen_ammo_OnEventsRegister()
{
	// Register all the events needed for this module.
	EventMgr_RegisterEvent(g_moduleregen_ammo, "Event_OnMapStart",           "regen_ammo_OnMapStart");
	EventMgr_RegisterEvent(g_moduleregen_ammo, "Event_OnClientPutInServer",     "regen_ammo_OnClientPutInServer");
	EventMgr_RegisterEvent(g_moduleregen_ammo, "Event_OnPlayerRunCmd",     "regen_ammo_OnPlayerRunCmd");
	EventMgr_RegisterEvent(g_moduleregen_ammo, "Event_OnClientDisconnect",      "regen_ammo_OnClientDisconnect");
	#if defined PROJECT_GAME_ZPS
		EventMgr_RegisterEvent(g_moduleregen_ammo, "Event_PlayerSpawnZPS",          "regen_ammo_PlayerSpawn");
	#endif
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public regen_ammo_OnEventsReady()
{
}

#if defined CONFIG_MANAGER
/**
 * Called when a registered config file (by this module) is manually reloaded.
 */
public regen_ammo_OnConfigReload(configindex)
{
    #if defined LOG_MANAGER
        LogMgr_Print(g_moduleregen_ammo, LogType_Normal, "regen_ammoConfigReload", "Reloaded regen_ammo module's config (index %d)", configindex);
    #endif

    ConfigMgr_CacheKv(g_moduleregen_ammo, CM_CONFIGINDEX_FIRST, "regen_ammoModule_ConfigCache");
}

/**
  * Read config values
  */
public KvCache:regen_ammoModule_ConfigCache(Handle:kv, sectionindex, const String:sectionname[])
{
    // Read Upgrade config
    if(StrEqual(sectionname, "regen_ammo", false))
    {
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_Disable] = KvGetNum(kv, "disable");
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_TeamLock] = KvGetNum(kv, "teamlock");
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_MaxLevel] = KvGetNum(kv, "maxlevel");
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_Cost] = KvGetNum(kv, "cost");
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_iCost] = KvGetNum(kv, "icost");
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_Interval] = KvGetFloat(kv, "interval");
        g_regen_ammoUpgradeConfigCache[UpgradeConfig_Amount] = KvGetFloat(kv, "amount");

        if(g_regen_ammoUpgradeConfigCache[UpgradeConfig_Disable]==1)
            ModuleMgr_Disable(g_moduleregen_ammo);

        ModuleMgr_WriteCell(g_moduleregen_ammo, ModuleData_TeamLock, g_regen_ammoUpgradeConfigCache[UpgradeConfig_TeamLock]);
        ModuleMgr_WriteCell(g_moduleregen_ammo, ModuleData_MaxLevel, g_regen_ammoUpgradeConfigCache[UpgradeConfig_MaxLevel]);
        ModuleMgr_WriteCell(g_moduleregen_ammo, ModuleData_Cost, g_regen_ammoUpgradeConfigCache[UpgradeConfig_Cost]);
        ModuleMgr_WriteCell(g_moduleregen_ammo, ModuleData_iCost, g_regen_ammoUpgradeConfigCache[UpgradeConfig_iCost]);
    }
    else
        ModuleMgr_Disable(g_moduleregen_ammo);
}
#endif

/**
 * The map has started.
 */
public regen_ammo_OnMapStart()
{
	#if defined CONFIG_MANAGER
		ConfigMgr_CacheKv(g_moduleregen_ammo, CM_CONFIGINDEX_FIRST, "regen_ammoModule_ConfigCache");
	#endif
	
	for(new client = 1; client <= MaxClients; client++)
		g_hRegenAmmoTimers[client] = INVALID_HANDLE;
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 * 
 */
public regen_ammo_PlayerSpawn(client)
{
	if(g_hRegenAmmoTimers[client] != INVALID_HANDLE)
	{
		KillTimer(g_hRegenAmmoTimers[client]);
		g_hRegenAmmoTimers[client] = INVALID_HANDLE;
	}
}

public Action:Timer_OnRegen_ammoInterval(Handle:timer, any:client)
{
	if(IsClientInGame(client))
	{
		new weapon = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
		new max_ammo = GetWeaponClipsCount(weapon);
		new ammo_count = GetEntProp(weapon, Prop_Data, "m_iClip1");
		if(max_ammo > ammo_count)
		{
			new amount=ammo_count+RoundToNearest(g_regen_ammoUpgradeConfigCache[UpgradeConfig_Amount]*GetPlayerUpgradeLevel(client,g_moduleregen_ammo));
			if(amount>max_ammo)
			{
				amount=max_ammo;
			}
			SetEntProp(weapon, Prop_Data, "m_iClip1", amount);
			return Plugin_Continue;
		}
		else
		{
			g_hRegenAmmoTimers[client] = INVALID_HANDLE;
			return Plugin_Stop;
		}
	}
	return Plugin_Stop;
}

public regen_ammo_OnClientPutInServer(client)
{
	if(!client)
		return;
	
	if(g_hRegenAmmoTimers[client] != INVALID_HANDLE)
	{
		KillTimer(g_hRegenAmmoTimers[client]);
		g_hRegenAmmoTimers[client] = INVALID_HANDLE;
	}
	
	SDKHook(client, SDKHook_WeaponEquipPost, regen_ammo_WeaponEquipPost);
	SDKHook(client, SDKHook_WeaponDropPost, regen_ammo_WeaponDropPost);
}

public regen_ammo_WeaponEquipPost(client, weapon)
{
	if(g_hRegenAmmoTimers[client] != INVALID_HANDLE)
	{
		KillTimer(g_hRegenAmmoTimers[client]);
		g_hRegenAmmoTimers[client] = INVALID_HANDLE;
	}
	
	if(IsValidPlayer(client))
	{
		if(GetClientTeam(client)!=g_regen_ammoUpgradeConfigCache[UpgradeConfig_TeamLock])
		{
			if(GetPlayerUpgradeLevel(client,g_moduleregen_ammo) && g_hRegenAmmoTimers[client] == INVALID_HANDLE)
			{
				new weapon_ent = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
				new max_ammo = GetWeaponClipsCount(weapon_ent);
				if(max_ammo > 0 && GetEntProp(weapon_ent, Prop_Data, "m_iClip1") < max_ammo)
					g_hRegenAmmoTimers[client] = CreateTimer(g_regen_ammoUpgradeConfigCache[UpgradeConfig_Interval],Timer_OnRegen_ammoInterval,client,TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
			}
		}
	}
}

public regen_ammo_WeaponDropPost(client, weapon)
{
	if(g_hRegenAmmoTimers[client] != INVALID_HANDLE)
	{
		KillTimer(g_hRegenAmmoTimers[client]);
		g_hRegenAmmoTimers[client] = INVALID_HANDLE;
	}
	
	if(IsValidPlayer(client))
	{
		if(GetClientTeam(client)!=g_regen_ammoUpgradeConfigCache[UpgradeConfig_TeamLock])
		{
			if(GetPlayerUpgradeLevel(client,g_moduleregen_ammo) && g_hRegenAmmoTimers[client] == INVALID_HANDLE)
			{
				new weapon_ent = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
				new max_ammo = GetWeaponClipsCount(weapon_ent);
				if(max_ammo > 0 && GetEntProp(weapon_ent, Prop_Data, "m_iClip1") < max_ammo)
				{
					g_hRegenAmmoTimers[client] = CreateTimer(g_regen_ammoUpgradeConfigCache[UpgradeConfig_Interval],Timer_OnRegen_ammoInterval,client,TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
				}
			}
		}
	}
}

public regen_ammo_OnPlayerRunCmd(client, buttons)
{
	if(IsValidPlayer(client) && g_hRegenAmmoTimers[client] == INVALID_HANDLE)
	{
		if(GetClientTeam(client)!=g_regen_ammoUpgradeConfigCache[UpgradeConfig_TeamLock])
		{
			if(GetPlayerUpgradeLevel(client,g_moduleregen_ammo))
			{
				if(buttons & IN_ATTACK)
				{
					new weapon_ent = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
					new max_ammo = GetWeaponClipsCount(weapon_ent);
					if(max_ammo > 0 && GetEntProp(weapon_ent, Prop_Data, "m_iClip1") < max_ammo)
					{
						g_hRegenAmmoTimers[client] = CreateTimer(g_regen_ammoUpgradeConfigCache[UpgradeConfig_Interval],Timer_OnRegen_ammoInterval,client,TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
					}
				}
			}
		}
	}
}

public regen_ammo_OnClientDisconnect(client)
{
	if(g_hRegenAmmoTimers[client] != INVALID_HANDLE)
	{
		KillTimer(g_hRegenAmmoTimers[client]);
		g_hRegenAmmoTimers[client] = INVALID_HANDLE;
	}
}

GetWeaponClipsCount(weapon)
{
	if(weapon > 0 && IsValidEdict(weapon))
	{
		decl String:classname[64];
		GetEdictClassname(weapon, classname, sizeof(classname));
		
		if(StrEqual(classname, "weapon_ppk", false))
		{
			return 7;
		}
		else if(StrEqual(classname, "weapon_glock", false) ||
			StrEqual(classname, "weapon_glock18c", false))
		{
			return 17;
		}
		else if(StrEqual(classname, "weapon_usp", false))
		{
			return 15;
		}
		else if(StrEqual(classname, "weapon_revolver", false))
		{
			return 6;
		}
		else if(StrEqual(classname, "weapon_ak47", false) ||
			StrEqual(classname, "weapon_mp5", false) ||
			StrEqual(classname, "weapon_m4", false))
		{
			return 30;
		}
		else if(StrEqual(classname, "weapon_supershorty", false))
		{
			return 3;
		}
		else if(StrEqual(classname, "weapon_winchester", false))
		{
			return 2;
		}
		else if(StrEqual(classname, "weapon_870", false))
		{
			return 6;
		}
	}
	
	return -1;
}