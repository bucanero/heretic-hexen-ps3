
//**************************************************************************
//**
//** p_acs.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision: 491 $
//** $Date: 2009-05-30 01:10:42 +0300 (Sat, 30 May 2009) $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include "h2def.h"
#include "p_local.h"

// MACROS ------------------------------------------------------------------

#define SCRIPT_CONTINUE		0
#define SCRIPT_STOP		1
#define SCRIPT_TERMINATE	2

#define OPEN_SCRIPTS_BASE	1000
#define PRINT_BUFFER_SIZE	256

#define GAME_SINGLE_PLAYER	0
#define GAME_NET_COOPERATIVE	1
#define GAME_NET_DEATHMATCH	2

#define TEXTURE_TOP		0
#define TEXTURE_MIDDLE		1
#define TEXTURE_BOTTOM		2

#define S_DROP		ACScript->stackPtr--
#define S_POP		ACScript->stack[--ACScript->stackPtr]
#define S_PUSH(x)	ACScript->stack[ACScript->stackPtr++] = (x)

// TYPES -------------------------------------------------------------------

typedef struct
{
	int marker;
	int infoOffset;
	int code;
} acsHeader_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void StartOpenACS(int number, int infoIndex, byte *address);
static void ScriptFinished(int number);
static BOOLEAN TagBusy(int tag);
static BOOLEAN AddToACSStore(int map, int number, byte *args);
static int GetACSIndex(int number);
static void Push(int value);
static int Pop(void);
static int Top(void);
static void Drop(void);

static int CmdNOP(void);
static int CmdTerminate(void);
static int CmdSuspend(void);
static int CmdPushNumber(void);
static int CmdLSpec1(void);
static int CmdLSpec2(void);
static int CmdLSpec3(void);
static int CmdLSpec4(void);
static int CmdLSpec5(void);
static int CmdLSpec1Direct(void);
static int CmdLSpec2Direct(void);
static int CmdLSpec3Direct(void);
static int CmdLSpec4Direct(void);
static int CmdLSpec5Direct(void);
static int CmdAdd(void);
static int CmdSubtract(void);
static int CmdMultiply(void);
static int CmdDivide(void);
static int CmdModulus(void);
static int CmdEQ(void);
static int CmdNE(void);
static int CmdLT(void);
static int CmdGT(void);
static int CmdLE(void);
static int CmdGE(void);
static int CmdAssignScriptVar(void);
static int CmdAssignMapVar(void);
static int CmdAssignWorldVar(void);
static int CmdPushScriptVar(void);
static int CmdPushMapVar(void);
static int CmdPushWorldVar(void);
static int CmdAddScriptVar(void);
static int CmdAddMapVar(void);
static int CmdAddWorldVar(void);
static int CmdSubScriptVar(void);
static int CmdSubMapVar(void);
static int CmdSubWorldVar(void);
static int CmdMulScriptVar(void);
static int CmdMulMapVar(void);
static int CmdMulWorldVar(void);
static int CmdDivScriptVar(void);
static int CmdDivMapVar(void);
static int CmdDivWorldVar(void);
static int CmdModScriptVar(void);
static int CmdModMapVar(void);
static int CmdModWorldVar(void);
static int CmdIncScriptVar(void);
static int CmdIncMapVar(void);
static int CmdIncWorldVar(void);
static int CmdDecScriptVar(void);
static int CmdDecMapVar(void);
static int CmdDecWorldVar(void);
static int CmdGoto(void);
static int CmdIfGoto(void);
static int CmdDrop(void);
static int CmdDelay(void);
static int CmdDelayDirect(void);
static int CmdRandom(void);
static int CmdRandomDirect(void);
static int CmdThingCount(void);
static int CmdThingCountDirect(void);
static int CmdTagWait(void);
static int CmdTagWaitDirect(void);
static int CmdPolyWait(void);
static int CmdPolyWaitDirect(void);
static int CmdChangeFloor(void);
static int CmdChangeFloorDirect(void);
static int CmdChangeCeiling(void);
static int CmdChangeCeilingDirect(void);
static int CmdRestart(void);
static int CmdAndLogical(void);
static int CmdOrLogical(void);
static int CmdAndBitwise(void);
static int CmdOrBitwise(void);
static int CmdEorBitwise(void);
static int CmdNegateLogical(void);
static int CmdLShift(void);
static int CmdRShift(void);
static int CmdUnaryMinus(void);
static int CmdIfNotGoto(void);
static int CmdLineSide(void);
static int CmdScriptWait(void);
static int CmdScriptWaitDirect(void);
static int CmdClearLineSpecial(void);
static int CmdCaseGoto(void);
static int CmdBeginPrint(void);
static int CmdEndPrint(void);
static int CmdPrintString(void);
static int CmdPrintNumber(void);
static int CmdPrintCharacter(void);
static int CmdPlayerCount(void);
static int CmdGameType(void);
static int CmdGameSkill(void);
static int CmdTimer(void);
static int CmdSectorSound(void);
static int CmdAmbientSound(void);
static int CmdSoundSequence(void);
static int CmdSetLineTexture(void);
static int CmdSetLineBlocking(void);
static int CmdSetLineSpecial(void);
static int CmdThingSound(void);
static int CmdEndPrintBold(void);

static void ThingCount(int type, int tid);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern const char *TextKeyMessages[11];

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int		ACScriptCount;
byte		*ActionCodeBase;
acsInfo_t	*ACSInfo;
int		MapVars[MAX_ACS_MAP_VARS];
int		WorldVars[MAX_ACS_WORLD_VARS];
acsstore_t	ACSStore[MAX_ACS_STORE + 1];	// +1 for termination marker

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static acs_t	*ACScript;
static byte	*PCodePtr;
static byte	SpecArgs[8];
static int	ACStringCount;
static char	**ACStrings;
static char	PrintBuffer[PRINT_BUFFER_SIZE];
static acs_t	*NewScript;

static int (*PCodeCmds[])(void) =
{
	CmdNOP,
	CmdTerminate,
	CmdSuspend,
	CmdPushNumber,
	CmdLSpec1,
	CmdLSpec2,
	CmdLSpec3,
	CmdLSpec4,
	CmdLSpec5,
	CmdLSpec1Direct,
	CmdLSpec2Direct,
	CmdLSpec3Direct,
	CmdLSpec4Direct,
	CmdLSpec5Direct,
	CmdAdd,
	CmdSubtract,
	CmdMultiply,
	CmdDivide,
	CmdModulus,
	CmdEQ,
	CmdNE,
	CmdLT,
	CmdGT,
	CmdLE,
	CmdGE,
	CmdAssignScriptVar,
	CmdAssignMapVar,
	CmdAssignWorldVar,
	CmdPushScriptVar,
	CmdPushMapVar,
	CmdPushWorldVar,
	CmdAddScriptVar,
	CmdAddMapVar,
	CmdAddWorldVar,
	CmdSubScriptVar,
	CmdSubMapVar,
	CmdSubWorldVar,
	CmdMulScriptVar,
	CmdMulMapVar,
	CmdMulWorldVar,
	CmdDivScriptVar,
	CmdDivMapVar,
	CmdDivWorldVar,
	CmdModScriptVar,
	CmdModMapVar,
	CmdModWorldVar,
	CmdIncScriptVar,
	CmdIncMapVar,
	CmdIncWorldVar,
	CmdDecScriptVar,
	CmdDecMapVar,
	CmdDecWorldVar,
	CmdGoto,
	CmdIfGoto,
	CmdDrop,
	CmdDelay,
	CmdDelayDirect,
	CmdRandom,
	CmdRandomDirect,
	CmdThingCount,
	CmdThingCountDirect,
	CmdTagWait,
	CmdTagWaitDirect,
	CmdPolyWait,
	CmdPolyWaitDirect,
	CmdChangeFloor,
	CmdChangeFloorDirect,
	CmdChangeCeiling,
	CmdChangeCeilingDirect,
	CmdRestart,
	CmdAndLogical,
	CmdOrLogical,
	CmdAndBitwise,
	CmdOrBitwise,
	CmdEorBitwise,
	CmdNegateLogical,
	CmdLShift,
	CmdRShift,
	CmdUnaryMinus,
	CmdIfNotGoto,
	CmdLineSide,
	CmdScriptWait,
	CmdScriptWaitDirect,
	CmdClearLineSpecial,
	CmdCaseGoto,
	CmdBeginPrint,
	CmdEndPrint,
	CmdPrintString,
	CmdPrintNumber,
	CmdPrintCharacter,
	CmdPlayerCount,
	CmdGameType,
	CmdGameSkill,
	CmdTimer,
	CmdSectorSound,
	CmdAmbientSound,
	CmdSoundSequence,
	CmdSetLineTexture,
	CmdSetLineBlocking,
	CmdSetLineSpecial,
	CmdThingSound,
	CmdEndPrintBold
};

// CODE --------------------------------------------------------------------

static inline int32_t ReadCodePtr (void)
{
	uint32_t val = READ_INT32(PCodePtr);
	return (int32_t) val;
}

static inline void IncrCodePtr (void)
{
	INCR_INT32(PCodePtr);
}

static inline int32_t ReadAndIncrCodePtr (void)
{
	uint32_t val = READ_INT32(PCodePtr);
	INCR_INT32(PCodePtr);
	return (int32_t) val;
}

//==========================================================================
//
// P_LoadACScripts
//
//==========================================================================

void P_LoadACScripts(int lump)
{
	int i;
	int *buffer;
	acsHeader_t *header;
	acsInfo_t *info;

	header = (acsHeader_t *) W_CacheLumpNum(lump, PU_LEVEL);
	ActionCodeBase = (byte *)header;
	buffer = (int *)((byte *)header + LONG(header->infoOffset));
	ACScriptCount = LONG(*buffer++);
	if (ACScriptCount == 0)
	{ // Empty behavior lump
		return;
	}
	ACSInfo = (acsInfo_t *) Z_Malloc(ACScriptCount*sizeof(acsInfo_t), PU_LEVEL, NULL);
	memset(ACSInfo, 0, ACScriptCount*sizeof(acsInfo_t));
	for (i = 0, info = ACSInfo; i < ACScriptCount; i++, info++)
	{
		info->number = LONG(*buffer++);
		info->address = ActionCodeBase + LONG(*buffer++);
		info->argCount = LONG(*buffer++);
		if (info->number >= OPEN_SCRIPTS_BASE)
		{ // Auto-activate
			info->number -= OPEN_SCRIPTS_BASE;
			StartOpenACS(info->number, i, info->address);
			info->state = ASTE_RUNNING;
		}
		else
		{
			info->state = ASTE_INACTIVE;
		}
	}
	ACStringCount = LONG(*buffer++);
	ACStrings = (char **) Z_Malloc(ACStringCount*sizeof(char *), PU_LEVEL, NULL);
	for (i = 0; i < ACStringCount; i++)
	{
		ACStrings[i] = (char *)ActionCodeBase + LONG(buffer[i]);
	}
	memset(MapVars, 0, sizeof(MapVars));
}

//==========================================================================
//
// StartOpenACS
//
//==========================================================================

static void StartOpenACS(int number, int infoIndex, byte *address)
{
	acs_t *script;

	script = (acs_t *) Z_Malloc(sizeof(acs_t), PU_LEVSPEC, NULL);
	memset(script, 0, sizeof(acs_t));
	script->number = number;

	// World objects are allotted 1 second for initialization
	script->delayCount = 35;

	script->infoIndex = infoIndex;
	script->ip = address;
	script->thinker.function = T_InterpretACS;
	P_AddThinker(&script->thinker);
}

//==========================================================================
//
// P_CheckACSStore
//
// Scans the ACS store and executes all scripts belonging to the current
// map.
//
//==========================================================================

void P_CheckACSStore(void)
{
	acsstore_t *store;

	for (store = ACSStore; store->map != 0; store++)
	{
		if (store->map == gamemap)
		{
			P_StartACS(store->script, 0, store->args, NULL, NULL, 0);
			if (NewScript)
			{
				NewScript->delayCount = 35;
			}
			store->map = -1;
		}
	}
}

//==========================================================================
//
// P_StartACS
//
//==========================================================================

static char ErrorMsg[128];

BOOLEAN P_StartACS(int number, int map, byte *args, mobj_t *activator,
	line_t *line, int side)
{
	int i;
	acs_t *script;
	int infoIndex;
	aste_t *statePtr;

	NewScript = NULL;
	if (map && map != gamemap)
	{ // Add to the script store
		return AddToACSStore(map, number, args);
	}
	infoIndex = GetACSIndex(number);
	if (infoIndex == -1)
	{ // Script not found
		//I_Error("P_StartACS: Unknown script number %d", number);
		snprintf(ErrorMsg, sizeof(ErrorMsg), "P_STARTACS ERROR: UNKNOWN SCRIPT %d", number);
		P_SetMessage(&players[consoleplayer], ErrorMsg, true);
	}
	statePtr = &ACSInfo[infoIndex].state;
	if (*statePtr == ASTE_SUSPENDED)
	{ // Resume a suspended script
		*statePtr = ASTE_RUNNING;
		return true;
	}
	if (*statePtr != ASTE_INACTIVE)
	{ // Script is already executing
		return false;
	}
	script = (acs_t *) Z_Malloc(sizeof(acs_t), PU_LEVSPEC, NULL);
	memset(script, 0, sizeof(acs_t));
	script->number = number;
	script->infoIndex = infoIndex;
	script->activator = activator;
	script->line = line;
	script->side = side;
	script->ip = ACSInfo[infoIndex].address;
	script->thinker.function = T_InterpretACS;
	for (i = 0; i < ACSInfo[infoIndex].argCount; i++)
	{
		script->vars[i] = args[i];
	}
	*statePtr = ASTE_RUNNING;
	P_AddThinker(&script->thinker);
	NewScript = script;
	return true;
}

//==========================================================================
//
// AddToACSStore
//
//==========================================================================

static BOOLEAN AddToACSStore(int map, int number, byte *args)
{
	int i;
	int idx;

	idx = -1;
	for (i = 0; ACSStore[i].map != 0; i++)
	{
		if (ACSStore[i].script == number && ACSStore[i].map == map)
		{ // Don't allow duplicates
			return false;
		}
		if (idx == -1 && ACSStore[i].map == -1)
		{ // Remember first empty slot
			idx = i;
		}
	}
	if (idx == -1)
	{ // Append required
		if (i == MAX_ACS_STORE)
		{
			I_Error("AddToACSStore: MAX_ACS_STORE (%d) exceeded.",
				MAX_ACS_STORE);
		}
		idx = i;
		ACSStore[idx + 1].map = 0;
	}
	ACSStore[idx].map = map;
	ACSStore[idx].script = number;
//	*((int *)ACSStore[idx].args) = *((int *)args);
	ACSStore[idx].args[0] = args[0];
	ACSStore[idx].args[1] = args[1];
	ACSStore[idx].args[2] = args[2];
	ACSStore[idx].args[3] = 0;	/* unused */
	return true;
}

//==========================================================================
//
// P_StartLockedACS
//
//==========================================================================

BOOLEAN P_StartLockedACS(line_t *line, byte *args, mobj_t *mo, int side)
{
	int i;
	int lock;
	byte newArgs[5];
	char LockedBuffer[80];

	lock = args[4];
	if (!mo->player)
	{
		return false;
	}
	if (lock)
	{
		if (!(mo->player->keys & (1<<(lock-1))))
		{
			snprintf(LockedBuffer, sizeof(LockedBuffer),
				 "YOU NEED THE %s\n", TextKeyMessages[lock-1]);
			P_SetMessage(mo->player, LockedBuffer, true);
			S_StartSound(mo, SFX_DOOR_LOCKED);
			return false;
		}
	}
	for (i = 0; i < 4; i++)
	{
		newArgs[i] = args[i];
	}
	newArgs[4] = 0;
	return P_StartACS(newArgs[0], newArgs[1], &newArgs[2], mo, line, side);
}

//==========================================================================
//
// P_TerminateACS
//
//==========================================================================

BOOLEAN P_TerminateACS(int number, int map)
{
	int infoIndex;

	infoIndex = GetACSIndex(number);
	if (infoIndex == -1)
	{ // Script not found
		return false;
	}
	if (ACSInfo[infoIndex].state == ASTE_INACTIVE
		|| ACSInfo[infoIndex].state == ASTE_TERMINATING)
	{ // States that disallow termination
		return false;
	}
	ACSInfo[infoIndex].state = ASTE_TERMINATING;
	return true;
}

//==========================================================================
//
// P_SuspendACS
//
//==========================================================================

BOOLEAN P_SuspendACS(int number, int map)
{
	int infoIndex;

	infoIndex = GetACSIndex(number);
	if (infoIndex == -1)
	{ // Script not found
		return false;
	}
	if (ACSInfo[infoIndex].state == ASTE_INACTIVE
		|| ACSInfo[infoIndex].state == ASTE_SUSPENDED
		|| ACSInfo[infoIndex].state == ASTE_TERMINATING)
	{ // States that disallow suspension
		return false;
	}
	ACSInfo[infoIndex].state = ASTE_SUSPENDED;
	return true;
}

//==========================================================================
//
// P_Init
//
//==========================================================================

void P_ACSInitNewGame(void)
{
	memset(WorldVars, 0, sizeof(WorldVars));
	memset(ACSStore, 0, sizeof(ACSStore));
}

//==========================================================================
//
// T_InterpretACS
//
//==========================================================================

void T_InterpretACS(acs_t *script)
{
	int cmd;
	int action;

	if (ACSInfo[script->infoIndex].state == ASTE_TERMINATING)
	{
		ACSInfo[script->infoIndex].state = ASTE_INACTIVE;
		ScriptFinished(ACScript->number);
		P_RemoveThinker(&ACScript->thinker);
		return;
	}
	if (ACSInfo[script->infoIndex].state != ASTE_RUNNING)
	{
		return;
	}
	if (script->delayCount)
	{
		script->delayCount--;
		return;
	}
	ACScript = script;
	PCodePtr = ACScript->ip;
	do
	{
		cmd = ReadAndIncrCodePtr();
		action = PCodeCmds[cmd]();
	} while (action == SCRIPT_CONTINUE);
	ACScript->ip = PCodePtr;
	if (action == SCRIPT_TERMINATE)
	{
		ACSInfo[script->infoIndex].state = ASTE_INACTIVE;
		ScriptFinished(ACScript->number);
		P_RemoveThinker(&ACScript->thinker);
	}
}

//==========================================================================
//
// P_TagFinished
//
//==========================================================================

void P_TagFinished(int tag)
{
	int i;

	if (TagBusy(tag) == true)
	{
		return;
	}
	for (i = 0; i < ACScriptCount; i++)
	{
		if (ACSInfo[i].state == ASTE_WAITINGFORTAG
			&& ACSInfo[i].waitValue == tag)
		{
			ACSInfo[i].state = ASTE_RUNNING;
		}
	}
}

//==========================================================================
//
// P_PolyobjFinished
//
//==========================================================================

void P_PolyobjFinished(int po)
{
	int i;

	if (PO_Busy(po) == true)
	{
		return;
	}
	for (i = 0; i < ACScriptCount; i++)
	{
		if (ACSInfo[i].state == ASTE_WAITINGFORPOLY
			&& ACSInfo[i].waitValue == po)
		{
			ACSInfo[i].state = ASTE_RUNNING;
		}
	}
}

//==========================================================================
//
// ScriptFinished
//
//==========================================================================

static void ScriptFinished(int number)
{
	int i;

	for (i = 0; i < ACScriptCount; i++)
	{
		if (ACSInfo[i].state == ASTE_WAITINGFORSCRIPT
			&& ACSInfo[i].waitValue == number)
		{
			ACSInfo[i].state = ASTE_RUNNING;
		}
	}
}

//==========================================================================
//
// TagBusy
//
//==========================================================================

static BOOLEAN TagBusy(int tag)
{
	int sectorIndex;

	sectorIndex = -1;
	while ((sectorIndex = P_FindSectorFromTag(tag, sectorIndex)) >= 0)
	{
		if (sectors[sectorIndex].specialdata)
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
// GetACSIndex
//
// Returns the index of a script number.  Returns -1 if the script number
// is not found.
//
//==========================================================================

static int GetACSIndex(int number)
{
	int i;

	for (i = 0; i < ACScriptCount; i++)
	{
		if (ACSInfo[i].number == number)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
// Push
//
//==========================================================================

static void Push(int value)
{
	ACScript->stack[ACScript->stackPtr++] = value;
}

//==========================================================================
//
// Pop
//
//==========================================================================

static int Pop(void)
{
	return ACScript->stack[--ACScript->stackPtr];
}

//==========================================================================
//
// Top
//
//==========================================================================

static int Top(void)
{
	return ACScript->stack[ACScript->stackPtr - 1];
}

//==========================================================================
//
// Drop
//
//==========================================================================

static void Drop(void)
{
	ACScript->stackPtr--;
}

//==========================================================================
//
// P-Code Commands
//
//==========================================================================

static int CmdNOP(void)
{
	return SCRIPT_CONTINUE;
}

static int CmdTerminate(void)
{
	return SCRIPT_TERMINATE;
}

static int CmdSuspend(void)
{
	ACSInfo[ACScript->infoIndex].state = ASTE_SUSPENDED;
	return SCRIPT_STOP;
}

static int CmdPushNumber(void)
{
	Push(ReadAndIncrCodePtr());
	return SCRIPT_CONTINUE;
}

static int CmdLSpec1(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[0] = Pop();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec2(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[1] = Pop();
	SpecArgs[0] = Pop();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec3(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[2] = Pop();
	SpecArgs[1] = Pop();
	SpecArgs[0] = Pop();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec4(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[3] = Pop();
	SpecArgs[2] = Pop();
	SpecArgs[1] = Pop();
	SpecArgs[0] = Pop();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec5(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[4] = Pop();
	SpecArgs[3] = Pop();
	SpecArgs[2] = Pop();
	SpecArgs[1] = Pop();
	SpecArgs[0] = Pop();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec1Direct(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[0] = ReadAndIncrCodePtr();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec2Direct(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[0] = ReadAndIncrCodePtr();
	SpecArgs[1] = ReadAndIncrCodePtr();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec3Direct(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[0] = ReadAndIncrCodePtr();
	SpecArgs[1] = ReadAndIncrCodePtr();
	SpecArgs[2] = ReadAndIncrCodePtr();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec4Direct(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[0] = ReadAndIncrCodePtr();
	SpecArgs[1] = ReadAndIncrCodePtr();
	SpecArgs[2] = ReadAndIncrCodePtr();
	SpecArgs[3] = ReadAndIncrCodePtr();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdLSpec5Direct(void)
{
	int special;

	special = ReadAndIncrCodePtr();
	SpecArgs[0] = ReadAndIncrCodePtr();
	SpecArgs[1] = ReadAndIncrCodePtr();
	SpecArgs[2] = ReadAndIncrCodePtr();
	SpecArgs[3] = ReadAndIncrCodePtr();
	SpecArgs[4] = ReadAndIncrCodePtr();
	P_ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			ACScript->side, ACScript->activator);
	return SCRIPT_CONTINUE;
}

static int CmdAdd(void)
{
	Push (Pop() + Pop());
	return SCRIPT_CONTINUE;
}

static int CmdSubtract(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() - operand2);
	return SCRIPT_CONTINUE;
}

static int CmdMultiply(void)
{
	Push (Pop() * Pop());
	return SCRIPT_CONTINUE;
}

static int CmdDivide(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() / operand2);
	return SCRIPT_CONTINUE;
}

static int CmdModulus(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() % operand2);
	return SCRIPT_CONTINUE;
}

static int CmdEQ(void)
{
	Push (Pop() == Pop());
	return SCRIPT_CONTINUE;
}

static int CmdNE(void)
{
	Push (Pop() != Pop());
	return SCRIPT_CONTINUE;
}

static int CmdLT(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() < operand2);
	return SCRIPT_CONTINUE;
}

static int CmdGT(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() > operand2);
	return SCRIPT_CONTINUE;
}

static int CmdLE(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() <= operand2);
	return SCRIPT_CONTINUE;
}

static int CmdGE(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() >= operand2);
	return SCRIPT_CONTINUE;
}

static int CmdAssignScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()] = Pop();
	return SCRIPT_CONTINUE;
}

static int CmdAssignMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()] = Pop();
	return SCRIPT_CONTINUE;
}

static int CmdAssignWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()] = Pop();
	return SCRIPT_CONTINUE;
}

static int CmdPushScriptVar(void)
{
	Push(ACScript->vars[ReadAndIncrCodePtr()]);
	return SCRIPT_CONTINUE;
}

static int CmdPushMapVar(void)
{
	Push(MapVars[ReadAndIncrCodePtr()]);
	return SCRIPT_CONTINUE;
}

static int CmdPushWorldVar(void)
{
	Push(WorldVars[ReadAndIncrCodePtr()]);
	return SCRIPT_CONTINUE;
}

static int CmdAddScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()] += Pop();
	return SCRIPT_CONTINUE;
}

static int CmdAddMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()] += Pop();
	return SCRIPT_CONTINUE;
}

static int CmdAddWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()] += Pop();
	return SCRIPT_CONTINUE;
}

static int CmdSubScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()] -= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdSubMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()] -= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdSubWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()] -= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdMulScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()] *= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdMulMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()] *= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdMulWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()] *= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdDivScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()] /= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdDivMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()] /= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdDivWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()] /= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdModScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()] %= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdModMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()] %= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdModWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()] %= Pop();
	return SCRIPT_CONTINUE;
}

static int CmdIncScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()]++;
	return SCRIPT_CONTINUE;
}

static int CmdIncMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()]++;
	return SCRIPT_CONTINUE;
}

static int CmdIncWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()]++;
	return SCRIPT_CONTINUE;
}

static int CmdDecScriptVar(void)
{
	ACScript->vars[ReadAndIncrCodePtr()]--;
	return SCRIPT_CONTINUE;
}

static int CmdDecMapVar(void)
{
	MapVars[ReadAndIncrCodePtr()]--;
	return SCRIPT_CONTINUE;
}

static int CmdDecWorldVar(void)
{
	WorldVars[ReadAndIncrCodePtr()]--;
	return SCRIPT_CONTINUE;
}

static int CmdGoto(void)
{
	PCodePtr = ActionCodeBase + ReadCodePtr();
	return SCRIPT_CONTINUE;
}

static int CmdIfGoto(void)
{
	if (Pop())
	{
		PCodePtr = ActionCodeBase + ReadCodePtr();
	}
	else
	{
		IncrCodePtr();
	}
	return SCRIPT_CONTINUE;
}

static int CmdDrop(void)
{
	Drop();
	return SCRIPT_CONTINUE;
}

static int CmdDelay(void)
{
	ACScript->delayCount = Pop();
	return SCRIPT_STOP;
}

static int CmdDelayDirect(void)
{
	ACScript->delayCount = ReadAndIncrCodePtr();
	return SCRIPT_STOP;
}

static int CmdRandom(void)
{
	int low;
	int high;

	high = Pop();
	low = Pop();
	Push (low + (P_Random() % (high - low + 1)));
	return SCRIPT_CONTINUE;
}

static int CmdRandomDirect(void)
{
	int low;
	int high;

	low = ReadAndIncrCodePtr();
	high = ReadAndIncrCodePtr();
	Push (low + (P_Random() % (high - low + 1)));
	return SCRIPT_CONTINUE;
}

static int CmdThingCount(void)
{
	int tid;

	tid = Pop();
	ThingCount(Pop(), tid);
	return SCRIPT_CONTINUE;
}

static int CmdThingCountDirect(void)
{
	int type;

	type = ReadAndIncrCodePtr();
	ThingCount(type, ReadAndIncrCodePtr());
	return SCRIPT_CONTINUE;
}

static void ThingCount(int type, int tid)
{
	int count;
	int searcher;
	mobj_t *mobj;
	mobjtype_t moType;
	thinker_t *think;

	if (!(type + tid))
	{ // Nothing to count
		return;
	}
	moType = TranslateThingType[type];
	count = 0;
	searcher = -1;
	if (tid)
	{ // Count TID things
		while ((mobj = P_FindMobjFromTID(tid, &searcher)) != NULL)
		{
			if (type == 0)
			{ // Just count TIDs
				count++;
			}
			else if (moType == mobj->type)
			{
				if (mobj->flags & MF_COUNTKILL && mobj->health <= 0)
				{ // Don't count dead monsters
					continue;
				}
				count++;
			}
		}
	}
	else
	{ // Count only types
		for (think = thinkercap.next; think != &thinkercap;
						think = think->next)
		{
			if (think->function != P_MobjThinker)
			{ // Not a mobj thinker
				continue;
			}
			mobj = (mobj_t *)think;
			if (mobj->type != moType)
			{ // Doesn't match
				continue;
			}
			if (mobj->flags & MF_COUNTKILL && mobj->health <= 0)
			{ // Don't count dead monsters
				continue;
			}
			count++;
		}
	}
	Push(count);
}

static int CmdTagWait(void)
{
	ACSInfo[ACScript->infoIndex].waitValue = Pop();
	ACSInfo[ACScript->infoIndex].state = ASTE_WAITINGFORTAG;
	return SCRIPT_STOP;
}

static int CmdTagWaitDirect(void)
{
	ACSInfo[ACScript->infoIndex].waitValue = ReadAndIncrCodePtr();
	ACSInfo[ACScript->infoIndex].state = ASTE_WAITINGFORTAG;
	return SCRIPT_STOP;
}

static int CmdPolyWait(void)
{
	ACSInfo[ACScript->infoIndex].waitValue = Pop();
	ACSInfo[ACScript->infoIndex].state = ASTE_WAITINGFORPOLY;
	return SCRIPT_STOP;
}

static int CmdPolyWaitDirect(void)
{
	ACSInfo[ACScript->infoIndex].waitValue = ReadAndIncrCodePtr();
	ACSInfo[ACScript->infoIndex].state = ASTE_WAITINGFORPOLY;
	return SCRIPT_STOP;
}

static int CmdChangeFloor(void)
{
	int tag;
	int flat;
	int sectorIndex;

	flat = R_FlatNumForName(ACStrings[Pop()]);
	tag = Pop();
	sectorIndex = -1;
	while ((sectorIndex = P_FindSectorFromTag(tag, sectorIndex)) >= 0)
	{
		sectors[sectorIndex].floorpic = flat;
	}
	return SCRIPT_CONTINUE;
}

static int CmdChangeFloorDirect(void)
{
	int tag;
	int flat;
	int sectorIndex;

	tag = ReadAndIncrCodePtr();
	flat = R_FlatNumForName(ACStrings[ReadAndIncrCodePtr()]);
	sectorIndex = -1;
	while ((sectorIndex = P_FindSectorFromTag(tag, sectorIndex)) >= 0)
	{
		sectors[sectorIndex].floorpic = flat;
	}
	return SCRIPT_CONTINUE;
}

static int CmdChangeCeiling(void)
{
	int tag;
	int flat;
	int sectorIndex;

	flat = R_FlatNumForName(ACStrings[Pop()]);
	tag = Pop();
	sectorIndex = -1;
	while ((sectorIndex = P_FindSectorFromTag(tag, sectorIndex)) >= 0)
	{
		sectors[sectorIndex].ceilingpic = flat;
	}
	return SCRIPT_CONTINUE;
}

static int CmdChangeCeilingDirect(void)
{
	int tag;
	int flat;
	int sectorIndex;

	tag = ReadAndIncrCodePtr();
	flat = R_FlatNumForName(ACStrings[ReadAndIncrCodePtr()]);
	sectorIndex = -1;
	while ((sectorIndex = P_FindSectorFromTag(tag, sectorIndex)) >= 0)
	{
		sectors[sectorIndex].ceilingpic = flat;
	}
	return SCRIPT_CONTINUE;
}

static int CmdRestart(void)
{
	PCodePtr = ACSInfo[ACScript->infoIndex].address;
	return SCRIPT_CONTINUE;
}

static int CmdAndLogical(void)
{
	Push (Pop() && Pop());
	return SCRIPT_CONTINUE;
}

static int CmdOrLogical(void)
{
	Push (Pop() || Pop());
	return SCRIPT_CONTINUE;
}

static int CmdAndBitwise(void)
{
	Push (Pop() & Pop());
	return SCRIPT_CONTINUE;
}

static int CmdOrBitwise(void)
{
	Push (Pop() | Pop());
	return SCRIPT_CONTINUE;
}

static int CmdEorBitwise(void)
{
	Push (Pop() ^ Pop());
	return SCRIPT_CONTINUE;
}

static int CmdNegateLogical(void)
{
	Push (!Pop());
	return SCRIPT_CONTINUE;
}

static int CmdLShift(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() << operand2);
	return SCRIPT_CONTINUE;
}

static int CmdRShift(void)
{
	int operand2;

	operand2 = Pop();
	Push (Pop() >> operand2);
	return SCRIPT_CONTINUE;
}

static int CmdUnaryMinus(void)
{
	Push (-Pop());
	return SCRIPT_CONTINUE;
}

static int CmdIfNotGoto(void)
{
	if (Pop())
	{
		IncrCodePtr();
	}
	else
	{
		PCodePtr = ActionCodeBase + ReadCodePtr();
	}
	return SCRIPT_CONTINUE;
}

static int CmdLineSide(void)
{
	Push(ACScript->side);
	return SCRIPT_CONTINUE;
}

static int CmdScriptWait(void)
{
	ACSInfo[ACScript->infoIndex].waitValue = Pop();
	ACSInfo[ACScript->infoIndex].state = ASTE_WAITINGFORSCRIPT;
	return SCRIPT_STOP;
}

static int CmdScriptWaitDirect(void)
{
	ACSInfo[ACScript->infoIndex].waitValue = ReadAndIncrCodePtr();
	ACSInfo[ACScript->infoIndex].state = ASTE_WAITINGFORSCRIPT;
	return SCRIPT_STOP;
}

static int CmdClearLineSpecial(void)
{
	if (ACScript->line)
	{
		ACScript->line->special = 0;
	}
	return SCRIPT_CONTINUE;
}

static int CmdCaseGoto(void)
{
	if (Top() == ReadAndIncrCodePtr())
	{
		PCodePtr = ActionCodeBase + ReadCodePtr();
		Drop();
	}
	else
	{
		IncrCodePtr();
	}
	return SCRIPT_CONTINUE;
}

static int CmdBeginPrint(void)
{
	*PrintBuffer = 0;
	return SCRIPT_CONTINUE;
}

static int CmdEndPrint(void)
{
	player_t *player;

	if (ACScript->activator && ACScript->activator->player)
	{
		player = ACScript->activator->player;
	}
	else
	{
		player = &players[consoleplayer];
	}
	P_SetMessage(player, PrintBuffer, true);
	return SCRIPT_CONTINUE;
}

static int CmdEndPrintBold(void)
{
	int i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			P_SetYellowMessage(&players[i], PrintBuffer, true);
		}
	}
	return SCRIPT_CONTINUE;
}

static int CmdPrintString(void)
{
	strcat(PrintBuffer, ACStrings[Pop()]);
	return SCRIPT_CONTINUE;
}

static int CmdPrintNumber(void)
{
	char tempStr[16];

	snprintf(tempStr, sizeof(tempStr), "%d", Pop());
	strcat(PrintBuffer, tempStr);
	return SCRIPT_CONTINUE;
}

static int CmdPrintCharacter(void)
{
	char *bufferEnd;

	bufferEnd = PrintBuffer + strlen(PrintBuffer);
	*bufferEnd++ = Pop();
	*bufferEnd = 0;
	return SCRIPT_CONTINUE;
}

static int CmdPlayerCount(void)
{
	int i;
	int count;

	count = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		count += playeringame[i];
	}
	Push(count);
	return SCRIPT_CONTINUE;
}

static int CmdGameType(void)
{
	int gametype;

	if (netgame == false)
	{
		gametype = GAME_SINGLE_PLAYER;
	}
	else if (deathmatch)
	{
		gametype = GAME_NET_DEATHMATCH;
	}
	else
	{
		gametype = GAME_NET_COOPERATIVE;
	}
	Push(gametype);
	return SCRIPT_CONTINUE;
}

static int CmdGameSkill(void)
{
	Push(gameskill);
	return SCRIPT_CONTINUE;
}

static int CmdTimer(void)
{
	Push(leveltime);
	return SCRIPT_CONTINUE;
}

static int CmdSectorSound(void)
{
	int volume;
	mobj_t *mobj;

	mobj = NULL;
	if (ACScript->line)
	{
		mobj = (mobj_t *)(void *)&ACScript->line->frontsector->soundorg;
	}
	volume = Pop();
	S_StartSoundAtVolume(mobj, S_GetSoundID(ACStrings[Pop()]), volume);
	return SCRIPT_CONTINUE;
}

static int CmdThingSound(void)
{
	int tid;
	int sound;
	int volume;
	mobj_t *mobj;
	int searcher;

	volume = Pop();
	sound = S_GetSoundID(ACStrings[Pop()]);
	tid = Pop();
	searcher = -1;
	while ((mobj = P_FindMobjFromTID(tid, &searcher)) != NULL)
	{
		S_StartSoundAtVolume(mobj, sound, volume);
	}
	return SCRIPT_CONTINUE;
}

static int CmdAmbientSound(void)
{
	int volume;

	volume = Pop();
	S_StartSoundAtVolume(NULL, S_GetSoundID(ACStrings[Pop()]), volume);
	return SCRIPT_CONTINUE;
}

static int CmdSoundSequence(void)
{
	mobj_t *mobj;

	mobj = NULL;
	if (ACScript->line)
	{
		mobj = (mobj_t *)(void *)&ACScript->line->frontsector->soundorg;
	}
	SN_StartSequenceName(mobj, ACStrings[Pop()]);
	return SCRIPT_CONTINUE;
}

static int CmdSetLineTexture(void)
{
	line_t *line;
	int lineTag;
	int side;
	int position;
	int texture;
	int searcher;

	texture = R_TextureNumForName(ACStrings[Pop()]);
	position = Pop();
	side = Pop();
	lineTag = Pop();
	searcher = -1;
	while ((line = P_FindLine(lineTag, &searcher)) != NULL)
	{
		if (position == TEXTURE_MIDDLE)
		{
			sides[line->sidenum[side]].midtexture = texture;
		}
		else if (position == TEXTURE_BOTTOM)
		{
			sides[line->sidenum[side]].bottomtexture = texture;
		}
		else
		{ // TEXTURE_TOP
			sides[line->sidenum[side]].toptexture = texture;
		}
	}
	return SCRIPT_CONTINUE;
}

static int CmdSetLineBlocking(void)
{
	line_t *line;
	int lineTag;
	BOOLEAN blocking;
	int searcher;

	blocking = Pop() ? ML_BLOCKING : 0;
	lineTag = Pop();
	searcher = -1;
	while ((line = P_FindLine(lineTag, &searcher)) != NULL)
	{
		line->flags = (line->flags & ~ML_BLOCKING) | blocking;
	}
	return SCRIPT_CONTINUE;
}

static int CmdSetLineSpecial(void)
{
	line_t *line;
	int lineTag;
	int special, arg1, arg2, arg3, arg4, arg5;
	int searcher;

	arg5 = Pop();
	arg4 = Pop();
	arg3 = Pop();
	arg2 = Pop();
	arg1 = Pop();
	special = Pop();
	lineTag = Pop();
	searcher = -1;
	while ((line = P_FindLine(lineTag, &searcher)) != NULL)
	{
		line->special = special;
		line->arg1 = arg1;
		line->arg2 = arg2;
		line->arg3 = arg3;
		line->arg4 = arg4;
		line->arg5 = arg5;
	}
	return SCRIPT_CONTINUE;
}

