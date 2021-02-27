new Module:g_moduleinfect;

new g_infectUpgradeConfigCache[UpgradeConfigs];

/**
 * Register this module.
 */
infect_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "infect");
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Infection");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Gives you the ability to infect survivors");

    new Module:dependencies[MM_DATA_DEPENDENCIES];
    dependencies[0] = g_moduleCore;
    dependencies[1] = INVALID_MODULE;
    moduledata[ModuleData_Dependencies] = dependencies;

    new Module:interlocks[MM_DATA_INTERLOCKS];
    interlocks[0] = INVALID_MODULE;
    moduledata[ModuleData_Interlocks] = interlocks;
    
    moduledata[ModuleData_TeamLock] = 0;
    moduledata[ModuleData_MaxLevel] = 5;
    moduledata[ModuleData_Cost] = 20;
    moduledata[ModuleData_iCost] = 15;

    // Send this array of data to the module manager.
    g_moduleinfect = ModuleMgr_Register(moduledata);

    // Now register the events we're going to use.
    #if defined EVENT_MANAGER
        EventMgr_RegisterEvent(g_moduleinfect, "Event_OnEventsRegister", "infect_OnEventsRegister");
    #endif
    
    // Register config file(s) that this module will use.
    #if defined CONFIG_MANAGER
        ConfigMgr_Register(g_moduleinfect, "infect_OnConfigReload", "configs/thc_rpg/infect.txt");
    #endif
}

/**
 * Register all events here.
 */
public infect_OnEventsRegister()
{
	// Register all the events needed for this module.
	EventMgr_RegisterEvent(g_moduleinfect, "Event_OnMapStart",             "infect_OnMapStart");
	EventMgr_RegisterEvent(g_moduleinfect, "Event_OnClientPutInServer",     "infect_OnClientPutInServer");
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public infect_OnEventsReady()
{
}

#if defined CONFIG_MANAGER
/**
 * Called when a registered config file (by this module) is manually reloaded.
 */
public infect_OnConfigReload(configindex)
{
    #if defined LOG_MANAGER
        LogMgr_Print(g_moduleinfect, LogType_Normal, "infectConfigReload", "Reloaded infect module's config (index %d)", configindex);
    #endif

    ConfigMgr_CacheKv(g_moduleinfect, CM_CONFIGINDEX_FIRST, "infectModule_ConfigCache");
}

/**
  * Read config values
  */
public KvCache:infectModule_ConfigCache(Handle:kv, sectionindex, const String:sectionname[])
{
    // Read Upgrade config
    if(StrEqual(sectionname, "infect", false))
    {
        g_infectUpgradeConfigCache[UpgradeConfig_Disable] = KvGetNum(kv, "disable");
        g_infectUpgradeConfigCache[UpgradeConfig_TeamLock] = KvGetNum(kv, "teamlock");
        g_infectUpgradeConfigCache[UpgradeConfig_Effects] = KvGetNum(kv, "effects");
        g_infectUpgradeConfigCache[UpgradeConfig_MaxLevel] = KvGetNum(kv, "maxlevel");
        g_infectUpgradeConfigCache[UpgradeConfig_Cost] = KvGetNum(kv, "cost");
        g_infectUpgradeConfigCache[UpgradeConfig_iCost] = KvGetNum(kv, "icost");
        g_infectUpgradeConfigCache[UpgradeConfig_Percent] = KvGetFloat(kv, "percent");

        if(g_infectUpgradeConfigCache[UpgradeConfig_Disable]==1)
            ModuleMgr_Disable(g_moduleinfect);

        ModuleMgr_WriteCell(g_moduleinfect, ModuleData_TeamLock, g_infectUpgradeConfigCache[UpgradeConfig_TeamLock]);
        ModuleMgr_WriteCell(g_moduleinfect, ModuleData_MaxLevel, g_infectUpgradeConfigCache[UpgradeConfig_MaxLevel]);
        ModuleMgr_WriteCell(g_moduleinfect, ModuleData_Cost, g_infectUpgradeConfigCache[UpgradeConfig_Cost]);
        ModuleMgr_WriteCell(g_moduleinfect, ModuleData_iCost, g_infectUpgradeConfigCache[UpgradeConfig_iCost]);
    }
    else
        ModuleMgr_Disable(g_moduleinfect);
}
#endif

/**
 * The map has started.
 */
public infect_OnMapStart()
{
    #if defined CONFIG_MANAGER
        ConfigMgr_CacheKv(g_moduleinfect, CM_CONFIGINDEX_FIRST, "infectModule_ConfigCache");
    #endif
}

public infect_OnClientPutInServer(client)
{
	if(!client || IsFakeClient(client))
		return;
	
	SDKHook(client, SDKHook_OnTakeDamage, infect_OnTakeDamage);
}

public Action:infect_OnTakeDamage(victim, &attacker, &inflictor, &Float:damage, &damagetype)
{
	if(!(attacker > 0 && attacker <= MaxClients))
		return Plugin_Continue;
	
	if(AreValidPlayers(victim,attacker))
	{
		if(GetClientTeam(attacker) == 3 && IsArmsWeapon(GetEntPropEnt(attacker, Prop_Data, "m_hActiveWeapon")) && GetClientTeam(victim) == 2)
		{
			new level = GetPlayerUpgradeLevel(attacker, g_moduleinfect);
			if(level > 0)
			{
				if(GetRandomInt(0, 100) <= RoundToNearest(100*g_infectUpgradeConfigCache[UpgradeConfig_Percent]*level))
				{
					InfectPlayer(victim, 85.0);
				}
			}
		}
	}
	
	return Plugin_Continue;
}

bool:IsArmsWeapon(weapon)
{
	if(weapon > 0 && IsValidEdict(weapon))
	{
		decl String:classname[64];
		GetEdictClassname(weapon, classname, sizeof(classname));
		
		if(StrEqual(classname, "weapon_arms", false) || StrEqual(classname, "weapon_carrierarms", false))
			return true;
	}
	
	return false;
}

InfectPlayer(client, Float:fTime)
{
	new Float:fTurnTime = GetGameTime() + fTime;
	SetEntDataFloat(client, FindDataMapOffs(client, "m_tbiPrev"), fTurnTime, true);
	SetEntData(client, FindSendPropOffs("CHL2MP_Player", "m_IsInfected"), 1);
}