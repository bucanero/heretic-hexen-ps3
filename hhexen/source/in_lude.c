
//**************************************************************************
//**
//** in_lude.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision: 458 $
//** $Date: 2009-05-25 15:35:27 +0300 (Mon, 25 May 2009) $
//**
//**************************************************************************

#include "h2stdinc.h"
#include <ctype.h>
#include "h2def.h"
#include "v_compat.h"

// MACROS ------------------------------------------------------------------

#define	TEXTSPEED	3
#define	TEXTWAIT	140

#ifdef RENDER3D
#define V_DrawPatch(x,y,p)		OGL_DrawPatch((x),(y),(p))
#define V_DrawRawScreen(a)		OGL_DrawRawScreen((a))
#endif

// TYPES -------------------------------------------------------------------

typedef enum
{
	SINGLE,
	COOPERATIVE,
	DEATHMATCH
} gametype_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

extern void AM_Stop(void);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void IN_Start(void);
void IN_Ticker(void);
void IN_Drawer(void);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void WaitStop(void);
static void Stop(void);
static void LoadPics(void);
static void UnloadPics(void);
static void CheckForSkip(void);
static void InitStats(void);
static void DrDeathTally(void);
static void DrNumber(int val, int x, int y, int wrapThresh);
static void DrNumberBold(int val, int x, int y, int wrapThresh);
static void DrawHubText(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DECLARATIONS ------------------------------------------------

BOOLEAN intermission;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static BOOLEAN skipintermission;
static int interstate = 0;
static int intertime = -1;
static gametype_t gametype;
static int cnt;
static int slaughterboy; // in DM, the player with the most kills
static PATCH_REF patchINTERPIC;
static PATCH_REF FontBNumbers[10];
static PATCH_REF FontBNegative;
static PATCH_REF FontBSlash;
static PATCH_REF FontBPercent;
static int FontABaseLump;
static int FontBLump;
static int FontBLumpBase;

static signed int totalFrags[MAXPLAYERS];

static int HubCount;
static const char *HubText;

// CODE --------------------------------------------------------------------

//========================================================================
//
// IN_Start
//
//========================================================================

void IN_Start(void)
{
	int i;

	V_SetPaletteBase();
	InitStats();
	LoadPics();
	intermission = true;
	interstate = 0;
	skipintermission = false;
	intertime = 0;
	AM_Stop();
	for (i = 0; i < MAXPLAYERS; i++)
	{
		players[i].messageTics = 0;
		players[i].message[0] = 0;
	}
	SN_StopAllSequences();
}

//========================================================================
//
// Stop
//
//========================================================================

static void Stop(void)
{
	intermission = false;
	UnloadPics();
	SB_state = -1;
	BorderNeedRefresh = true;
}

//========================================================================
//
// WaitStop
//
//========================================================================

void WaitStop(void)
{
	if (!--cnt)
	{
		Stop();
//		gamestate = GS_LEVEL;
//		G_DoLoadLevel();
		gameaction = ga_leavemap;
//		G_WorldDone();
	}
}

//========================================================================
//
// InitStats
//
// 	Initializes the stats for single player mode
//========================================================================

static void InitStats(void)
{
	int i;
	int j;
	int oldCluster;
	signed int slaughterfrags;
	int posnum;
	int slaughtercount;
	int playercount;

	extern int LeaveMap;

	if (!deathmatch)
	{
		gametype = SINGLE;
		HubCount = 0;
		oldCluster = P_GetMapCluster(gamemap);
		if (oldCluster != P_GetMapCluster(LeaveMap))
		{
			if (oldCluster >= 1 && oldCluster <= 5)
			{
				HubText = GetClusterText(oldCluster - 1);
				HubCount = strlen(HubText)*TEXTSPEED + TEXTWAIT;
				S_StartSongName("hub", true);
			}
		}
	}
	else
	{
		gametype = DEATHMATCH;
		slaughterboy = 0;
		slaughterfrags = -9999;
		posnum = 0;
		playercount = 0;
		slaughtercount = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			totalFrags[i] = 0;
			if (playeringame[i])
			{
				playercount++;
				for (j = 0; j < MAXPLAYERS; j++)
				{
					if (playeringame[j])
					{
						totalFrags[i] += players[i].frags[j];
					}
				}
				posnum++;
			}
			if (totalFrags[i] > slaughterfrags)
			{
				slaughterboy = 1<<i;
				slaughterfrags = totalFrags[i];
				slaughtercount = 1;
			}
			else if (totalFrags[i] == slaughterfrags)
			{
				slaughterboy |= 1<<i;
				slaughtercount++;
			}
		}
		if (playercount == slaughtercount)
		{ // don't do the slaughter stuff if everyone is equal
			slaughterboy = 0;
		}
		S_StartSongName("hub", true);
	}
}

//========================================================================
//
// LoadPics
//
//========================================================================

static void LoadPics(void)
{
	int i;

	if (HubCount || gametype == DEATHMATCH)
	{
		patchINTERPIC = (PATCH_REF) WR_CacheLumpName("INTERPIC", PU_STATIC);
		FontBLumpBase = W_GetNumForName("FONTB16");
		for (i = 0; i < 10; i++)
		{
			FontBNumbers[i] = (PATCH_REF) WR_CacheLumpNum(FontBLumpBase + i, PU_STATIC);
		}
		FontBLump = W_GetNumForName("FONTB_S") + 1;
		FontBNegative = (PATCH_REF) WR_CacheLumpName("FONTB13", PU_STATIC);
		FontABaseLump = W_GetNumForName("FONTA_S") + 1;

		FontBSlash = (PATCH_REF) WR_CacheLumpName("FONTB15", PU_STATIC);
		FontBPercent = (PATCH_REF) WR_CacheLumpName("FONTB05", PU_STATIC);
	}
}

//========================================================================
//
// UnloadPics
//
//========================================================================

static void UnloadPics(void)
{
#ifndef RENDER3D
	int i;

	if (HubCount || gametype == DEATHMATCH)
	{
		Z_ChangeTag(patchINTERPIC, PU_CACHE);
		for (i = 0; i < 10; i++)
		{
			Z_ChangeTag(FontBNumbers[i], PU_CACHE);
		}
		Z_ChangeTag(FontBNegative, PU_CACHE);
		Z_ChangeTag(FontBSlash, PU_CACHE);
		Z_ChangeTag(FontBPercent, PU_CACHE);
	}
#endif
}

//========================================================================
//
// IN_Ticker
//
//========================================================================

void IN_Ticker(void)
{
	if (!intermission)
	{
		return;
	}
	if (interstate)
	{
		WaitStop();
		return;
	}
	skipintermission = false;
	CheckForSkip();
	intertime++;
	if (skipintermission || (gametype == SINGLE && !HubCount))
	{
		interstate = 1;
		cnt = 10;
		skipintermission = false;
		//S_StartSound(NULL, sfx_dorcls);
	}
}

//========================================================================
//
// CheckForSkip
//
// 	Check to see if any player hit a key
//========================================================================

static void CheckForSkip(void)
{
	int i;
	player_t *player;
	static BOOLEAN triedToSkip;

	for (i = 0, player = players; i < MAXPLAYERS; i++, player++)
	{
		if (playeringame[i])
		{
			if (player->cmd.buttons & BT_ATTACK)
			{
				if (!player->attackdown)
				{
					skipintermission = 1;
				}
				player->attackdown = true;
			}
			else
			{
				player->attackdown = false;
			}
			if (player->cmd.buttons & BT_USE)
			{
				if (!player->usedown)
				{
					skipintermission = 1;
				}
				player->usedown = true;
			}
			else
			{
				player->usedown = false;
			}
		}
	}
	if (deathmatch && intertime < 140)
	{ // wait for 4 seconds before allowing a skip
		if (skipintermission == 1)
		{
			triedToSkip = true;
			skipintermission = 0;
		}
	}
	else
	{
		if (triedToSkip)
		{
			skipintermission = 1;
			triedToSkip = false;
		}
	}
}

//========================================================================
//
// IN_Drawer
//
//========================================================================

void IN_Drawer(void)
{
	if (!intermission)
	{
		return;
	}
	if (interstate)
	{
		return;
	}
	UpdateState |= I_FULLSCRN;

	V_DrawRawScreen((BYTE_REF) patchINTERPIC);

	if (gametype == SINGLE)
	{
		if (HubCount)
		{
			DrawHubText();
		}
	}
	else
	{
		DrDeathTally();
	}
}

//========================================================================
//
// DrDeathTally
//
//========================================================================

#define TALLY_EFFECT_TICKS	20
#define TALLY_FINAL_X_DELTA	(23 * FRACUNIT)
#define TALLY_FINAL_Y_DELTA	(13 * FRACUNIT)
#define TALLY_START_XPOS	(178* FRACUNIT)
#define TALLY_STOP_XPOS		(90 * FRACUNIT)
#define TALLY_START_YPOS	(132* FRACUNIT)
#define TALLY_STOP_YPOS		(83 * FRACUNIT)
#define TALLY_TOP_X		85
#define TALLY_TOP_Y		9
#define TALLY_LEFT_X		7
#define TALLY_LEFT_Y		71
#define TALLY_TOTALS_X		291

static void DrDeathTally(void)
{
	int i, j, temp;
	fixed_t xPos, yPos;
	fixed_t xDelta, yDelta;
	fixed_t xStart, scale;
	int x, y;
	BOOLEAN bold;
	static BOOLEAN showTotals;

	V_DrawPatch(TALLY_TOP_X, TALLY_TOP_Y, (PATCH_REF)WR_CacheLumpName("tallytop", PU_CACHE));
	V_DrawPatch(TALLY_LEFT_X, TALLY_LEFT_Y, (PATCH_REF)WR_CacheLumpName("tallylft", PU_CACHE));
	if (intertime < TALLY_EFFECT_TICKS)
	{
		showTotals = false;
		scale = (intertime * FRACUNIT) / TALLY_EFFECT_TICKS;
		xDelta = FixedMul(scale, TALLY_FINAL_X_DELTA);
		yDelta = FixedMul(scale, TALLY_FINAL_Y_DELTA);
		xStart = TALLY_START_XPOS - FixedMul(scale, TALLY_START_XPOS-TALLY_STOP_XPOS);
		yPos = TALLY_START_YPOS - FixedMul(scale, TALLY_START_YPOS-TALLY_STOP_YPOS);
	}
	else
	{
		xDelta = TALLY_FINAL_X_DELTA;
		yDelta = TALLY_FINAL_Y_DELTA;
		xStart = TALLY_STOP_XPOS;
		yPos = TALLY_STOP_YPOS;
	}
	if (intertime >= TALLY_EFFECT_TICKS && showTotals == false)
	{
		showTotals = true;
		S_StartSound(NULL, SFX_PLATFORM_STOP);
	}
	y = yPos>>FRACBITS;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		xPos = xStart;
		for (j = 0; j < MAXPLAYERS; j++, xPos += xDelta)
		{
			x = xPos>>FRACBITS;
			bold = (i == consoleplayer || j == consoleplayer);
			if (playeringame[i] && playeringame[j])
			{
				if (bold)
				{
					DrNumberBold(players[i].frags[j], x, y, 100);
				}
				else
				{
					DrNumber(players[i].frags[j], x, y, 100);
				}
			}
			else
			{
				temp = MN_TextAWidth("--")/2;
				if (bold)
				{
					MN_DrTextAYellow("--", x - temp, y);
				}
				else
				{
					MN_DrTextA("--", x - temp, y);
				}
			}
		}
		if (showTotals && playeringame[i]
			&& !((slaughterboy & (1<<i)) && !(intertime & 16)))
		{
			DrNumber(totalFrags[i], TALLY_TOTALS_X, y, 1000);
		}
		yPos += yDelta;
		y = yPos>>FRACBITS;
	}
}

//==========================================================================
//
// DrNumber
//
//==========================================================================

static void DrNumber(int val, int x, int y, int wrapThresh)
{
	char buff[8] = "XX";

	if (!(val < -9 && wrapThresh < 1000))
	{
		snprintf(buff, sizeof(buff), "%d", val >= wrapThresh ? val % wrapThresh : val);
	}
	MN_DrTextA(buff, x - MN_TextAWidth(buff)/2, y);
}

//==========================================================================
//
// DrNumberBold
//
//==========================================================================

static void DrNumberBold(int val, int x, int y, int wrapThresh)
{
	char buff[8] = "XX";

	if (!(val < -9 && wrapThresh < 1000))
	{
		snprintf(buff, sizeof(buff), "%d", val >= wrapThresh ? val % wrapThresh : val);
	}
	MN_DrTextAYellow(buff, x - MN_TextAWidth(buff)/2, y);
}

//===========================================================================
//
// DrawHubText
//
//===========================================================================

static void DrawHubText(void)
{
	int		count;
	const char	*ch;
	int		c;
	int		cx, cy;
	patch_t		*w;
	int		width;

	cy = 5;
	cx = 10;
	ch = HubText;
	count = (intertime - 10) / TEXTSPEED;
	if (count < 0)
	{
		count = 0;
	}
	for ( ; count; count--)
	{
		c = *ch++;
		if (!c)
		{
			break;
		}
		if (c == '\n')
		{
			cx = 10;
			cy += 9;
			continue;
		}
		if (c < 32)
		{
			continue;
		}
		c = toupper(c);
		if (c == 32)
		{
			cx += 5;
			continue;
		}
		w = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
		width = SHORT(w->width);
		if (cx + width > SCREENWIDTH)
		{
			break;
		}
#ifdef RENDER3D
		OGL_DrawPatch_CS(cx, cy, FontABaseLump + c - 33);
#else
		V_DrawPatch(cx, cy, w);
#endif
		cx += width;
	}
}

