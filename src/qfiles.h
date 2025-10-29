/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

struct modelBone_t {
	vec3_t				vRotate[3];
	vec3_t				vTranslation;
	bool				bCalculate;
	int					nParentIndex; // TEST
};

struct modelDrawVertex_t {
	vec3_t		vPos;
	vec2_t		vTexCoord;
	vec3_t		vNormal;
};
/*
========================================================================

.MD2 triangle model file format

========================================================================
*/

#define IDALIASHEADER		(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define ALIAS_VERSION	8

#define	MAX_TRIANGLES	4096
#define MAX_VERTS		2048
#define MAX_FRAMES		512
#define MAX_MD2SKINS	32
#define	MAX_SKINNAME	64

typedef struct
{
	short	s;
	short	t;
} dstvert_t;

typedef struct 
{
	short	index_xyz[3];
	short	index_st[3];
} dtriangle_t;

typedef struct
{
	byte	v[3];			// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
} dtrivertx_t;

typedef struct
{
	float		scale[3];	// multiply byte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	dtrivertx_t	verts[1];	// variable sized
} daliasframe_t;


// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.


typedef struct
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// byte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MAX_SKINNAME string
	int			ofs_st;			// byte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;	
	int			ofs_end;		// end of file

} dmdl_t;

/*
========================================================================

.MD3 sprite file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MD3_XYZ_SCALE		(1.0/64)

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;			
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;

typedef struct {
	int		ident;				// 

	char	name[MAX_QPATH];	// polyset name

	int		flags;
	int		numFrames;			// all surfaces in a model should have the same

	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;

	int		numTriangles;
	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct {
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct {
	int			indexes[3];
} md3Triangle_t;

typedef struct {
	float		st[2];
} md3St_t;

typedef struct {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct {
	vec3_t mins;
	vec3_t maxs;
	vec3_t offset;
	float scale;
	char name[16];
} md3Frame_t;



typedef struct
{
  float st[2];
  int   nVertIndex;
} glst_t;

typedef struct
{
  int     nCount;
  int     ObjectIndex;
  glst_t  GlSt;
} gl_t;


/*
========================================================================

.MDC sprite file format

========================================================================
*/
#define MDC_IDENT           ( ( 'C' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MDC_VERSION         2

// version history:
// 1 - original
// 2 - changed tag structure so it only lists the names once

typedef struct {
	unsigned int ofsVec;                    // offset direction from the last base frame
	//	unsigned short	ofsVec;
} mdcXyzCompressed_t;

typedef struct {
	char name[MAX_QPATH];           // tag name
} mdcTagName_t;

#define MDC_TAG_ANGLE_SCALE ( 360.0 / 32700.0 )

typedef struct {
	short xyz[3];
	short angles[3];
} mdcTag_t;
/*
** mdcSurface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numBaseFrames
** XyzCompressed	sizeof( mdcXyzCompressed ) * numVerts * numCompFrames
** frameBaseFrames	sizeof( short ) * numFrames
** frameCompFrames	sizeof( short ) * numFrames (-1 if frame is a baseFrame)
*/
typedef struct {
	int ident;                  //

	char name[MAX_QPATH];       // polyset name

	int flags;
	int numCompFrames;          // all surfaces in a model should have the same
	int numBaseFrames;          // ditto

	int numShaders;             // all surfaces in a model should have the same
	int numVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsShaders;             // offset from start of md3Surface_t
	int ofsSt;                  // texture coords are common for all frames
	int ofsXyzNormals;          // numVerts * numBaseFrames
	int ofsXyzCompressed;       // numVerts * numCompFrames

	int ofsFrameBaseFrames;     // numFrames
	int ofsFrameCompFrames;     // numFrames

	int ofsEnd;                 // next surface follows
} mdcSurface_t;

typedef struct {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	int flags;

	int numFrames;
	int numTags;
	int numSurfaces;

	int numSkins;

	int ofsFrames;                  // offset for first frame, stores the bounds and localOrigin
	int ofsTagNames;                // numTags
	int ofsTags;                    // numFrames * numTags
	int ofsSurfaces;                // first surface, others follow

	int ofsEnd;                     // end of file
} mdcHeader_t;

/*
========================================================================

.SP2 sprite file format

========================================================================
*/

#define IDSPRITEHEADER	(('2'<<24)+('S'<<16)+('D'<<8)+'I')
		// little-endian "IDS2"
#define SPRITE_VERSION	2

typedef struct
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[MAX_SKINNAME];		// name of pcx file
} dsprframe_t;

typedef struct {
	int			ident;
	int			version;
	int			numframes;
	dsprframe_t	frames[1];			// variable sized
} dsprite_t;

/*
==============================================================================

  .WAL texture file format

==============================================================================
*/


#define	MIPLEVELS	4
typedef struct miptex_s
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} q2_miptex_t;



/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
		// little-endian "IBSP"

#define BSPVERSION	36


// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS		1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	2048
#define	MAX_MAP_ENTSTRING	0x20000
#define	MAX_MAP_TEXINFO		8192

#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x100000

#define MAX_WORLD_COORD		( 128*1024 )
#define MIN_WORLD_COORD		( -128*1024 )
#define WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

#define MAX_BRUSH_SIZE		( WORLD_SIZE )

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

typedef struct
{
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16

#define	HEADER_LUMPS		17

typedef struct
{
	int			ident;
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces
										// without walking the bsp tree
} dmodel_t;


typedef struct
{
	float	point[3];
} dvertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are allways opposites

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			    0x1000000	  // removed before bsping an entity

#define	CONTENTS_MONSTER		    0x2000000	  // should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	  0x4000000   // corpse
#define	CONTENTS_DETAIL			    0x8000000	  // brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	  0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER         0x20000000	// ladder
#define	CONTENTS_NEGATIVE_CURVE 0x40000000	// reverse inside / outside

#define	CONTENTS_KEEP	(CONTENTS_DETAIL | CONTENTS_NEGATIVE_CURVE)


typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dnode_t;


typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
	int			value;			// light emission, etc
	char		texture[32];	// texture name (textures/*.wal)
	int			nexttexinfo;	// for animations, -1 = end of chain
} texinfo_t;


#define	SURF_LIGHT		0x1		// value will hold the light strength

#define	SURF_SLICK		0x2		// effects game physics

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture


#define SURF_EDITOR		0x10000000
#define SURF_PATCH        0x20000000
#define	SURF_CURVE_FAKE		0x40000000
#define	SURF_CURVE		    0x80000000
#define	SURF_KEEP		(SURF_CURVE | SURF_CURVE_FAKE | SURF_PATCH)

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct
{
	unsigned short	planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dface_t;

typedef struct
{
	int			contents;			// OR of all brushes (not needed?)

	int			pvsofs;				// -1 = no info
	int			phsofs;				// -1 = no info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned short		firstleafface;
	unsigned short		numleaffaces;

	unsigned short		firstleafbrush;
	unsigned short		numleafbrushes;
} dleaf_t;

typedef struct
{
	unsigned short	planenum;		// facing out of the leaf
	short	texinfo;
} dbrushside_t;

typedef struct
{
	int			firstside;
	int			numsides;
	int			contents;
} dbrush_t;

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2




/*
========================================================================

.mdl Q1 model file format

========================================================================
*/
#define ID_Q1_MDLHEADER		(('O'<<24)+('P'<<16)+('D'<<8)+'I')
#define	MAX_SKINS	32
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	int			synctype;
	int			flags;
	float		size;
} aliashdr_t;

typedef struct {
	int onseam;
	int s;
	int t;
} stvert_t;


typedef struct dtriangle_s {
	int facesfront;
	int vertindex[3];
} triangle_t;

typedef struct {
	unsigned char position[3];    // X,Y,Z coordinate, packed on 0-255
	unsigned char lightnormalindex;     // index of the vertex normal
} trivertx_t;

#define DT_FACES_FRONT                0x0010
/*
========================================================================

.bsp Q1 model file format

========================================================================
*/

#define	BSP29_LUMP_ENTITIES		0
#define	BSP29_LUMP_PLANES		1
#define	BSP29_LUMP_TEXTURES		2
#define	BSP29_LUMP_VERTEXES		3
#define	BSP29_LUMP_VISIBILITY	4
#define	BSP29_LUMP_NODES		5
#define	BSP29_LUMP_TEXINFO		6
#define	BSP29_LUMP_FACES		7
#define	BSP29_LUMP_LIGHTING		8
#define	BSP29_LUMP_CLIPNODES	9
#define	BSP29_LUMP_LEAFS		10
#define	BSP29_LUMP_MARKSURFACES 11
#define	BSP29_LUMP_EDGES		12
#define	BSP29_LUMP_SURFEDGES	13
#define	BSP29_LUMP_MODELS		14
#define BSP29_HEADER_LUMPS		15

typedef struct {
	int			ident;
	lump_t		lumps[BSP29_HEADER_LUMPS];
} q1_dheader_t;

typedef struct {
	float	point[3];
} q1_dvertex_t;

typedef struct {
	unsigned short	v[2];		// vertex numbers
} q1_dedge_t;

typedef struct {
	float		vecs[2][4];		// [s/t][xyz offset]
	int			miptex;
	int			flags;
} q1_texinfo_t;

#define	MAXLIGHTMAPS	4
typedef struct {
	short		planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;
	short		texinfo;

	// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
} q1_dface_t;

/*
========================================================================

.wad sprite file format

========================================================================
*/



#define	TYP_MIPTEX	68
#define	TYP_PALETTE	64

typedef struct {
	char	id[4];		// should be WAD2 or 2DAW
	int		numlumps;
	int		infotableofs;
} wadinfo_t;


typedef struct {
	int		filepos;
	int		disksize;
	int		size;					// uncompressed
	char	type;
	char	compression;
	char	pad1, pad2;
	char	name[16];				// must be null terminated
} lumpinfo_t;

struct q1_miptex_t {
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
};


/*
========================================================================

.SPR sprite file format

	0: Parallel	Rotates to always face the camera
	1: Parallel Upright	Like parallel, but doesn't rotate around Z axis
	2: Oriented	Rotated in map editor to face a specific direction
	3: Parallel Oriented	Combination of parallel and oriented.
	4: Facing Upright	Like parallel upright, but rotated based on player origin, rather than camera
========================================================================
*/
#define ID_Q1_SPRITEHEADER	(('P'<<24)+('S'<<16)+('D'<<8)+'I')
enum {
	SPR_FWD_PARALLEL_UPRIGHT = 0,
	SPR_FACING_UPRIGHT = 1,
	SPR_FWD_PARALLEL = 2,
	SPR_ORIENTED = 3,
	SPR_FWD_PARALLEL_ORIENTED = 4
};

typedef struct {
	int					ident;
	int					version;
	int					type;
	float				radius;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	int					synctype;
} q1_msprite_t;


typedef struct {
	int					offset_x;
	int					offset_y;
	int					width;
	int					height;
} q1_msprframe_t;



/*
========================================================================

.SPR sprite file format

0: Parallel	Rotates to always face the camera
1: Parallel Upright	Like parallel, but doesn't rotate around Z axis
2: Oriented	Rotated in map editor to face a specific direction
3: Parallel Oriented	Combination of parallel and oriented.
4: Facing Upright	Like parallel upright, but rotated based on player origin, rather than camera
========================================================================
*/
typedef struct mspriteframe_s {
	int		group;
	int		origin_x;
	int		origin_y;
	int		width;
	int		height;
} hl_mspriteframe_t;

typedef struct {
	char				id[4];
	int					version;
	int					type;
	int					tex_format;
	float				radius;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;
	int					sync_type;
} hl_msprite_t;

/*
========================================================================

.PAK file format

========================================================================
*/
struct dpackfile_t {
	char            name[56];
	int             filepos, filelen;
};

struct dpackheader_t {
	char            id[4];
	int             dirofs;
	int             dirlen;
};


/*
==============================================================================

MDS file format (Wolfenstein Skeletal Format)

==============================================================================
*/

#define MDS_IDENT           ( ( 'W' << 24 ) + ( 'S' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDS_VERSION         4
#define MDS_MAX_VERTS       6000
#define MDS_MAX_TRIANGLES   8192
#define MDS_MAX_BONES       128
#define MDS_MAX_SURFACES    32
#define MDS_MAX_TAGS        128

#define MDS_TRANSLATION_SCALE   ( 1.0 / 64 )
#define BONEFLAG_TAG        1       // this bone is actually a tag
#define MAX_QPATH 64


struct mdsWeight_t {
	int boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
};

struct mdsVertex_t {
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	int fixedParent;            // stay equi-distant from this parent
	float fixedDist;
	mdsWeight_t weights[1];     // variable sized
};

struct mdsTriangle_t {
	int indexes[3];
};

struct mdsSurface_t {
	int ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                // for in-game use

	int minLod;

	int ofsHeader;                  // this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsCollapseMap;           // numVerts * int

	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next surface follows
};

struct mdsBoneFrameCompressed_t {
	short angles[4];            // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];         // PITCH/YAW, head in this direction from parent to go to the offset position
};

struct mdsFrame_t {
	vec3_t mins, maxs;              // bounds of all surfaces of all LOD's for this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
	mdsBoneFrameCompressed_t bones[1];              // [numBones]
};

struct mdsLOD_t {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
};

struct mdsTag_t {
	char name[MAX_QPATH];           // name of tag
	float torsoWeight;
	int boneIndex;                  // our index in the bones
};

struct mdsBoneInfo_t {
	char name[MAX_QPATH];           // name of bone
	int parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int flags;
};

struct mdsHeader_t {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
	int numFrames;
	int numBones;
	int ofsFrames;                  // mdsFrame_t[numFrames]
	int ofsBones;                   // mdsBoneInfo_t[numBones]
	int torsoParent;                // index of bone that is the parent of the torso

	int numSurfaces;
	int ofsSurfaces;

	// tag data
	int numTags;
	int ofsTags;                    // mdsTag_t[numTags]

	int ofsEnd;                     // end of file
};



/*
==============================================================================

MDM file format (Wolfenstein Mesh Data)

==============================================================================
*/

#define MDM_IDENT           ( ( 'W' << 24 ) + ( 'M' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDM_VERSION         3
#define MDM_MAX_VERTS       6000
#define MDM_MAX_TRIANGLES   8192
#define MDM_MAX_SURFACES    32
#define MDM_MAX_TAGS        128

#define MDM_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct {
	int boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
} mdmWeight_t;

typedef struct {
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	mdmWeight_t weights[1];     // variable sized
} mdmVertex_t;

typedef struct {
	int indexes[3];
} mdmTriangle_t;

typedef struct {
	int ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                // for in-game use

	int minLod;

	int ofsHeader;                  // this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsCollapseMap;           // numVerts * int
	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next surface follows
} mdmSurface_t;

typedef struct {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
} mdmLOD_t;

typedef struct {
	char name[MAX_QPATH];           // name of tag
	vec3_t axis[3];

	int boneIndex;
	vec3_t offset;

	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next tag follows
} mdmTag_t;

typedef struct {
	int ident;
	int version;
	char name[MAX_QPATH];           // model name
	float lodScale;
	float lodBias;
	int numSurfaces;
	int ofsSurfaces;
	int numTags;
	int ofsTags;
	int ofsEnd;                     // end of file
} mdmHeader_t;

/*
==============================================================================

MDX file format (Wolfenstein Skeletal Data)

version history:
1 - initial version
2 - moved parentOffset from the mesh to the skeletal data file

==============================================================================
*/
#define MDX_IDENT           ( ( 'W' << 24 ) + ( 'X' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDX_VERSION         2
#define MDX_MAX_BONES       128



typedef struct {
	short angles[4];                // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];             // PITCH/YAW, head in this direction from parent to go to the offset position
} mdxBoneFrameCompressed_t;
typedef struct {
	float matrix[3][3];             // 3x3 rotation
	vec3_t translation;             // translation vector
} mdxBoneFrame_t;

typedef struct {
	vec3_t bounds[2];               // bounds of this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
	mdxBoneFrameCompressed_t bones[1]; // increment
} mdxFrame_t;

typedef struct {
	char name[MAX_QPATH];           // name of bone
	int parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int flags;
} mdxBoneInfo_t;

typedef struct {
	int ident;
	int version;
	char name[MAX_QPATH];           // model name
	int numFrames;
	int numBones;
	int ofsFrames;                  // (mdxFrame_t + mdxBoneFrameCompressed_t[numBones]) * numframes
	int ofsBones;                   // mdxBoneInfo_t[numBones]
	int torsoParent;                // index of bone that is the parent of the torso

	int ofsEnd;                     // end of file
} mdxHeader_t;
/*
==============================================================================

MD4 file format

==============================================================================
*/

#define MD4_IDENT           ( ( '4' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MD4_VERSION         1
#define MD4_MAX_BONES       128

typedef struct {
	int boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
} md4Weight_t;

typedef struct {
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	md4Weight_t weights[1];     // variable sized
} md4Vertex_t;

typedef struct {
	int indexes[3];
} md4Triangle_t;

typedef struct {
	int ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                // for in-game use

	int ofsHeader;                  // this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next surface follows
} md4Surface_t;

typedef struct {
	float matrix[3][4];
} md4Bone_t;

typedef struct {
	vec3_t bounds[2];               // bounds of all surfaces of all LOD's for this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	char name[16];
	md4Bone_t bones[1];             // [numBones]
} md4Frame_t;

typedef struct {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
} md4LOD_t;

typedef struct {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	// frames and bones are shared by all levels of detail
	int numFrames;
	int numBones;
	int ofsFrames;                  // md4Frame_t[numFrames]

	// each level of detail has completely separate sets of surfaces
	int numLODs;
	int ofsLODs;

	int ofsEnd;                     // end of file
} md4Header_t;

/*
==============================================================================

MDX file format KIIINGPIN

==============================================================================
*/


#define KP_MAX_MDX_FRAMES 512 //hypov8 was 1024??
#define KP_MAX_MDX_OBJECTS 64
#define KP_MAX_MDX_VERT 2048
#define KP_MAX_MDX_TRI 4096

enum {
	KP_MDX_PLAYER_HEAD,	//0
	KP_MDX_PLAYER_BODY,	//1
	KP_MDX_PLAYER_LEGS,	//2
	KP_MDX_PLAYER_RL,		//3
	KP_MDX_PLAYER_FL,		//4
	KP_MDX_PLAYER_GL,		//5
	KP_MDX_PLAYER_HMG,	//6
	KP_MDX_PLAYER_PIPE,	//7
	KP_MDX_PLAYER_PISTOL,	//8
	KP_MDX_PLAYER_SG,		//9
	KP_MDX_PLAYER_TG,		//10
	KP_MDX_PLAYER_MAX,	//11
	KP_MDX_PLAYER_NULL = 255
};


typedef struct {
	int					exportPlayerModel; // keepBoneSpaces;
} kp_mdxOpts_t;

typedef struct {
	byte			id[4];
	int				ver; //kp ver = 4

	int				skinWidth;
	int				skinHeight;
	int				frameSize;

	int				numSkins;				// number of textures
	int				numVerts;				// number of vertices
	int				numTris;				// number of triangles
	int				numGLCmds;				// number of gl commands
	int				numFrames;				// number of frames
	int				num_SfxDefines; //mdx	// number of sfx definitions
	int				num_SfxEntries; //mdx	// number of sfx entries
	int				num_SubObjects; //mdx	// number of subobjects

	int				ofsSkins;			/*mdxSkin_t*/	//name[64];
	int				ofsTris;			/*mdxTri_t*/	//vertIDX[3], nornalIdx[3].
	int				ofsFrames;			//vertex pos, vertex normalIDX
	int				ofsGLCmds;			//triCount(-/+ is type), objectNum, (tri1)s,t, vertIdx... (tri2)s,t,vertIdx...
	int				offsetVertexInfo;	//objectID //mdx
	int				offsetSfxDefines;	//mdx 
	int				offsetSfxEntries;	//mdx 
	int				offsetBBoxFrames;	//mdx 
	int				offsetDummyEnd;

	int				ofsEnd;
} kp_mdxHdr_t;


typedef struct {
	char			name[64];
} kp_mdxSkin_t;

//vertPos[3], normalIndex
typedef struct {
	byte			pos[3]; //vertex position
	byte			nrmIdx; //vertex normal index. shared normals
} kp_mdxVert_t;

typedef struct {
	int				objectNum; //allways 1 (first object)
} kp_mdxVertInfo_t;

//vertIDX[3], normalIDX[3]
typedef struct {
	WORD			vIdx[3]; //3 vertex index to make a tri
	WORD			uvIdx[3]; //vertexnormal[3] nornalIdx
} kp_mdxTri_t;

//scale[3], trans[3], name[16]
typedef struct {
	float			scale[3];
	float			trans[3];
	char			name[16]; //frame name
	kp_mdxVert_t	verts[1]; // sized
} kp_mdxFrame_t;


typedef struct {
	float min[3];
	float max[3];
} kp_mdxBBox_t;

typedef struct {
	float			st[2];
	int				vIdx;
} kp_mdxGLCmd_t;


//GL OBJECT HEADDER
typedef struct {
	int TrisTypeNum;
	int SubObjectID;
} kp_mdxGLCmdHeader_t;


