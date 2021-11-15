
//**************************************************************************
//**
//** ct_chat.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision: 529 $
//** $Date: 2009-06-10 16:27:26 +0300 (Wed, 10 Jun 2009) $
//**
//**************************************************************************

#include "h2stdinc.h"
#include <ctype.h>
#include "h2def.h"
#include "p_local.h"
#include "soundst.h"
#ifdef RENDER3D
#include "ogl_def.h"
#endif

#define QUEUESIZE		128
#define MESSAGESIZE		128
#define MESSAGELEN		265

// 8-player note:  Change this stuff (CT_PLR_*, and the key mappings)
enum
{
	CT_PLR_BLUE	= 1,
	CT_PLR_RED,
	CT_PLR_YELLOW,
	CT_PLR_GREEN,
#if (MAXPLAYERS >= MAXPLAYERS_11)
	CT_PLR_PLAYER5,
	CT_PLR_PLAYER6,
	CT_PLR_PLAYER7,
	CT_PLR_PLAYER8,
#endif
	CT_PLR_ALL
};

#define CT_KEY_BLUE		'b'
#define CT_KEY_RED		'r'
#define CT_KEY_YELLOW		'y'
#define CT_KEY_GREEN		'g'
#define CT_KEY_PLAYER5		'j'	/* Jade */
#define CT_KEY_PLAYER6		'w'	/* White */
#define CT_KEY_PLAYER7		'h'	/* Hazel */
#define CT_KEY_PLAYER8		'p'	/* Purple */
#define CT_KEY_ALL		't'
#define CT_ESCAPE		6

// Public data

void CT_Init(void);
void CT_Drawer(void);
BOOLEAN CT_Responder(event_t *ev);
void CT_Ticker(void);
char CT_dequeueChatChar(void);

BOOLEAN chatmodeon;

char chat_macros[10][80] =
{
	HUSTR_CHATMACRO0,
	HUSTR_CHATMACRO1,
	HUSTR_CHATMACRO2,
	HUSTR_CHATMACRO3,
	HUSTR_CHATMACRO4,
	HUSTR_CHATMACRO5,
	HUSTR_CHATMACRO6,
	HUSTR_CHATMACRO7,
	HUSTR_CHATMACRO8,
	HUSTR_CHATMACRO9
};

// Private data

static void CT_queueChatChar(char ch);
static void CT_ClearChatMessage(int player);
static void CT_AddChar(int player, char c);
static void CT_BackSpace(int player);

static int head, tail;
static byte ChatQueue[QUEUESIZE];
static int chat_dest[MAXPLAYERS];
static char chat_msg[MAXPLAYERS][MESSAGESIZE];
static char plr_lastmsg[MAXPLAYERS][MESSAGESIZE+9];
static int msgptr[MAXPLAYERS];
static int msglen[MAXPLAYERS];

static int FontABaseLump;

static const char *CT_FromPlrText[MAXPLAYERS_11] =
{
	"BLUE:  ",
	"RED:  ",
	"YELLOW:  ",
	"GREEN:  ",
	"JADE:  ",
	"WHITE:  ",
	"HAZEL:  ",
	"PURPLE:  "
};

static BOOLEAN altdown, shiftdown;

extern BOOLEAN usearti;


//===========================================================================
//
// CT_Init
//
// 	Initialize chat mode data
//===========================================================================

void CT_Init(void)
{
	int i;

	head = 0; //initialize the queue index
	tail = 0;
	chatmodeon = false;
	memset(ChatQueue, 0, QUEUESIZE);
	for (i = 0; i < MAXPLAYERS; i++)
	{
		chat_dest[i] = 0;
		msgptr[i] = 0;
		memset(plr_lastmsg[i], 0, MESSAGESIZE);
		memset(chat_msg[i], 0, MESSAGESIZE);
	}
	FontABaseLump = W_GetNumForName("FONTA_S") + 1;
	return;
}

//===========================================================================
//
// CT_Stop
//
//===========================================================================

void CT_Stop(void)
{
	chatmodeon = false;
	return;
}

//===========================================================================
//
// CT_Responder
//
//===========================================================================

BOOLEAN CT_Responder(event_t *ev)
{
	const char *macro;
	int sendtarget;

	if (!netgame)
	{
		return false;
	}
	if (ev->data1 == KEY_RALT)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}
	if (ev->data1 == KEY_RSHIFT)
	{
		shiftdown = (ev->type == ev_keydown);
		return false;
	}
	if (gamestate != GS_LEVEL || ev->type != ev_keydown)
	{
		return false;
	}
	if (!chatmodeon)
	{
		sendtarget = 0;
		if (ev->data1 == CT_KEY_ALL)
		{
			sendtarget = CT_PLR_ALL;
		}
		else if (ev->data1 == CT_KEY_GREEN)
		{
			sendtarget = CT_PLR_GREEN;
		}
		else if (ev->data1 == CT_KEY_YELLOW)
		{
			sendtarget = CT_PLR_YELLOW;
		}
		else if (ev->data1 == CT_KEY_RED)
		{
			sendtarget = CT_PLR_RED;
		}
		else if (ev->data1 == CT_KEY_BLUE)
		{
			sendtarget = CT_PLR_BLUE;
		}
#if (MAXPLAYERS == MAXPLAYERS_11)
		else if (ev->data1 == CT_KEY_PLAYER5)
		{
			sendtarget = CT_PLR_PLAYER5;
		}
		else if (ev->data1 == CT_KEY_PLAYER6)
		{
			sendtarget = CT_PLR_PLAYER6;
		}
		else if (ev->data1 == CT_KEY_PLAYER7)
		{
			sendtarget = CT_PLR_PLAYER7;
		}
		else if (ev->data1 == CT_KEY_PLAYER8)
		{
			sendtarget = CT_PLR_PLAYER8;
		}
#endif	/* 8-players */
		if (sendtarget == 0 || (sendtarget != CT_PLR_ALL && !playeringame[sendtarget - 1])
			|| sendtarget == consoleplayer + 1)
		{
			return false;
		}
		CT_queueChatChar(sendtarget);
		chatmodeon = true;
		return true;
	}
	else
	{
		if (altdown)
		{
			if (ev->data1 >= '0' && ev->data1 <= '9')
			{
				if (ev->data1 == '0')
				{ // macro 0 comes after macro 9
					ev->data1 = '9' + 1;
				}
				macro = chat_macros[ev->data1-'1'];
				CT_queueChatChar(KEY_ENTER); //send old message
				CT_queueChatChar(chat_dest[consoleplayer]); // chose the dest.
				while (*macro)
				{
					CT_queueChatChar(toupper((int) *macro++));
				}
				CT_queueChatChar(KEY_ENTER); //send it off...
				CT_Stop();
				return true;
			}
		}
		if (ev->data1 == KEY_ENTER)
		{
			CT_queueChatChar(KEY_ENTER);
			usearti = false;
			CT_Stop();
			return true;
		}
		else if (ev->data1 == KEY_ESCAPE)
		{
			CT_queueChatChar(CT_ESCAPE);
			CT_Stop();
			return true;
		}
		else if (ev->data1 >= 'a' && ev->data1 <= 'z')
		{
			CT_queueChatChar(ev->data1-32);
			return true;
		}
		else if (shiftdown)
		{
			if (ev->data1 == '1')
			{
				CT_queueChatChar('!');
				return true;
			}
			else if (ev->data1 == '/')
			{
				CT_queueChatChar('?');
				return true;
			}
		}
		if (ev->data1 == ' ' || ev->data1 == ',' || ev->data1 == '.' ||
		    (ev->data1 >= '0' && ev->data1 <= '9') || ev->data1 == '\'' ||
		    ev->data1 == KEY_BACKSPACE || ev->data1 == '-' || ev->data1 == '=')
		{
			CT_queueChatChar(ev->data1);
			return true;
		}
	}
	return false;
}

//===========================================================================
//
// CT_Ticker
//
//===========================================================================

void CT_Ticker(void)
{
	int i, j;
	char c;
	int numplayers;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			continue;
		}
		if ((c = players[i].cmd.chatchar) != 0)
		{
			if (c <= CT_PLR_ALL)
			{
				chat_dest[i] = c;
				continue;
			}
			else if (c == CT_ESCAPE)
			{
				CT_ClearChatMessage(i);
			}
			else if (c == KEY_ENTER)
			{
				numplayers = 0;
				for (j = 0; j < MAXPLAYERS; j++)
				{
					numplayers += playeringame[j];
				}
				CT_AddChar(i, 0); // set the end of message character
				if (numplayers > 2)
				{
					strcpy(plr_lastmsg[i], CT_FromPlrText[i]);
					strcat(plr_lastmsg[i], chat_msg[i]);
				}
				else
				{
					strcpy(plr_lastmsg[i], chat_msg[i]);
				}
				if (i != consoleplayer && (chat_dest[i] == consoleplayer + 1
					|| chat_dest[i] == CT_PLR_ALL) && *chat_msg[i])
				{
					P_SetMessage(&players[consoleplayer], plr_lastmsg[i], true);
					S_StartSound(NULL, SFX_CHAT);
				}
				else if (i == consoleplayer && (*chat_msg[i]))
				{
					if (numplayers <= 1)
					{
						P_SetMessage(&players[consoleplayer],
							"THERE ARE NO OTHER PLAYERS IN THE GAME!", true);
						S_StartSound(NULL, SFX_CHAT);
					}
				}
				CT_ClearChatMessage(i);
			}
			else if (c == KEY_BACKSPACE)
			{
				CT_BackSpace(i);
			}
			else
			{
				CT_AddChar(i, c);
			}
		}
	}
	return;
}

//===========================================================================
//
// CT_Drawer
//
//===========================================================================

void CT_Drawer(void)
{
	int i;
	int x;
	patch_t *patch;

	if (chatmodeon)
	{
		x = 25;
		for (i = 0; i < msgptr[consoleplayer]; i++)
		{
			if (chat_msg[consoleplayer][i] < 33)
			{
				x += 6;
			}
			else
			{
				patch = (patch_t *) W_CacheLumpNum(FontABaseLump + chat_msg[consoleplayer][i] - 33, PU_CACHE);
#ifdef RENDER3D
				OGL_DrawPatch_CS(x, 10, FontABaseLump + chat_msg[consoleplayer][i] - 33);
#else
				V_DrawPatch(x, 10, patch);
#endif
				x += SHORT(patch->width);
			}
		}
#ifdef RENDER3D
		OGL_DrawPatch_CS(x, 10, W_GetNumForName("FONTA59"));
#else
		patch = (patch_t *) W_CacheLumpName("FONTA59", PU_CACHE);
		V_DrawPatch(x, 10, patch);
#endif
		BorderTopRefresh = true;
		UpdateState |= I_MESSAGES;
	}
}

//===========================================================================
//
// CT_queueChatChar
//
//===========================================================================

static void CT_queueChatChar(char ch)
{
	if (((tail + 1) & (QUEUESIZE - 1)) == head)
	{ // the queue is full
		return;
	}
	ChatQueue[tail] = ch;
	tail = (tail + 1) & (QUEUESIZE - 1);
}

//===========================================================================
//
// CT_dequeueChatChar
//
//===========================================================================

char CT_dequeueChatChar(void)
{
	byte temp;

	if (head == tail)
	{ // queue is empty
		return 0;
	}
	temp = ChatQueue[head];
	head = (head + 1) & (QUEUESIZE - 1);
	return temp;
}

//===========================================================================
//
// CT_AddChar
//
//===========================================================================

static void CT_AddChar(int player, char c)
{
	patch_t *patch;

	if (msgptr[player] + 1 >= MESSAGESIZE || msglen[player] >= MESSAGELEN)
	{ // full.
		return;
	}
	chat_msg[player][msgptr[player]] = c;
	msgptr[player]++;
	if (c < 33)
	{
		msglen[player] += 6;
	}
	else
	{
		patch = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
		msglen[player] += SHORT(patch->width);
	}
}

//===========================================================================
//
// CT_BackSpace
//
// 	Backs up a space, when the user hits (obviously) backspace
//===========================================================================

static void CT_BackSpace(int player)
{
	patch_t *patch;
	char c;

	if (msgptr[player] == 0)
	{ // message is already blank
		return;
	}
	msgptr[player]--;
	c = chat_msg[player][msgptr[player]];
	if (c < 33)
	{
		msglen[player] -= 6;
	}
	else
	{
		patch = (patch_t *) W_CacheLumpNum(FontABaseLump + c - 33, PU_CACHE);
		msglen[player] -= SHORT(patch->width);
	}
	chat_msg[player][msgptr[player]] = 0;
}

//===========================================================================
//
// CT_ClearChatMessage
//
// 	Clears out the data for the chat message, but the player's message
//		is still saved in plrmsg.
//===========================================================================

static void CT_ClearChatMessage(int player)
{
	memset(chat_msg[player], 0, MESSAGESIZE);
	msgptr[player] = 0;
	msglen[player] = 0;
}

