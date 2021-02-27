#pragma semicolon 1

#include <sourcemod>
#include <shop>

#include <colors>

#define PLUGIN_VERSION	"1.3.2"

new Handle:g_hInterval;
new Handle:g_hMoneyPerTick;
new Handle:h_timer[MAXPLAYERS+1];

new bool:bRoundEnd, bool:bStopRoundEnd;

new Handle:kv;

public Plugin:myinfo =
{
    name        = "[Shop] Money Distributor",
    author      = "FrozDark (HLModders LLC)",
    description = "Money Distributor component for [Shop]",
    version     = PLUGIN_VERSION,
    url         = "www.hlmod.ru"
};

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	MarkNativeAsOptional("GetUserMessageType");
}

public OnPluginStart() 
{
	CreateConVar("sm_shop_credits_version", PLUGIN_VERSION, "Money distributor version.", FCVAR_PLUGIN|FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_CHEAT|FCVAR_DONTRECORD);
	
	g_hInterval = CreateConVar("sm_shop_credits_interval", "60.0", "The interval of timer.", 0, true, 5.0);
	g_hMoneyPerTick = CreateConVar("sm_shop_credits_amount", "5", "Amount of credits all players get every time.", 0, true, 1.0);
	
	HookConVarChange(CreateConVar("sm_shop_credits_stop_events_on_round_end", "1", "Don't listen to events on round end", 0, true, 0.0, true, 1.0), OnEventListenChange);
	
	HookConVarChange(g_hInterval, OnIntervalChange);
	
	AutoExecConfig(true, "shop_moneydistributor", "shop");
	
	HookEvent("player_team", OnPlayerTeam);
	
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i) || !(1 < GetClientTeam(i) < 4)) continue;
		OnClientDisconnect_Post(i);
		h_timer[i] = CreateTimer(60.0, GivePoints, i, TIMER_REPEAT);
	}
	
	RegAdminCmd("shop_money_reload", Command_Reload, ADMFLAG_ROOT);
	Command_Reload(0,0);
	
	if (HookEventEx("round_end", OnRoundStartEnd))
	{
		HookEvent("round_start", OnRoundStartEnd);
	}
}

public OnEventListenChange(Handle:convar, const String:oldValue[], const String:newValue[])
{
	bStopRoundEnd = bool:StringToInt(newValue);
	if (!bStopRoundEnd)
	{
		bRoundEnd = false;
	}
}

public OnRoundStartEnd(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (bStopRoundEnd)
	{
		if (StrEqual(name, "round_end"))
		{
			bRoundEnd = true;
		}
		else
		{
			bRoundEnd = false;
		}
	}
}

public Action:Command_Reload(client, args)
{
	decl String:buffer[PLATFORM_MAX_PATH];
	if (kv != INVALID_HANDLE)
	{
		KvRewind(kv);
		if (KvGotoFirstSubKey(kv))
		{
			do
			{
				if (KvGetNum(kv, "hooked", 0) != 0)
				{
					KvGetSectionName(kv, buffer, sizeof(buffer));
					UnhookEvent(buffer, OnSomeEvent);
				}
			} while (KvGotoNextKey(kv));
		}
		CloseHandle(kv);
	}
	
	GetCfgFile(buffer, sizeof(buffer), "moneydistributor.txt");
	
	kv = CreateKeyValues("Events");
	
	if (!FileToKeyValues(kv, buffer))
	{
		SetFailState("Could not parse %s", buffer);
	}
	
	if (KvGotoFirstSubKey(kv))
	{
		do
		{
			KvGetSectionName(kv, buffer, sizeof(buffer));
			if (HookEventEx(buffer, OnSomeEvent))
			{
				KvSetNum(kv, "hooked", 1);
			}
			else
			{
				LogError("Invalid event \"%s\"", buffer);
			}
		} while (KvGotoNextKey(kv));
	}
	KvRewind(kv);
}

public OnSomeEvent(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (bRoundEnd || !KvJumpToKey(kv, name)) return;
	
	if (KvGotoFirstSubKey(kv))
	{
		if (KvJumpToKey(kv, "all"))
		{
			new credits = KvGetNum(kv, "credits", 0);
			if (credits == 0)
			{
				KvRewind(kv);
				return;
			}
			new team = KvGetNum(kv, "team", 1);
			new teamfilter = KvGetNum(kv, "teamfilter", 1);
			new bool:alive = bool:KvGetNum(kv, "alive", 0);
			for (new client = 1; client <= MaxClients; client++)
			{
				if (IsClientInGame(client) && !IsFakeClient(client))
				{
					if (alive && !IsPlayerAlive(client))
					{
						continue;
					}
					new cl_team = GetClientTeam(client);
					if (team != 0 && cl_team != team)
					{
						continue;
					}
					if (teamfilter != 0 && cl_team == teamfilter)
					{
						continue;
					}
					Shop_GiveCredits(client, credits);
					
					decl String:text[256], String:sCredits[12];
					KvGetString(kv, "text", text, sizeof(text), "");
					if (text[0])
					{
						IntToString(credits, sCredits, sizeof(sCredits));
						ReplaceString(text, sizeof(text), "{credits}", sCredits, false);
						CPrintToChat(client, text);
					}
				}
			}
			
			KvRewind(kv);
			return;
		}
		decl String:section[32], String:type[12], String:text[256], client;
		do
		{
			new credits = KvGetNum(kv, "credits", 0);
			if (credits == 0)
			{
				continue;
			}
			KvGetSectionName(kv, section, sizeof(section));
			KvGetString(kv, "type", type, sizeof(type), "int");
			
			if (StrEqual(type, "userid", false))
			{
				client = GetClientOfUserId(GetEventInt(event, section));
				if (!client || !IsClientInGame(client))
				{
					continue;
				}
			}
			else if (StrEqual(type, "int", false))
			{
				client = GetEventInt(event, section);
				if (!(0 < client <= MaxClients) || !IsClientInGame(client))
				{
					continue;
				}
			}
			else
			{
				LogError("Invelid type set \"%s\"", type);
				continue;
			}
			if (IsFakeClient(client))
			{
				continue;
			}
			Shop_GiveCredits(client, credits);
			KvGetString(kv, "text", text, sizeof(text), "");
			if (text[0])
			{
				IntToString(credits, type, sizeof(type));
				ReplaceString(text, sizeof(text), "{credits}", type, false);
				CPrintToChat(client, text);
			}
		} while (KvGotoNextKey(kv));
	}
	KvRewind(kv);
}

public OnIntervalChange(Handle:convar, const String:oldValue[], const String:newValue[])
{
	for (new i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i) || !(1 < GetClientTeam(i) < 4)) continue;
		
		Create(i);
	}
}

public OnClientDisconnect_Post(client)
{
	if (h_timer[client] != INVALID_HANDLE)
	{
		KillTimer(h_timer[client]);
		h_timer[client] = INVALID_HANDLE;
	}
}

Create(client)
{
	OnClientDisconnect_Post(client);
	h_timer[client] = CreateTimer(GetConVarFloat(g_hInterval), GivePoints, client, TIMER_REPEAT);
}

public OnPlayerTeam(Handle:event, const String:name[], bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	if (!client || IsFakeClient(client)) return;
	
	switch (GetEventInt(event, "team"))
	{
		case 2, 3 :
		{
			Create(client);
		}
		default :
		{
			OnClientDisconnect_Post(client);
		}
	}
}

public Action:GivePoints(Handle:timer, any:client)
{
	new amount = GetConVarInt(g_hMoneyPerTick);
	if (Shop_GiveCredits(client, amount) != -1)
	{
		PrintToChat(client, "\x04[Shop] \x01Вы получили \x04%d кредита(ов)\x01. Вы можете открыть магазин \x04!store \x01или \x04!shop \x01чтобы использовать их.", amount);
	}
}