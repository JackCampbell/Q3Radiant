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

void VectorTransform(const vec3_t in1, const float in2[3][4], vec3_t out) {
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}

float g_bonetransform[MAXSTUDIOBONES][3][4];
struct {
	vec3_t	pos;
	vec2_t	st;
} stTempVertex[4096];

// test: models/bigrat.mdl
void Load_MDLModel_v10(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.size = -1;
	stream.offset = 0;
	return;

	studiohdr_t *header = stream.GetPtr<studiohdr_t>();
	assert(header->id == 0x54534449); // IDST
	assert(header->version == STUDIO_VERSION);

	int m_sequence = 0;

	mstudiobone_t *pbones = stream.GetOffset<mstudiobone_t>(header->boneindex, header->numbones);
	mstudioseqdesc_t *pseqdesc = stream.GetOffset<mstudioseqdesc_t>(header->seqindex, header->numseq) + m_sequence;
	
	mstudioseqgroup_t *seqgroup = stream.GetOffset<mstudioseqgroup_t>(header->seqgroupindex, header->numseqgroups) + pseqdesc->seqgroup;

	// mstudiobonecontroller_t *bonectl = stream.GetOffset<mstudiobonecontroller_t>(header->bonecontrollerindex, header->numbonecontrollers);
	// mstudiobbox_t *hitboxes = stream.GetOffset<mstudiobbox_t>(header->hitboxindex, header->numhitboxes);
	// mstudioattachment_t *attachment = stream.GetOffset<mstudioattachment_t>(header->attachmentindex, header->numattachments);
	
	static vec3_t		pos[MAXSTUDIOBONES];
	float				bonematrix[3][4];
	static vec4_t		q[MAXSTUDIOBONES];
	mstudioanim_t *panim = nullptr;
	if (pseqdesc->seqgroup == 0) {
		panim = stream.GetOffset<mstudioanim_t>(pseqdesc->animindex);
	} else {
		assert(false); // TODO
	}
	/*
	CalcRotations(pos, q, pseqdesc, panim, m_frame);
	for (int i = 0; i < header->numbones; i++) {
		QuaternionMatrix(q[i], bonematrix);
		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];
		if (pbones[i].parent == -1) {
			memcpy(g_bonetransform[i], bonematrix, sizeof(float) * 12);
		} else {
			R_ConcatTransforms(g_bonetransform[pbones[i].parent], bonematrix, g_bonetransform[i]);
		}
	}
	*/

	mstudiotexture_t *textures = stream.GetOffset<mstudiotexture_t>(header->textureindex, header->numtextures);
	for (int i = 0; i < header->numtextures; i++) {
		int size = textures[i].width * textures[i].height;
		byte *pix = stream.GetOffset<byte>(textures[i].index);
		byte *pal = stream.GetOffset<byte>(textures[i].index + size);
		
		byte *out = (byte *)qmalloc(sizeof(byte) * (size * 4));
		WAD3_ConvertPalToRGBA(pix, pal, out, size, textures[i].name, false);

		qtexture_t *q = Texture_LoadTGATexture(out, textures[i].width, textures[i].height, nullptr, 0, 0, 0);
		strncpy(q->name, textures[i].name, sizeof(q->name) - 1);
		StripExtension(q->name);
		q->inuse = true;
		q->next = g_qeglobals.d_qtextures;
		g_qeglobals.d_qtextures = q;

		free(out);
	}


	vec2_t st;
	int numTris = 0;
	short *pskinref = stream.GetOffset<short>(header->skinindex);
	for (int k = 0; k < header->numbodyparts; k++) {
		mstudiobodyparts_t *pbodyparts = stream.GetOffset<mstudiobodyparts_t>(header->bodypartindex, header->numbodyparts) + k;
		for (int j = 0; j < pbodyparts->nummodels; j++) {
			mstudiomodel_t *pmodel = stream.GetOffset<mstudiomodel_t>(pbodyparts->modelindex) + j;
			vec3_t *pstudioverts = stream.GetOffset<vec3_t>(pmodel->vertindex);
			vec3_t *pstudionorms = stream.GetOffset<vec3_t>(pmodel->normindex);
			byte *pvertbone = stream.GetOffset<byte>(pmodel->vertinfoindex);
			byte *pnormbone = stream.GetOffset<byte>(pmodel->norminfoindex);

			vec3_t *g_pxformverts = (vec3_t *)qmalloc(sizeof(vec3_t) * pmodel->numverts);

			for (int i = 0; i < pmodel->numverts; i++) {
				// VectorTransform(pstudioverts[i], g_bonetransform[pvertbone[i]], g_pxformverts[i]);
				VectorCopy(pstudioverts[i], g_pxformverts[i]);
			}

			for (int l = 0; l < pmodel->nummesh; l++) {
				mstudiomesh_t *pmesh = stream.GetOffset<mstudiomesh_t>(pmodel->meshindex) + l;
				short *ptricmds = stream.GetOffset<short>(pmesh->triindex);
				short pskin = pskinref[pmesh->skinref];

				float s = 1.0f / textures[pskin].width;
				float t = 1.0f / textures[pskin].height;
				int i;
				while ((i = *(ptricmds++))) {
					bool isFan = false;
					if (i < 0) {
						isFan = true;
						i = -i;
					}
					for (; i > 0; i--, ptricmds += 4) {
						int index = ptricmds[0];
						// vec3_t pos = g_pxformverts[index];
						st[0] = ptricmds[2] * s;
						st[1] = ptricmds[3] * t;

						if (numTris < 3) {
							VectorCopy(g_pxformverts[index], stTempVertex[numTris].pos);
							VectorCopy2(st, stTempVertex[numTris].st);
							numTris++;
						} else if (isFan) {
							VectorCopy(stTempVertex[0].pos, stTempVertex[numTris].pos);
							VectorCopy2(stTempVertex[0].st, stTempVertex[numTris].st);
							VectorCopy(stTempVertex[numTris - 1].pos, stTempVertex[numTris + 1].pos);
							VectorCopy2(stTempVertex[numTris - 1].st, stTempVertex[numTris + 1].st);
							VectorCopy(g_pxformverts[index], stTempVertex[numTris + 2].pos);
							VectorCopy2(st, stTempVertex[numTris + 2].st);
							numTris += 3;
						} else {
							VectorCopy(stTempVertex[numTris - 2].pos, stTempVertex[numTris].pos);
							VectorCopy2(stTempVertex[numTris - 2].st, stTempVertex[numTris].st);
							VectorCopy(stTempVertex[numTris - 1].pos, stTempVertex[numTris + 1].pos);
							VectorCopy2(stTempVertex[numTris - 1].st, stTempVertex[numTris + 1].st);
							VectorCopy(g_pxformverts[index], stTempVertex[numTris + 2].pos);
							VectorCopy2(st, stTempVertex[numTris + 2].st);
							numTris += 3;
						}
					}
				}
			}
			free(g_pxformverts);


			int num_size = numTris / 3;

			pModel->nSkinHeight = 4;
			pModel->nSkinWidth = 4;
			pModel->nTriCount = num_size;
			pModel->nModelPosition = 0;
			pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * num_size);
			pModel->nTextureBind[pModel->nNumTextures++] = notexture->texture_number;

			for (int i = 0; i < numTris; i++) {
				VectorCopy(stTempVertex[i].pos, pModel->pTriList[i / 3].v[i % 3]);
				VectorCopy2(stTempVertex[i].st, pModel->pTriList[i / 3].st[i % 3]);
			}
			// Sys_Printf("-- %f %f %f\n", pstudioverts[0], pstudioverts[1], pstudioverts[2]);
		}
	}
	Sys_Printf("Test\n");
}
