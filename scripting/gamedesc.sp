#include <sourcemod>
#include <sdkhooks> 
new Handle:g_Desc;
public Plugin:myinfo = 
{
	name = "GameDesc override (SDKHooks)",
	author = "KorDen",
	description = "Overriding game description using modified SDKHooks",
	version = "1.0",
	url = "http://css32.ru/"
};
public OnPluginStart()
{
	g_Desc=CreateConVar("sm_gamedesc","","Set game descption when server loading (64 symbols max)");
}
public Action:OnGetGameDescription(String:gameDesc[64]) 
{
	decl String:desc[64];
	GetConVarString(g_Desc,desc,sizeof(desc));
	if (StrEqual(desc,""))
	{
		LogError("Game description not set, please set it in you runscript using +sm_gamedesc");
	}
	else
	{
		PrintToServer("[SM] Game description set to %s",desc);
		strcopy(gameDesc, sizeof(gameDesc), desc);
	}
	return Plugin_Changed; 
} 