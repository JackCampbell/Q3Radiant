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
//-----------------------------------------------------------------------------
//
// $LogFile$
// $Revision: 1.1.2.2 $
// $Author: ttimo $
// $Date: 2000/02/24 22:24:45 $
// $Log: IShaders.cpp,v $
// Revision 1.1.2.2  2000/02/24 22:24:45  ttimo
// RC2
//
// Revision 1.1.2.1  2000/02/11 03:52:30  ttimo
// working on the IShader interface
//
//
// DESCRIPTION:
// implementation of the shaders / textures interface
//

#include "stdafx.h"

//++timo NOTE: this whole part is evolving on a seperate branch on SourceForge
// will eventually turn into a major rewrite of the shader / texture code

// this is a modified version of Texture_ForName
qtexture_t* WINAPI QERApp_TryTextureForName(const char* name) {
	unsigned char* pPixels = NULL;
	int nWidth;
	int nHeight;
	qtexture_t *q;
	char filename[1024];
	for (q = g_qeglobals.d_qtextures; q; q = q->next) {
		if (!strcmp(name, q->filename))
			return q;
	}
	// try loading from file .. this is a copy of the worst part of Texture_ForName
	char cWork[1024];
	sprintf(filename, "textures/%s.tga", name);
	QE_ConvertDOSToUnixName(cWork, filename);
	strcpy(filename, cWork);
	LoadImage(filename, &pPixels, &nWidth, &nHeight);
	if (pPixels == NULL) {
		StripExtension(filename);
		strcat(filename, ".jpg");
		LoadImage(filename, &pPixels, &nWidth, &nHeight);
	}
	if (pPixels) {
		q = Texture_LoadTGATexture(pPixels, nWidth, nHeight, NULL, 0, 0, 0);
		//++timo storing the filename .. will be removed by shader code cleanup
		// this is dirty, and we sure miss some places were we should fill the filename info
		strcpy(q->filename, name);
		SetNameShaderInfo(q, filename, name);
		Sys_Printf("done.\n", name);
		free(pPixels);
		return q;
	}
	return NULL;
}










enum {
	MAXSTUDIOTRIANGLES = 20000,	// TODO: tune this
	MAXSTUDIOVERTS = 2048,	// TODO: tune this
	MAXSTUDIOSEQUENCES = 2048,	// total animation sequences -- KSH incremented
	MAXSTUDIOSKINS = 100,		// total textures
	MAXSTUDIOSRCBONES = 512,		// bones allowed at source movement
	MAXSTUDIOBONES = 128,		// total bones actually used
	MAXSTUDIOMODELS = 32,		// sub-models per model
	MAXSTUDIOBODYPARTS = 32,
	MAXSTUDIOGROUPS = 16,
	MAXSTUDIOANIMATIONS = 2048,
	MAXSTUDIOMESHES = 256,
	MAXSTUDIOEVENTS = 1024,
	MAXSTUDIOPIVOTS = 256,
	MAXSTUDIOCONTROLLERS = 8,
	STUDIO_VERSION = 10,
	STUDIO_MAX_EVENT_OPTIONS_LENGTH = 64,
	STUDIO_NUM_COORDINATE_AXES = 6,
	STUDIO_ATTACH_NUM_VECTORS = 3,

	SequenceBlendCount = 2,
	ControllerCount = 4,
	SequenceBlendXIndex = 0,
	SequenceBlendYIndex = 1,
	MaxBoneNameBytes = 32,
	MaxAttachmentNameBytes = 32,
	MaxTextureNameBytes = 64,
	MaxModelNameBytes = 64
};

struct studioseqhdr_t { // sequence group data
	int		id;
	int		version;
	char	name[64];
	int		length;
};
// bones
struct mstudiobone_t {
	char	name[MaxBoneNameBytes];	// bone name for symbolic links
	int		parent;		// parent bone
	int		flags;		// ??
	int		bonecontroller[STUDIO_NUM_COORDINATE_AXES];	// bone controller index, -1 == none
	float	value[STUDIO_NUM_COORDINATE_AXES];	// default DoF values
	float	scale[STUDIO_NUM_COORDINATE_AXES];   // scale for delta DoF values
};
struct mstudiobonecontroller_t {
	int		bone;	// -1 == 0
	int		type;	// X, Y, Z, XR, YR, ZR, M
	float	start;
	float	end;
	int		rest;	// byte index value at rest
	int		index;	// 0-3 user set controller, 4 mouth
};
struct mstudiobbox_t {
	int			bone;
	int			group;			// intersection group
	vec3_t		bbmin;		// bounding box
	vec3_t		bbmax;
};
struct mstudioseqgroup_t {
	char	label[32];	// textual name
	char	name[64];	// file name
	int		unused1;    // was "cache"  - index pointer
	int		unused2;    // was "data" -  hack for group 0
};
struct mstudioseqdesc_t {
	char		label[32];	// sequence label

	float		fps;		// frames per second	
	int			flags;		// looping/non-looping flags

	int			activity;
	int			actweight;

	int			numevents;
	int			eventindex;

	int			numframes;	// number of frames per sequence

	int			numpivots;	// number of foot pivots
	int			pivotindex;

	int			motiontype;
	int			motionbone;
	vec3_t		linearmovement;
	int			automoveposindex;
	int			automoveangleindex;

	vec3_t		bbmin;		// per sequence bounding box
	vec3_t		bbmax;

	int			numblends;
	int			animindex;		// mstudioanim_t pointer relative to start of sequence group data
	// [blend][bone][X, Y, Z, XR, YR, ZR]

	int			blendtype[SequenceBlendCount];	// X, Y, Z, XR, YR, ZR
	float		blendstart[SequenceBlendCount];	// starting value
	float		blendend[SequenceBlendCount];	// ending value
	int			blendparent;

	int			seqgroup;		// sequence group for demand loading

	int			entrynode;		// transition node at entry
	int			exitnode;		// transition node at exit
	int			nodeflags;		// transition rules

	int			nextseq;		// auto advancing sequences
};
struct mstudioevent_t {
	int 	frame;
	int		event;
	int		type;
	char	options[STUDIO_MAX_EVENT_OPTIONS_LENGTH];
};
struct mstudiopivot_t {
	vec3_t		org;	// pivot point
	int			start;
	int			end;
};
struct mstudioattachment_t {
	char name[MaxAttachmentNameBytes];
	int type;
	int bone;
	vec3_t org;
	vec3_t vectors[STUDIO_ATTACH_NUM_VECTORS];
};
struct mstudioanim_t {
	unsigned short	offset[STUDIO_NUM_COORDINATE_AXES];
};
union mstudioanimvalue_t {
	struct {
		unsigned char valid;
		unsigned char total;
	} num;
	short		value;
};
struct mstudiobodyparts_t {
	char	name[64];
	int		nummodels;
	int		base;
	int		modelindex; // index into models array
};
struct mstudiotexture_t {
	char	name[MaxTextureNameBytes];
	int		flags;
	int		width;
	int		height;
	int		index;
};
struct mstudiomodel_t {
	char	name[MaxModelNameBytes];
	int		type;
	float	boundingradius;
	int		nummesh;
	int		meshindex;
	int		numverts;		// number of unique vertices
	int		vertinfoindex;	// vertex bone info
	int		vertindex;		// vertex glm::vec3
	int		numnorms;		// number of unique surface normals
	int		norminfoindex;	// normal bone info
	int		normindex;		// normal glm::vec3
	int		numgroups;		// deformation groups
	int		groupindex;
};
struct mstudiomesh_t {
	int		numtris;
	int		triindex;
	int		skinref;
	int		numnorms;		// per mesh normals
	int		normindex;		// normal glm::vec3
};
struct mstudiotrivert_t {
	short	vertindex;		// index into vertex array
	short	normindex;		// index into normal array
	short	s, t;			// s,t position on skin
};
struct studiohdr_t {
	int			id;
	int			version;

	char		name[64];
	int			length;
	vec3_t		eyeposition;	// ideal eye position
	vec3_t		min;			// ideal movement hull size
	vec3_t		max;
	vec3_t		bbmin;			// clipping bounding box
	vec3_t		bbmax;
	int			flags;
	int			numbones;			// bones
	int			boneindex;
	int			numbonecontrollers;		// bone controllers
	int			bonecontrollerindex;
	int			numhitboxes;			// complex bounding boxes
	int			hitboxindex;
	int			numseq;				// animation sequences
	int			seqindex;
	int			numseqgroups;		// demand loaded sequences
	int			seqgroupindex;
	int			numtextures;		// raw textures
	int			textureindex;
	int			texturedataindex;
	int			numskinref;			// replaceable textures
	int			numskinfamilies;
	int			skinindex;
	int			numbodyparts;
	int			bodypartindex;
	int			numattachments;		// queryable attachable points
	int			attachmentindex;
	int			soundtable;
	int			soundindex;
	int			soundgroups;
	int			soundgroupindex;

	int			numtransitions;		// animation node to animation node transition graph
	int			transitionindex;
};

/*
void Test_Proc() {
	const char *filename = "C:\\Steam\\steamapps\\common\\Half-Life\\valve\\models\\bigrat.mdl";
	byte *buffer;
	int len = LoadFile(filename, (void **)&buffer);
	if (len == -1) {
		return;
	}

	readbuf_t stream;
	stream.buffer = buffer;
	stream.size = len;
	stream.offset = 0;


	studiohdr_t *header = stream.GetPtr<studiohdr_t>();
	assert(header->id == 0x54534449); // IDST
	assert(header->version == STUDIO_VERSION);

	mstudiobone_t *bone = stream.GetIndex<mstudiobone_t>(header->boneindex, header->numbones);
	mstudiobonecontroller_t *bonectl = stream.GetIndex<mstudiobonecontroller_t>(header->bonecontrollerindex, header->numbonecontrollers);
	mstudiobbox_t *hitboxes = stream.GetIndex<mstudiobbox_t>(header->hitboxindex, header->numhitboxes);
	mstudioseqdesc_t *seqdesc = stream.GetIndex<mstudioseqdesc_t>(header->seqindex, header->numseq);
	mstudioseqgroup_t *seqgroup = stream.GetIndex<mstudioseqgroup_t>(header->seqgroupindex, header->numseqgroups);
	mstudiotexture_t *textures = stream.GetIndex<mstudiotexture_t>(header->textureindex, header->numtextures);



	short *skins = stream.GetIndex<short>(header->skinindex, 1);
	mstudiobodyparts_t *bodyparts = stream.GetIndex<mstudiobodyparts_t>(header->bodypartindex, header->numbodyparts);
	mstudioattachment_t *attachment = stream.GetIndex<mstudioattachment_t>(header->attachmentindex, header->numattachments);

	for (int b = 0; b < header->numbodyparts; b++) {
		for (int m = 0; m < bodyparts->nummodels; m++) {
			// PASS
		}
	}

	Sys_Printf("Test\n");
	free(buffer);
}


*/
