// DoomData.h
// $Revision: 535 $
// $Date: 2009-08-24 09:16:53 +0300 (Mon, 24 Aug 2009) $

/* all external data is defined here
 * most of the data is loaded into different structures at run time
 */

#ifndef __DOOMDATA__
#define __DOOMDATA__

/* ---- Map level types ---- */

/* lump order in a map wad */
enum
{
	ML_LABEL,
	ML_THINGS,
	ML_LINEDEFS,
	ML_SIDEDEFS,
	ML_VERTEXES,
	ML_SEGS,
	ML_SSECTORS,
	ML_NODES,
	ML_SECTORS,
	ML_REJECT,
	ML_BLOCKMAP
};

typedef struct
{
	short		x, y;
} mapvertex_t;
COMPILE_TIME_ASSERT(mapvertex_t, sizeof(mapvertex_t) == 4);

typedef struct
{
	short		textureoffset;
	short		rowoffset;
	char		toptexture[8], bottomtexture[8], midtexture[8];
	short		sector;		/* on viewer's side */
} __attribute__((packed)) mapsidedef_t;
COMPILE_TIME_ASSERT(mapsidedef_t, sizeof(mapsidedef_t) == 30);

typedef struct
{
	short		v1, v2;
	short		flags;
	short		special, tag;
	short		sidenum[2];	/* sidenum[1] will be -1 if one sided */
} __attribute__((packed)) maplinedef_t;
COMPILE_TIME_ASSERT(maplinedef_t, sizeof(maplinedef_t) == 14);

#define	ML_BLOCKING			1
#define	ML_BLOCKMONSTERS		2
#define	ML_TWOSIDED			4	/* backside will not be present at all */
								/* if not two sided */

/* if a texture is pegged, the texture will have the end exposed to air held
 * constant at the top or bottom of the texture (stairs or pulled down things)
 * and will move with a height change of one of the neighbor sectors
 * Unpegged textures allways have the first row of the texture at the top
 * pixel of the line for both top and bottom textures (windows)
 */
#define	ML_DONTPEGTOP			8
#define	ML_DONTPEGBOTTOM		16

#define	ML_SECRET			32	/* don't map as two sided: IT'S A SECRET! */
#define	ML_SOUNDBLOCK			64	/* don't let sound cross two of these */
#define	ML_DONTDRAW			128	/* don't draw on the automap */
#define	ML_MAPPED			256	/* set if allready drawn in automap */

typedef	struct
{
	short		floorheight, ceilingheight;
	char		floorpic[8], ceilingpic[8];
	short		lightlevel;
	short		special, tag;
} __attribute__((packed)) mapsector_t;
COMPILE_TIME_ASSERT(mapsector_t, sizeof(mapsector_t) == 26);

typedef struct
{
	short		numsegs;
	short		firstseg;	/* segs are stored sequentially */
} mapsubsector_t;
COMPILE_TIME_ASSERT(mapsubsector_t, sizeof(mapsubsector_t) == 4);

typedef struct
{
	short		v1, v2;
	short		angle;
	short		linedef, side;
	short		offset;
} mapseg_t;
COMPILE_TIME_ASSERT(mapseg_t, sizeof(mapseg_t) == 12);

/* bbox coordinates */
enum
{
	BOXTOP,
	BOXBOTTOM,
	BOXLEFT,
	BOXRIGHT
};

#define	NF_SUBSECTOR	0x8000
typedef struct
{
	short		x, y, dx, dy;	/* partition line */
	short		bbox[2][4];	/* bounding box for each child */
	unsigned short	children[2];	/* if NF_SUBSECTOR its a subsector */
} mapnode_t;
COMPILE_TIME_ASSERT(mapnode_t, sizeof(mapnode_t) == 28);

typedef struct
{
	short		x, y;
	short		angle;
	short		type;
	short		options;
} __attribute__((packed)) mapthing_t;
COMPILE_TIME_ASSERT(mapthing_t, sizeof(mapthing_t) == 10);

#define	MTF_EASY		1
#define	MTF_NORMAL		2
#define	MTF_HARD		4
#define	MTF_AMBUSH		8

/* ---- Texture definition ---- */

typedef struct
{
	short		originx;
	short		originy;
	short		patch;
	short		stepdir;
	short		colormap;
} __attribute__((packed)) mappatch_t;
COMPILE_TIME_ASSERT(mappatch_t, sizeof(mappatch_t) == 10);

typedef struct
{
	char		name[8];
	BOOLEAN		masked;	
	short		width;
	short		height;
	int32_t		columndirectory;	/* OBSOLETE */
	short		patchcount;
	mappatch_t	patches[1];
} __attribute__((packed)) maptexture_t;
COMPILE_TIME_ASSERT(maptexture_t, sizeof(maptexture_t) == 32);


/* ---- Graphics ---- */

/* posts are runs of non masked source pixels */
typedef struct
{
	byte		topdelta;	/* -1 is the last post in a column */
	byte		length;
	/* length data bytes follows */
} __attribute__((packed)) post_t;
COMPILE_TIME_ASSERT(post_t, sizeof(post_t) == 2);

/* column_t is a list of 0 or more post_t, (byte)-1 terminated */
typedef post_t	column_t;

/* a patch holds one or more columns
 * patches are used for sprites and all masked pictures
 */
typedef struct
{
	short		width;			/* bounding box size */
	short		height;
	short		leftoffset;		/* pixels to the left of origin */
	short		topoffset;		/* pixels below the origin */
	int		columnofs[8];		/* only [width] used */
							/* the [0] is &columnofs[width] */
} patch_t;
COMPILE_TIME_ASSERT(patch_t, sizeof(patch_t) == 40);

#endif	/* __DOOMDATA__ */

