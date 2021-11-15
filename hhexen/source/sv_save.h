/*
	sv_save.h: Heretic 2 (Hexen)
	Structures used for saved games.

	$Revision: 522 $
	$Date: 2009-06-08 11:45:17 +0300 (Mon, 08 Jun 2009) $

	See the file SAVEGAME for notes and/or issues.
*/

#ifndef __SAVE_DEFS
#define __SAVE_DEFS

#ifndef _DOSSAVE_COMPAT
#define __compat_doshexen
#else
#define __compat_doshexen	__attribute__((packed))
#endif

typedef struct
{
	int32_t	state_idx;				/* state_t	*state */
	int	tics;
	fixed_t	sx, sy;
} save_pspdef_t;

typedef struct
{
	int32_t		prev_idx, next_idx;		/* struct thinker_s *prev, *next; */
	int32_t		function_idx;			/* think_t	function; */
} save_thinker_t;

typedef struct
{
	save_thinker_t		thinker;		/* thinker_t	thinker; */

	fixed_t			x, y, z;
	int32_t		snext_idx, sprev_idx;		/* struct mobj_s *snext, *sprev; */
	angle_t			angle;
	int			sprite;			/* spritenum_t	sprite */
	int			frame;

	int32_t		bnext_idx, bprev_idx;		/* struct mobj_s *bnext, *bprev; */
	int32_t		subsector_idx;			/* struct subsector_s *subsector; */
	fixed_t			floorz, ceilingz;
	fixed_t			floorpic;
	fixed_t			radius, height;
	fixed_t			momx, momy, momz;
	int			validcount;
	int			type;			/* mobjtype_t	type */
	int32_t			info_idx;		/* mobjinfo_t	*info; */
	int			tics;
	int32_t			state_idx;		/* state_t	*state; */
	int			damage;
	int			flags;
	int			flags2;
	int32_t			special1;		/* intptr_t	special1; */
	int32_t			special2;		/* intptr_t	special2; */
	int			health;
	int			movedir;
	int			movecount;
	int32_t			target_idx;		/* struct mobj_s *target; */
	int			reactiontime;
	int			threshold;
	int32_t			player_idx;		/* struct player_s *player; */
	int			lastlook;
	fixed_t			floorclip;
	int			archiveNum;
	short			tid;
	byte			special;
	byte			args[5];
} __compat_doshexen save_mobj_t;
#if !(defined(VERSION10_WAD) || defined(_DOSSAVE_COMPAT))
/* make sure the struct is of 176 bytes size, so that all our
   saved games are uniform. */
COMPILE_TIME_ASSERT(save_mobj_t, sizeof(save_mobj_t) == 176);
#endif

typedef struct
{
	int32_t		mo_idx;				/* mobj_t	*mo; */
	int		playerstate;			/* playerstate_t playerstate */
	ticcmd_t	cmd;	/* note: sizeof(ticcmd_t) is
				   10, not 4 byte aligned. */

	int		playerclass;			/* pclass_t	playerclass */

	fixed_t		viewz;
	fixed_t		viewheight;
	fixed_t		deltaviewheight;
	fixed_t		bob;

	int		flyheight;
	int		lookdir;
	int		centering;			/* BOOLEAN	centering */
	int		health;
	int		armorpoints[NUMARMOR];

	inventory_t	inventory[NUMINVENTORYSLOTS];
	int		readyArtifact;			/* artitype_t	readyArtifact */
	int		artifactCount;
	int		inventorySlotNum;
	int		powers[NUMPOWERS];
	int		keys;
	int		pieces;
	signed int	frags[MAXPLAYERS];
	int		readyweapon;			/* weapontype_t	readyweapon */
	int		pendingweapon;			/* weapontype_t	pendingweapon */
	int		weaponowned[NUMWEAPONS];	/* BOOLEAN	weaponowned[NUMWEAPONS] */
	int		mana[NUMMANA];
	int		attackdown, usedown;
	int		cheats;

	int		refire;

	int		killcount, itemcount, secretcount;
	char		message[80];
	int		messageTics;
	short		ultimateMessage;
	short		yellowMessage;
	int		damagecount, bonuscount;
	int		poisoncount;
	int32_t		poisoner_idx;			/* mobj_t	*poisoner; */
	int32_t		attacker_idx;			/* mobj_t	*attacker; */
	int		extralight;
	int		fixedcolormap;
	int		colormap;
	save_pspdef_t	psprites[NUMPSPRITES];		/* pspdef_t	psprites[NUMPSPRITES]; */
	int		morphTics;
	unsigned int	jumpTics;
	unsigned int	worldTimer;
} __compat_doshexen save_player_t;
#if !(defined(VERSION10_WAD) || defined(_DOSSAVE_COMPAT))
/* make sure the struct is of 648 bytes size, so that all our saved
   games are uniform: Raven's DOS versions seem to have this struct
   packed, with sizeof(player_t) == 646 and offsetof playerclass at
   18 instead of 20. */
COMPILE_TIME_ASSERT(save_player_1, sizeof(save_player_t) == 648);
COMPILE_TIME_ASSERT(save_player_2, offsetof(save_player_t,playerclass) == 20);
#endif

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	int		type;				/* floor_e	type; */
	int		crush;
	int		direction;
	int		newspecial;
	short		texture;		/*  */
	fixed_t		floordestheight;
	fixed_t		speed;
	int		delayCount;
	int		delayTotal;
	fixed_t		stairsDelayHeight;
	fixed_t		stairsDelayHeightDelta;
	fixed_t		resetHeight;
	short		resetDelay;
	short		resetDelayCount;
	byte		textureChange;		/*  */
} __compat_doshexen save_floormove_t;
#if !(defined(VERSION10_WAD) || defined(_DOSSAVE_COMPAT))
/* make sure the struct is of 72 bytes size, so that all our saved
   games are uniform. */
COMPILE_TIME_ASSERT(save_floormove_1, sizeof(save_floormove_t) == 72);
COMPILE_TIME_ASSERT(save_floormove_2, offsetof(save_floormove_t,floordestheight) == 36);
#endif

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	int		ceilingSpeed;
	int		floorSpeed;
	int		floordest;
	int		ceilingdest;
	int		direction;
	int		crush;
} save_pillar_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	fixed_t		originalHeight;
	fixed_t		accumulator;
	fixed_t		accDelta;
	fixed_t		targetScale;
	fixed_t		scale;
	fixed_t		scaleDelta;
	int		ticker;
	int		state;
} save_floorWaggle_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	fixed_t		speed;
	fixed_t		low;
	fixed_t		high;
	int		wait;
	int		count;
	int		status;				/* plat_e	status; */
	int		oldstatus;			/* plat_e	oldstatus; */
	int		crush;
	int		tag;
	int		type;				/* plattype_e	type; */
} save_plat_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	int		type;				/* ceiling_e	type; */
	fixed_t		bottomheight, topheight;
	fixed_t		speed;
	int		crush;
	int		direction;
	int		tag;
	int		olddirection;
} save_ceiling_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	int		type;				/* lighttype_t	type; */
	int		value1;
	int		value2;
	int		tics1;
	int		tics2;
	int		count;
} save_light_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	int		index;
	int		base;
} save_phase_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		sector_idx;			/* sector_t	*sector; */
	int		type;				/* vldoor_e	type; */
	fixed_t		topheight;
	fixed_t		speed;
	int		direction;
	int		topwait;
	int		topcountdown;
} save_vldoor_t;

typedef struct
{
	save_thinker_t thinker;				/* thinker_t thinker; */
	int polyobj;
	int speed;
	unsigned int dist;
	int angle;
	fixed_t xSpeed;
	fixed_t ySpeed;
} save_polyevent_t;

typedef struct
{
	save_thinker_t thinker;				/* thinker_t thinker; */
	int polyobj;
	int speed;
	int dist;
	int totalDist;
	int direction;
	fixed_t xSpeed, ySpeed;
	int tics;
	int waitTics;
	int type;					/* podoortype_t type; */
	int close;					/* BOOLEAN	close; */
} save_polydoor_t;

typedef struct
{
	save_thinker_t	thinker;			/* thinker_t	thinker; */
	int32_t		activator_idx;			/* mobj_t	*activator; */
	int32_t		line_idx;			/* line_t	*line; */
	int		side;
	int		number;
	int		infoIndex;
	int		delayCount;
	int		stack[ACS_STACK_DEPTH];
	int		stackPtr;
	int		vars[MAX_ACS_SCRIPT_VARS];
	int32_t		ip_idx;				/* byte		*ip; */
} save_acs_t;

#endif	/* __SAVE_DEFS */

