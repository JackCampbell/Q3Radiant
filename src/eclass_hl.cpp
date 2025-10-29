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


#define MAXSTUDIOTRIANGLES	20000	// TODO: tune this
#define MAXSTUDIOVERTS		2048	// TODO: tune this
#define MAXSTUDIOSEQUENCES	256		// total animation sequences
#define MAXSTUDIOSKINS		100		// total textures
#define MAXSTUDIOSRCBONES	512		// bones allowed at source movement
#define MAXSTUDIOBONES		128		// total bones actually used
#define MAXSTUDIOMODELS		32		// sub-models per model
#define MAXSTUDIOBODYPARTS	32
#define MAXSTUDIOGROUPS		4
#define MAXSTUDIOANIMATIONS	512		// per sequence
#define MAXSTUDIOMESHES		256
#define MAXSTUDIOEVENTS		1024
#define MAXSTUDIOPIVOTS		256
#define MAXSTUDIOCONTROLLERS 8

typedef struct {
	int					id;
	int					version;

	char				name[64];
	int					length;

	vec3_t				eyeposition;	// ideal eye position
	vec3_t				min;			// ideal movement hull size
	vec3_t				max;			

	vec3_t				bbmin;			// clipping bounding box
	vec3_t				bbmax;		

	int					flags;

	int					numbones;			// bones
	int					boneindex;

	int					numbonecontrollers;		// bone controllers
	int					bonecontrollerindex;

	int					numhitboxes;			// complex bounding boxes
	int					hitboxindex;			

	int					numseq;				// animation sequences
	int					seqindex;

	int					numseqgroups;		// demand loaded sequences
	int					seqgroupindex;

	int					numtextures;		// raw textures
	int					textureindex;
	int					texturedataindex;

	int					numskinref;			// replaceable textures
	int					numskinfamilies;
	int					skinindex;

	int					numbodyparts;		
	int					bodypartindex;

	int					numattachments;		// queryable attachable points
	int					attachmentindex;

	int					soundtable;
	int					soundindex;
	int					soundgroups;
	int					soundgroupindex;

	int					numtransitions;		// animation node to animation node transition graph
	int					transitionindex;
} studiohdr_t;

// header for demand loaded sequence group data
typedef struct {
	int					id;
	int					version;

	char				name[64];
	int					length;
} studioseqhdr_t;

// bones
typedef struct  {
	char				name[32];	// bone name for symbolic links
	int		 			parent;		// parent bone
	int					flags;		// ??
	int					bonecontroller[6];	// bone controller index, -1 == none
	float				value[6];	// default DoF values
	float				scale[6];   // scale for delta DoF values
} mstudiobone_t;


// bone controllers
typedef struct {
	int					bone;	// -1 == 0
	int					type;	// X, Y, Z, XR, YR, ZR, M
	float				start;
	float				end;
	int					rest;	// byte index value at rest
	int					index;	// 0-3 user set controller, 4 mouth
} mstudiobonecontroller_t;

// intersection boxes
typedef struct {
	int					bone;
	int					group;			// intersection group
	vec3_t				bbmin;		// bounding box
	vec3_t				bbmax;
} mstudiobbox_t;

// demand loaded sequence groups
typedef struct {
	char				label[32];	// textual name
	char				name[64];	// file name
	int					cache;		// cache index pointer
	int					data;		// hack for group 0
} mstudioseqgroup_t;

// sequence descriptions
typedef struct {
	char				label[32];	// sequence label

	float				fps;		// frames per second	
	int					flags;		// looping/non-looping flags

	int					activity;
	int					actweight;

	int					numevents;
	int					eventindex;

	int					numframes;	// number of frames per sequence

	int					numpivots;	// number of foot pivots
	int					pivotindex;

	int					motiontype;
	int					motionbone;
	vec3_t				linearmovement;
	int					automoveposindex;
	int					automoveangleindex;

	vec3_t				bbmin;		// per sequence bounding box
	vec3_t				bbmax;

	int					numblends;
	int					animindex;		// mstudioanim_t pointer relative to start of sequence group data
	// [blend][bone][X, Y, Z, XR, YR, ZR]

	int					blendtype[2];	// X, Y, Z, XR, YR, ZR
	float				blendstart[2];	// starting value
	float				blendend[2];	// ending value
	int					blendparent;

	int					seqgroup;		// sequence group for demand loading

	int					entrynode;		// transition node at entry
	int					exitnode;		// transition node at exit
	int					nodeflags;		// transition rules

	int					nextseq;		// auto advancing sequences
} mstudioseqdesc_t;

// events
typedef struct {
	int 				frame;
	int					event;
	int					type;
	char				options[64];
} mstudioevent_t;


// pivots
typedef struct {
	vec3_t				org;	// pivot point
	int					start;
	int					end;
} mstudiopivot_t;

// attachment
typedef struct {
	char				name[32];
	int					type;
	int					bone;
	vec3_t				org;	// attachment point
	vec3_t				vectors[3];
} mstudioattachment_t;

typedef struct {
	unsigned short	offset[6];
} mstudioanim_t;

// animation frames
typedef union {
	struct {
		byte	valid;
		byte	total;
	} num;
	short		value;
} mstudioanimvalue_t;



// body part index
typedef struct {
	char				name[64];
	int					nummodels;
	int					base;
	int					modelindex; // index into models array
} mstudiobodyparts_t;



// skin info
typedef struct {
	char					name[64];
	int						flags;
	int						width;
	int						height;
	int						index;
} mstudiotexture_t;


// skin families
// short	index[skinfamilies][skinref]

// studio models
typedef struct {
	char				name[64];

	int					type;

	float				boundingradius;

	int					nummesh;
	int					meshindex;

	int					numverts;		// number of unique vertices
	int					vertinfoindex;	// vertex bone info
	int					vertindex;		// vertex vec3_t
	int					numnorms;		// number of unique surface normals
	int					norminfoindex;	// normal bone info
	int					normindex;		// normal vec3_t

	int					numgroups;		// deformation groups
	int					groupindex;
} mstudiomodel_t;


// vec3_t	boundingbox[model][bone][2];	// complex intersection info


// meshes
typedef struct {
	int					numtris;
	int					triindex;
	int					skinref;
	int					numnorms;		// per mesh normals
	int					normindex;		// normal vec3_t
} mstudiomesh_t;

// triangles
typedef struct {
	short				vertindex;		// index into vertex array
	short				normindex;		// index into normal array
	short				s, t;			// s,t position on skin
} mstudiotrivert_t;


// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT		0x0004
#define STUDIO_NF_ADDITIVE		0x0020
#define STUDIO_NF_TRANSPARENT		0x0040


// motion flags
#define STUDIO_X		0x0001
#define STUDIO_Y		0x0002	
#define STUDIO_Z		0x0004
#define STUDIO_XR		0x0008
#define STUDIO_YR		0x0010
#define STUDIO_ZR		0x0020
#define STUDIO_LX		0x0040
#define STUDIO_LY		0x0080
#define STUDIO_LZ		0x0100
#define STUDIO_AX		0x0200
#define STUDIO_AY		0x0400
#define STUDIO_AZ		0x0800
#define STUDIO_AXR		0x1000
#define STUDIO_AYR		0x2000
#define STUDIO_AZR		0x4000
#define STUDIO_TYPES	0x7FFF
#define STUDIO_RLOOP	0x8000	// controller that wraps shortest distance

// sequence flags
#define STUDIO_LOOPING	0x0001

// bone flags
#define STUDIO_HAS_NORMALS	0x0001
#define STUDIO_HAS_VERTICES 0x0002
#define STUDIO_HAS_BBOX		0x0004
#define STUDIO_HAS_CHROME	0x0008	// if any of the textures have chrome on them

#define RAD_TO_STUDIO		(32768.0/M_PI)
#define STUDIO_TO_RAD		(M_PI/32768.0)

typedef float vec4p_t[4];

void AngleQuaternion(const vec3_t &angles, vec4p_t &quaternion) {
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[1] * 0.5;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[0] * 0.5;
	sr = sin(angle);
	cr = cos(angle);

	quaternion[0] = sr * cp * cy - cr * sp * sy;
	quaternion[1] = cr * sp * cy + sr * cp * sy;
	quaternion[2] = cr * cp * sy - sr * sp * cy;
	quaternion[3] = cr * cp * cy + sr * sp * sy;
}

void QuaternionSlerp(const vec4p_t& p, vec4p_t& q, float t, vec4p_t& qt) {
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++) {
		a += (p[i] - q[i])*(p[i] - q[i]);
		b += (p[i] + q[i])*(p[i] + q[i]);
	}
	if (a > b) {
		for (i = 0; i < 4; i++) {
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if ((1.0 + cosom) > 0.00000001) {
		if ((1.0 - cosom) > 0.00000001) {
			omega = acos(cosom);
			sinom = sin(omega);
			sclp = sin((1.0 - t)*omega) / sinom;
			sclq = sin(t*omega) / sinom;
		} else {
			sclp = 1.0 - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++) {
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	} else {
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = sin((1.0 - t) * 0.5 * Q_PI);
		sclq = sin(t * 0.5 * Q_PI);
		for (i = 0; i < 3; i++) {
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

void QuaternionMatrix(const vec4p_t quaternion, float(*matrix)[4]) {
	matrix[0][0] = 1.0 - 2.0 * quaternion[1] * quaternion[1] - 2.0 * quaternion[2] * quaternion[2];
	matrix[1][0] =       2.0 * quaternion[0] * quaternion[1] + 2.0 * quaternion[3] * quaternion[2];
	matrix[2][0] =       2.0 * quaternion[0] * quaternion[2] - 2.0 * quaternion[3] * quaternion[1];

	matrix[0][1] =       2.0 * quaternion[0] * quaternion[1] - 2.0 * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0 - 2.0 * quaternion[0] * quaternion[0] - 2.0 * quaternion[2] * quaternion[2];
	matrix[2][1] =       2.0 * quaternion[1] * quaternion[2] + 2.0 * quaternion[3] * quaternion[0];

	matrix[0][2] =       2.0 * quaternion[0] * quaternion[2] + 2.0 * quaternion[3] * quaternion[1];
	matrix[1][2] =       2.0 * quaternion[1] * quaternion[2] - 2.0 * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0 - 2.0 * quaternion[0] * quaternion[0] - 2.0 * quaternion[1] * quaternion[1];
}

void R_ConcatTransforms(const float in1[3][4], const float in2[3][4], float out[3][4]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

void MDL_VectorTransform(const vec3_t in1, const float in2[3][4], vec3_t out) {
#if 0
	out[0] =   DotProduct(in1, in2[0]) + in2[0][3];
	out[2] = -(DotProduct(in1, in2[1]) + in2[1][3]);
	out[1] =   DotProduct(in1, in2[2]) + in2[2][3];
#else
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
#endif
}

void VectorTestSwap(vec3_t in) {
#if 0
	vec3_t temp;
	VectorCopy(in, temp);
	in[0] = temp[0];
	in[1] = temp[2] * -1.0f;
	in[2] = temp[1];
#endif
}


#define MAX_TEXTURE_DIMS 512	

void MDL_TestUploadTexture(const mstudiotexture_t *ptexture, const byte *data, const byte *pal) {
	// unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight;
	int	i, j;
	int	row1[MAX_TEXTURE_DIMS], row2[MAX_TEXTURE_DIMS];
	int col1[MAX_TEXTURE_DIMS], col2[MAX_TEXTURE_DIMS];
	const byte *pix1, *pix2, *pix3, *pix4;
	byte	*tex, *out;

	// convert texture to power of 2
	int outwidth;
	for (outwidth = 1; outwidth < ptexture->width; outwidth <<= 1)
		;

	if (outwidth > MAX_TEXTURE_DIMS)
		outwidth = MAX_TEXTURE_DIMS;

	int outheight;
	for (outheight = 1; outheight < ptexture->height; outheight <<= 1)
		;

	if (outheight > MAX_TEXTURE_DIMS)
		outheight = MAX_TEXTURE_DIMS;

	tex = out = (byte *)malloc(outwidth * outheight * 4);
	if (!out) {
		return;
	}

	for (i = 0; i < outwidth; i++) {
		col1[i] = (int)((i + 0.25) * (ptexture->width / (float)outwidth));
		col2[i] = (int)((i + 0.75) * (ptexture->width / (float)outwidth));
	}

	for (i = 0; i < outheight; i++) {
		row1[i] = (int)((i + 0.25) * (ptexture->height / (float)outheight)) * ptexture->width;
		row2[i] = (int)((i + 0.75) * (ptexture->height / (float)outheight)) * ptexture->width;
	}

	// scale down and convert to 32bit RGB
	for (i = 0; i<outheight; i++) {
		for (j = 0; j<outwidth; j++, out += 4) {
			pix1 = &pal[data[row1[i] + col1[j]] * 3];
			pix2 = &pal[data[row1[i] + col2[j]] * 3];
			pix3 = &pal[data[row2[i] + col1[j]] * 3];
			pix4 = &pal[data[row2[i] + col2[j]] * 3];

			out[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			out[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			out[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			out[3] = 0xFF;
		}
	}

	qtexture_t *q = Texture_LoadTGATexture(tex, outwidth, outheight, nullptr, 0, 0, 0);
	strcpy(q->name, ptexture->name);
	StripExtension(q->name);
	q->inuse = false;
	q->next = g_qeglobals.d_qtextures;
	g_qeglobals.d_qtextures = q;

	free(tex);
}


struct StudioModel {
	int						m_sequence;			// sequence index
	float					m_frame;			// frame
	int						m_bodynum;			// bodypart selection	
	int						m_skinnum;			// skin group selection
	byte					m_controller[4];	// bone controllers
	byte					m_blending[2];		// animation blending
	byte					m_mouth;			// mouth position
	bool					m_owntexmodel;		// do we have a modelT.mdl ?

	// internal data
	studiohdr_t				*m_pstudiohdr;
	mstudiomodel_t			*m_pmodel;

	studiohdr_t				*m_ptexturehdr;
	studiohdr_t				*m_panimhdr[32];
	float					m_bonetransform[MAXSTUDIOBONES][3][4];
	vec3_t					m_formverts[MAXSTUDIOVERTS];
	float					m_adj[MAXSTUDIOCONTROLLERS];

	void CalcBoneAdj();
	void CalcBoneQuaternion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, vec4p_t &q);
	void CalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos);
	void CalcRotations(vec3_t *pos, vec4p_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f);
	mstudioanim_t *GetAnim(mstudioseqdesc_t *pseqdesc);
	void SlerpBones(vec4p_t q1[], vec3_t pos1[], vec4p_t q2[], vec3_t pos2[], float s);
	void AdvanceFrame(float dt);
	int SetFrame(int nFrame);
	void SetUpBones(bool righthand = false);
	void SetupModel(int bodypart);
	bool isCSOExternalTexture(const mstudiotexture_t &texture);
	bool hasCSOTexture(const studiohdr_t *phdr);
	void ScaleBones(float scale);
	void ScaleMeshes(float scale);
	int SetSkin(int iValue);
	int SetBodygroup(int iGroup, int iValue);
	float SetBlending(int iBlender, float flValue);
	float SetMouth(float flValue);
	float SetController(int iController, float flValue);
	int GetSequence();
	void ExtractBbox(float *mins, float *maxs);
	void GetSequenceInfo(float *pflFrameRate, float *pflGroundSpeed);
	int SetSequence(int iSequence);
	bool PostLoadModel(studiohdr_t *phdr, const char *modelname);
	void FreeModel();
	void LoadModelTextures(const studiohdr_t *phdr);
	void LoadModelTexturesCSO(studiohdr_t *phdr, const char *texturePath);
	studiohdr_t *LoadModel(const char *modelname);
	void SetupBuffer(byte *pBuff);
	studiohdr_t *getStudioHeader() const { return m_pstudiohdr; }
	studiohdr_t *getTextureHeader() const { return m_ptexturehdr; }
	studiohdr_t *getAnimHeader(int i) const { return m_panimhdr[i]; }
	void LoadRadiant(entitymodel *&pModel, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int &nCountMesh);
} g_studioModel;

void StudioModel::CalcBoneAdj() {
	int					i;
	float				value;
	mstudiobonecontroller_t *pbonecontroller;

	pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bonecontrollerindex);

	for (i = 0; i < m_pstudiohdr->numbonecontrollers; i++) {
		int index = pbonecontroller[i].index;
		if (index <= 3) {
			// check for 360% wrapping
			if (pbonecontroller[i].type & STUDIO_RLOOP) {
				value = m_controller[index] * (360.0 / 256.0) + pbonecontroller[i].start;
			} else {
				value = m_controller[index] / 255.0;
				if (value < 0) value = 0;
				if (value > 1.0) value = 1.0;
				value = (1.0 - value) * pbonecontroller[i].start + value * pbonecontroller[i].end;
			}
			// Con_DPrintf( "%d %d %f : %f\n", m_controller[j], m_prevcontroller[j], value, dadt );
		} else {
			value = m_mouth / 64.0;
			if (value > 1.0) value = 1.0;
			value = (1.0 - value) * pbonecontroller[i].start + value * pbonecontroller[i].end;
			// Con_DPrintf("%d %f\n", mouthopen, value );
		}
		switch (pbonecontroller[i].type & STUDIO_TYPES) {
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			m_adj[i] = value * (Q_PI / 180.0);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			m_adj[i] = value;
			break;
		}
	}
}

void StudioModel::CalcBoneQuaternion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, vec4p_t &q) {
	int					j, k;
	vec4p_t				q1, q2;
	vec3_t				angle1, angle2;
	mstudioanimvalue_t	*pAnimValue;

	assert(panim);
	VectorClear(angle1);
	VectorClear(angle2);

	for (j = 0; j < 3; j++) {
		if (panim->offset[j + 3] == 0) {
			angle2[j] = angle1[j] = pbone->value[j + 3]; // default;
		} else {
			pAnimValue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j + 3]);
			k = frame;

			while (pAnimValue->num.total <= k) {
				k -= pAnimValue->num.total;
				pAnimValue += pAnimValue->num.valid + 1;
			}
			// Bah, missing blend!
			if (pAnimValue->num.valid > k) {
				angle1[j] = pAnimValue[k + 1].value;

				if (pAnimValue->num.valid > k + 1) {
					angle2[j] = pAnimValue[k + 2].value;
				} else {
					if (pAnimValue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = pAnimValue[pAnimValue->num.valid + 2].value;
				}
			} else {
				angle1[j] = pAnimValue[pAnimValue->num.valid].value;
				if (pAnimValue->num.total > k + 1) {
					angle2[j] = angle1[j];
				} else {
					angle2[j] = pAnimValue[pAnimValue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
			angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
		}

		if (pbone->bonecontroller[j + 3] != -1) {
			angle1[j] += m_adj[pbone->bonecontroller[j + 3]];
			angle2[j] += m_adj[pbone->bonecontroller[j + 3]];
		}
	}

	if (!VectorCompare(angle1, angle2)) {
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q);
	} else {
		AngleQuaternion(angle1, q);
	}
}
void StudioModel::CalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *pAnim, float *pos) {
	int					j, k;
	mstudioanimvalue_t	*pAnimValue;

	assert(pAnim);

	for (j = 0; j < 3; j++) {
		pos[j] = pbone->value[j]; // default;
		if (pAnim->offset[j] != 0) {
			pAnimValue = (mstudioanimvalue_t *)((byte *)pAnim + pAnim->offset[j]);

			k = frame;

			// find span of values that includes the frame we want
			while (pAnimValue->num.total <= k) {
				k -= pAnimValue->num.total;
				pAnimValue += pAnimValue->num.valid + 1;
			}
			// if we're inside the span
			if (pAnimValue->num.valid > k) {
				// and there's more data in the span
				if (pAnimValue->num.valid > k + 1) {
					pos[j] += (pAnimValue[k + 1].value * (1.0 - s) + s * pAnimValue[k + 2].value) * pbone->scale[j];
				} else {
					pos[j] += pAnimValue[k + 1].value * pbone->scale[j];
				}
			} else {
				// are we at the end of the repeating values section and there's another section with data?
				if (pAnimValue->num.total <= k + 1) {
					pos[j] += (pAnimValue[pAnimValue->num.valid].value * (1.0 - s) + s * pAnimValue[pAnimValue->num.valid + 2].value) * pbone->scale[j];
				} else {
					pos[j] += pAnimValue[pAnimValue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if (pbone->bonecontroller[j] != -1) {
			pos[j] += m_adj[pbone->bonecontroller[j]];
		}
	}
}
void StudioModel::CalcRotations(vec3_t *pos, vec4p_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f) {
	int					i;
	int					frame;
	mstudiobone_t		*pbone;
	float				s;

	if (f > pseqdesc->numframes - 1) {
		f = 0.0f;
	} else if (f < -0.01f) {
		f = -0.01f;
	}

	frame = (int)f;
	s = (f - frame);

	// add in programatic controllers
	CalcBoneAdj();

	pbone = (mstudiobone_t *)((byte *)m_pstudiohdr + m_pstudiohdr->boneindex);
	for (i = 0; i < m_pstudiohdr->numbones; i++, pbone++, panim++) {
		CalcBoneQuaternion(frame, s, pbone, panim, q[i]);
		CalcBonePosition(frame, s, pbone, panim, pos[i]);
	}

	if (pseqdesc->motiontype & STUDIO_X)
		pos[pseqdesc->motionbone][0] = 0.0;
	if (pseqdesc->motiontype & STUDIO_Y)
		pos[pseqdesc->motionbone][1] = 0.0;
	if (pseqdesc->motiontype & STUDIO_Z)
		pos[pseqdesc->motionbone][2] = 0.0;
}

mstudioanim_t * StudioModel::GetAnim(mstudioseqdesc_t *pseqdesc) {
	mstudioseqgroup_t	*pseqgroup;
	pseqgroup = (mstudioseqgroup_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0) {
		return (mstudioanim_t *)((byte *)m_pstudiohdr + pseqdesc->animindex);
	}

	return (mstudioanim_t *)((byte *)m_panimhdr[pseqdesc->seqgroup] + pseqdesc->animindex);
}

void StudioModel::SlerpBones(vec4p_t q1[], vec3_t pos1[], vec4p_t q2[], vec3_t pos2[], float s) {
	int			i;
	vec4p_t		q3;
	float		s1;

	if (s < 0) s = 0;
	else if (s > 1.0) s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < m_pstudiohdr->numbones; i++) {
		QuaternionSlerp(q1[i], q2[i], s, q3);
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}
void StudioModel::AdvanceFrame(float dt) {
	if (!m_pstudiohdr)
		return;

	mstudioseqdesc_t	*pseqdesc;
	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex) + m_sequence;

	if (dt > 0.1)
		dt = 0.1f;
	m_frame += dt * pseqdesc->fps;

	if (pseqdesc->numframes <= 1) {
		m_frame = 0;
	} else { // wrap
		m_frame -= (int)(m_frame / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
	}
}
int StudioModel::SetFrame(int nFrame) {
	if (nFrame == -1)
		return m_frame;

	if (!m_pstudiohdr)
		return 0;

	mstudioseqdesc_t	*pseqdesc;
	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex) + m_sequence;

	m_frame = nFrame;

	if (pseqdesc->numframes <= 1) {
		m_frame = 0;
	} else {
		// wrap
		m_frame -= (int)(m_frame / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
	}

	return m_frame;
}
void StudioModel::SetUpBones(bool righthand) {
	int					i;

	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t		*panim;

	static vec3_t		pos[MAXSTUDIOBONES];
	float				bonematrix[3][4];
	static vec4p_t		q[MAXSTUDIOBONES];

	static vec3_t		pos2[MAXSTUDIOBONES];
	static vec4p_t		q2[MAXSTUDIOBONES];
	static vec3_t		pos3[MAXSTUDIOBONES];
	static vec4p_t		q3[MAXSTUDIOBONES];
	static vec3_t		pos4[MAXSTUDIOBONES];
	static vec4p_t		q4[MAXSTUDIOBONES];


	if (m_sequence >= m_pstudiohdr->numseq) {
		m_sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex) + m_sequence;

	panim = GetAnim(pseqdesc);
	CalcRotations(pos, q, pseqdesc, panim, m_frame);

	if (pseqdesc->numblends > 1) {
		float				s;

		panim += m_pstudiohdr->numbones;
		CalcRotations(pos2, q2, pseqdesc, panim, m_frame);
		s = m_blending[0] / 255.0;

		SlerpBones(q, pos, q2, pos2, s);

		if (pseqdesc->numblends == 4) {
			panim += m_pstudiohdr->numbones;
			CalcRotations(pos3, q3, pseqdesc, panim, m_frame);

			panim += m_pstudiohdr->numbones;
			CalcRotations(pos4, q4, pseqdesc, panim, m_frame);

			s = m_blending[0] / 255.0;
			SlerpBones(q3, pos3, q4, pos4, s);

			s = m_blending[1] / 255.0;
			SlerpBones(q, pos, q3, pos3, s);
		}
	}

	float modelAngleMatrix[3][4];
	vec3_t angles;
	VectorSet(angles, 0, 0, 0);

	vec4p_t angleQuat;
	VectorScale(angles, Q_PI / 180, angles);
	AngleQuaternion(angles, angleQuat);


	pbones = (mstudiobone_t *)((byte *)m_pstudiohdr + m_pstudiohdr->boneindex);
#ifdef MAKE_UNBIND_BONE
	for (i = 0; i < m_pstudiohdr->numbones; i++) {
		vec3_t *pAngle = (vec3_t *)(pbones[i].value + 3);
		vec3_t *pPos = (vec3_t *)(pbones[i].value + 0);

		pos[i][0] = pbones[i].value[0];
		pos[i][1] = pbones[i].value[1];
		pos[i][2] = pbones[i].value[2];
		angles[0] = pbones[i].value[3];
		angles[1] = pbones[i].value[4];
		angles[2] = pbones[i].value[5];

		AngleQuaternion(angles, q[i]);

		QuaternionMatrix(q[i], bonematrix);

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) {
			memcpy(m_bonetransform[i], bonematrix, sizeof bonematrix);
		} else {
			R_ConcatTransforms(m_bonetransform[pbones[i].parent], bonematrix, m_bonetransform[i]);
		}
	}
#else
	for (i = 0; i < m_pstudiohdr->numbones; i++) {
		QuaternionMatrix(q[i], bonematrix);

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) {
			QuaternionMatrix(angleQuat, modelAngleMatrix);

			modelAngleMatrix[0][3] = 0;
			modelAngleMatrix[1][3] = 0;
			modelAngleMatrix[2][3] = 0;

			R_ConcatTransforms(modelAngleMatrix, bonematrix, m_bonetransform[i]);
		} else {
			R_ConcatTransforms(m_bonetransform[pbones[i].parent], bonematrix, m_bonetransform[i]);
		}
	}
#endif
}
void StudioModel::SetupModel(int bodypart) {
	int index;

	if (bodypart > m_pstudiohdr->numbodyparts) {
		// Con_DPrintf ("StudioModel::SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	mstudiobodyparts_t   *pbodypart = (mstudiobodyparts_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bodypartindex) + bodypart;

	index = m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	m_pmodel = (mstudiomodel_t *)((byte *)m_pstudiohdr + pbodypart->modelindex) + index;
}
bool StudioModel::hasCSOTexture(const studiohdr_t *phdr) {
	const byte *pin = reinterpret_cast<const byte *>(phdr);
	const mstudiotexture_t *ptextures = reinterpret_cast<const mstudiotexture_t *>(pin + phdr->textureindex);

	if (phdr->textureindex > 0 && phdr->numtextures <= MAXSTUDIOSKINS) {
		int n = phdr->numtextures;
		// return std::any_of(ptextures, ptextures + n, isCSOExternalTexture);
		return true;
	}
	return false;
}
bool StudioModel::isCSOExternalTexture(const mstudiotexture_t &texture) {
	return texture.width == 4 && texture.height == 1 && (texture.name[0] == '#' || texture.name[0] == '$');
}
void StudioModel::ScaleBones(float scale) {
	if (!m_pstudiohdr)
		return;

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)m_pstudiohdr + m_pstudiohdr->boneindex);
	for (int i = 0; i < m_pstudiohdr->numbones; i++) {
		for (int j = 0; j < 3; j++) {
			pbones[i].value[j] *= scale;
			pbones[i].scale[j] *= scale;
		}
	}
}
void StudioModel::ScaleMeshes(float scale) {
	if (!m_pstudiohdr)
		return;

	int i, j, k;

	// scale verts
	int tmp = m_bodynum;
	for (i = 0; i < m_pstudiohdr->numbodyparts; i++) {
		mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bodypartindex) + i;
		for (j = 0; j < pbodypart->nummodels; j++) {
			SetBodygroup(i, j);
			SetupModel(i);

			vec3_t *pstudioverts = (vec3_t *)((byte *)m_pstudiohdr + m_pmodel->vertindex);
			for (k = 0; k < m_pmodel->numverts; k++)
				VectorScale(pstudioverts[k], scale, pstudioverts[k]);
		}
	}

	m_bodynum = tmp;

	// scale complex hitboxes
	mstudiobbox_t *pbboxes = (mstudiobbox_t *)((byte *)m_pstudiohdr + m_pstudiohdr->hitboxindex);
	for (i = 0; i < m_pstudiohdr->numhitboxes; i++) {
		VectorScale(pbboxes[i].bbmin, scale, pbboxes[i].bbmin);
		VectorScale(pbboxes[i].bbmax, scale, pbboxes[i].bbmax);
	}

	// scale bounding boxes
	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex);
	for (i = 0; i < m_pstudiohdr->numseq; i++) {
		VectorScale(pseqdesc[i].bbmin, scale, pseqdesc[i].bbmin);
		VectorScale(pseqdesc[i].bbmax, scale, pseqdesc[i].bbmax);
	}

	// maybe scale exeposition, pivots, attachments
}
int StudioModel::SetSkin(int iValue) {
	if (!m_ptexturehdr)
		return 0;

	if (iValue >= m_ptexturehdr->numskinfamilies) {
		return m_skinnum;
	}

	m_skinnum = iValue;

	return iValue;
}
int StudioModel::SetBodygroup(int iGroup, int iValue) {
	if (!m_pstudiohdr)
		return 0;

	if (iGroup > m_pstudiohdr->numbodyparts)
		return -1;

	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bodypartindex) + iGroup;

	int iCurrent = (m_bodynum / pbodypart->base) % pbodypart->nummodels;

	if (iValue >= pbodypart->nummodels)
		return iCurrent;

	m_bodynum = (m_bodynum - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));

	return iValue;
}
float StudioModel::SetBlending(int iBlender, float flValue) {
	mstudioseqdesc_t	*pseqdesc;

	if (!m_pstudiohdr)
		return 0.0f;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex) + (int)m_sequence;

	if (pseqdesc->blendtype[iBlender] == 0)
		return flValue;

	if (pseqdesc->blendtype[iBlender] & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) {
		// ugly hack, invert value if end < start
		if (pseqdesc->blendend[iBlender] < pseqdesc->blendstart[iBlender])
			flValue = -flValue;

		// does the controller not wrap?
		if (pseqdesc->blendstart[iBlender] + 359.0 >= pseqdesc->blendend[iBlender]) {
			if (flValue > ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.0) - 180)
				flValue = flValue + 360;
		}
	}

	int setting = (int)(255 * (flValue - pseqdesc->blendstart[iBlender]) / (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]));

	if (setting < 0) setting = 0;
	if (setting > 255) setting = 255;

	m_blending[iBlender] = setting;

	return setting * (1.0 / 255.0) * (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]) + pseqdesc->blendstart[iBlender];
}
float StudioModel::SetMouth(float flValue) {
	if (!m_pstudiohdr)
		return 0.0f;

	mstudiobonecontroller_t	*pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bonecontrollerindex);

	// find first controller that matches the mouth
	for (int i = 0; i < m_pstudiohdr->numbonecontrollers; i++, pbonecontroller++) {
		if (pbonecontroller->index == 4)
			break;
	}

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) {
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end) {
			if (flValue >((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		} else {
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	int setting = (int)(64 * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start));

	if (setting < 0) setting = 0;
	if (setting > 64) setting = 64;
	m_mouth = setting;

	return setting * (1.0 / 64.0) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}
float StudioModel::SetController(int iController, float flValue) {
	if (!m_pstudiohdr)
		return 0.0f;

	mstudiobonecontroller_t	*pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pstudiohdr + m_pstudiohdr->bonecontrollerindex);

	// find first controller that matches the index
	int i;
	for (i = 0; i < m_pstudiohdr->numbonecontrollers; i++, pbonecontroller++) {
		if (pbonecontroller->index == iController)
			break;
	}
	if (i >= m_pstudiohdr->numbonecontrollers)
		return flValue;

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR)) {
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end) {
			if (flValue >((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		} else {
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	int setting = (int)(255 * (flValue - pbonecontroller->start) /
		(pbonecontroller->end - pbonecontroller->start));

	if (setting < 0) setting = 0;
	if (setting > 255) setting = 255;
	m_controller[iController] = setting;

	return setting * (1.0 / 255.0) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}

int StudioModel::GetSequence() {
	return m_sequence;
}

int StudioModel::SetSequence(int iSequence) {
	if (iSequence > m_pstudiohdr->numseq)
		return m_sequence;

	m_sequence = iSequence;
	m_frame = 0;

	return m_sequence;
}

void StudioModel::ExtractBbox(float *mins, float *maxs) {
	mstudioseqdesc_t	*pseqdesc;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex);

	mins[0] = pseqdesc[m_sequence].bbmin[0];
	mins[1] = pseqdesc[m_sequence].bbmin[1];
	mins[2] = pseqdesc[m_sequence].bbmin[2];

	maxs[0] = pseqdesc[m_sequence].bbmax[0];
	maxs[1] = pseqdesc[m_sequence].bbmax[1];
	maxs[2] = pseqdesc[m_sequence].bbmax[2];
}
void StudioModel::GetSequenceInfo(float *pflFrameRate, float *pflGroundSpeed) {
	mstudioseqdesc_t	*pseqdesc;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pstudiohdr + m_pstudiohdr->seqindex) + (int)m_sequence;

	if (pseqdesc->numframes > 1) {
		*pflFrameRate = 256 * pseqdesc->fps / (pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrt(pseqdesc->linearmovement[0] * pseqdesc->linearmovement[0] + pseqdesc->linearmovement[1] * pseqdesc->linearmovement[1] + pseqdesc->linearmovement[2] * pseqdesc->linearmovement[2]);
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	} else {
		*pflFrameRate = 256.0;
		*pflGroundSpeed = 0.0;
	}
}
bool StudioModel::PostLoadModel(studiohdr_t *phdr, const char *modelname) {
	studiohdr_t *ptexturehdr = nullptr;
	bool owntexmodel = false;

	// preload textures
	if (phdr->numtextures == 0) {
		char texturename[256];
		strcpy(texturename, modelname);
		StripExtension(texturename);
		strcat(texturename, "t.mdl");

		ptexturehdr = LoadModel(texturename);
		if (!ptexturehdr) {
			FreeModel();
			return false;
		}
		owntexmodel = true;
	} else {
		ptexturehdr = phdr;
		owntexmodel = false;
	}

	// preload animations
	if (phdr->numseqgroups > 1) {
		for (int i = 1; i < phdr->numseqgroups; i++) {
			char seqgroupname[256];

			strcpy(seqgroupname, modelname);
			sprintf(&seqgroupname[strlen(seqgroupname) - 4], "%02d.mdl", i);

			m_panimhdr[i] = LoadModel(seqgroupname);
			if (!m_panimhdr[i]) {
				FreeModel();
				return false;
			}
		}
	}

	m_pstudiohdr = phdr;
	m_ptexturehdr = ptexturehdr;
	m_owntexmodel = owntexmodel;

	SetSequence(0);
	SetController(0, 0.0f);
	SetController(1, 0.0f);
	SetController(2, 0.0f);
	SetController(3, 0.0f);
	SetMouth(0.0f);

	int n;
	for (n = 0; n < phdr->numbodyparts; n++)
		SetBodygroup(n, 0);

	SetSkin(0);
	/*
	vec3_t mins, maxs;
	ExtractBbox (mins, maxs);
	if (mins[2] < 5.0f)
	m_origin[2] = -mins[2];
	*/

	return true;
}
void StudioModel::LoadModelTextures(const studiohdr_t *phdr) {
	const byte *pin = reinterpret_cast<const byte *>(phdr);
	const mstudiotexture_t *ptexture = reinterpret_cast<const mstudiotexture_t *>(pin + phdr->textureindex);

	if (phdr->textureindex > 0 && phdr->numtextures <= MAXSTUDIOSKINS) {
		int n = phdr->numtextures;
		for (int i = 0; i < n; i++) {
			MDL_TestUploadTexture(&ptexture[i], pin + ptexture[i].index, pin + ptexture[i].width * ptexture[i].height + ptexture[i].index);
		}
	}
}
void StudioModel::LoadModelTexturesCSO(studiohdr_t *phdr, const char *texturePath) {
	byte *pin = reinterpret_cast<byte *>(phdr);
	mstudiotexture_t *ptexture = reinterpret_cast<mstudiotexture_t *>(pin + phdr->textureindex);

	if (phdr->textureindex > 0 && phdr->numtextures <= MAXSTUDIOSKINS) {
		int n = phdr->numtextures;
		for (int i = 0; i < n; i++) {
			if (isCSOExternalTexture(ptexture[i])) {
				/*std::string path = texturePath;
				path.push_back('/');
				path += (ptexture[i].name);

				if (UploadTextureTGA(&ptexture[i], path.c_str(), g_texnum) || UploadTextureBMP(&ptexture[i], path.c_str(), g_texnum)) {
					// ...
					char buffer[64] = "*CSO* ";
					strcpy(ptexture[i].name, strcat(buffer, ptexture[i].name));
				} else */ {
					MDL_TestUploadTexture(&ptexture[i], pin + ptexture[i].index, pin + ptexture[i].width * ptexture[i].height + ptexture[i].index);
				}
			} else {
				MDL_TestUploadTexture(&ptexture[i], pin + ptexture[i].index, pin + ptexture[i].width * ptexture[i].height + ptexture[i].index);
			}
		}
	}
}
studiohdr_t *StudioModel::LoadModel(const char *modelname) {
	FILE *fp;
	long size;
	void *buffer;

	if (!modelname)
		return nullptr;

	size = PakLoadFile(modelname, &buffer);
	if (size == -1) {
		return nullptr;
	}
	if (strncmp((const char *)buffer, "IDST", 4) &&
		strncmp((const char *)buffer, "IDSQ", 4)) {
		free(buffer);
		return nullptr;
	}
	return (studiohdr_t *)buffer;
}
void StudioModel::FreeModel() {
	// if (m_pstudiohdr)
	//	free(m_pstudiohdr);

	if (m_ptexturehdr && m_owntexmodel)
		free(m_ptexturehdr);

	m_pstudiohdr = m_ptexturehdr = 0;
	m_owntexmodel = false;

	int i;
	for (i = 0; i < 32; i++) {
		if (m_panimhdr[i]) {
			free(m_panimhdr[i]);
			m_panimhdr[i] = 0;
		}
	}
}

void Load_MDLModel_v10(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.size = -1;
	stream.offset = 0;

	studiohdr_t *phdr = stream.GetPtr<studiohdr_t>();

	if (!g_studioModel.PostLoadModel(phdr, e->name)) {
		return;
	}
	
	g_studioModel.LoadModelTextures(g_studioModel.getTextureHeader());
	g_studioModel.SetSequence(0);
	for (int i = 0; i < phdr->numseq; i++) {
		mstudioseqdesc_t *pSeqDesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex);
		if (strncmp(pSeqDesc[i].label, "idle", 4) == 0) {
			g_studioModel.SetSequence(i);
			break;
		}
	}
	g_studioModel.SetUpBones();
	g_studioModel.SetBlending(0, 0.0);
	g_studioModel.SetBlending(1, 0.0);
	g_studioModel.AdvanceFrame(1.0f);

	int nCountMesh = 0;
	for (int i = 0; i < phdr->numbodyparts; i++) {
		g_studioModel.SetBodygroup(i, 0);
		g_studioModel.SetupModel(i);
		g_studioModel.LoadRadiant(pModel, vMin, vMax, e, nCountMesh);
	}
}




modelDrawVertex_t mdlDrawList[MAXSTUDIOVERTS];
void StudioModel::LoadRadiant(entitymodel *&pModel, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int &nCountMesh) {
	int					i, j;
	mstudiomesh_t		*pmesh;
	byte				*pvertbone;
	byte				*pnormbone;
	vec3_t				*pstudioverts;
	vec3_t				*pstudionorms;
	mstudiotexture_t	*ptexture;
	short				*pskinref;

	pvertbone = ((byte *)m_pstudiohdr + m_pmodel->vertinfoindex);
	pnormbone = ((byte *)m_pstudiohdr + m_pmodel->norminfoindex);
	ptexture = (mstudiotexture_t *)((byte *)m_ptexturehdr + m_ptexturehdr->textureindex);

	pmesh = (mstudiomesh_t *)((byte *)m_pstudiohdr + m_pmodel->meshindex);

	pstudioverts = (vec3_t *)((byte *)m_pstudiohdr + m_pmodel->vertindex);
	pstudionorms = (vec3_t *)((byte *)m_pstudiohdr + m_pmodel->normindex);

	pskinref = (short *)((byte *)m_ptexturehdr + m_ptexturehdr->skinindex);
	if (m_skinnum != 0 && m_skinnum < m_ptexturehdr->numskinfamilies) {
		pskinref += (m_skinnum * m_ptexturehdr->numskinref);
	}
	for (i = 0; i < m_pmodel->numverts; i++) {
		int index = pvertbone[i];
		MDL_VectorTransform(pstudioverts[i], m_bonetransform[index], m_formverts[i]);
	}

	for (j = 0; j < m_pmodel->nummesh; j++) {
		float s, t;
		short *ptricmds;
		float coords2d[2];
		int tri_type;

		pmesh = (mstudiomesh_t *)((byte *)m_pstudiohdr + m_pmodel->meshindex) + j;
		ptricmds = (short *)((byte *)m_pstudiohdr + pmesh->triindex);

		char *strTexName = ptexture[pskinref[pmesh->skinref]].name;
		int nCountVertex = 0;

		s = 1.0/(float)ptexture[pskinref[pmesh->skinref]].width;
		t = 1.0/(float)ptexture[pskinref[pmesh->skinref]].height;

		while ((i = *(ptricmds++))) {
			if (i < 0) {
				tri_type = GL_TRIANGLE_FAN;
				i = -i;
			} else {
				tri_type = GL_TRIANGLE_STRIP;
			}

			int nTriPos = 0, nBasePos = nCountVertex;
			for( ; i > 0; i--, ptricmds += 4)
			{
				vec3_t pos;
				coords2d[0] = ptricmds[2] * s;
				coords2d[1] = ptricmds[3] * t;
				VectorCopy(m_formverts[ptricmds[0]], pos);

				VectorTestSwap(pos);
				
				if (nTriPos < 3) {
					VectorCopy2(coords2d, mdlDrawList[nCountVertex].vTexCoord);
					VectorCopy(pos, mdlDrawList[nCountVertex].vPos);
					nCountVertex++, nTriPos++;
				} else if (tri_type == GL_TRIANGLE_FAN) {
					VectorCopy2(mdlDrawList[nBasePos].vTexCoord, mdlDrawList[nCountVertex + 0].vTexCoord);
					VectorCopy(mdlDrawList[nBasePos].vPos, mdlDrawList[nCountVertex + 0].vPos);
					

					VectorCopy2(mdlDrawList[nCountVertex - 1].vTexCoord, mdlDrawList[nCountVertex + 1].vTexCoord);
					VectorCopy(mdlDrawList[nCountVertex - 1].vPos, mdlDrawList[nCountVertex + 1].vPos);

					VectorCopy2(coords2d, mdlDrawList[nCountVertex + 2].vTexCoord);
					VectorCopy(pos, mdlDrawList[nCountVertex + 2].vPos);

					nCountVertex += 3, nTriPos++;
				} else {
					VectorCopy2(mdlDrawList[nCountVertex - 2].vTexCoord, mdlDrawList[nCountVertex + 0].vTexCoord);
					VectorCopy(mdlDrawList[nCountVertex - 2].vPos, mdlDrawList[nCountVertex + 0].vPos);


					VectorCopy2(mdlDrawList[nCountVertex - 1].vTexCoord, mdlDrawList[nCountVertex + 1].vTexCoord);
					VectorCopy(mdlDrawList[nCountVertex - 1].vPos, mdlDrawList[nCountVertex + 1].vPos);

					VectorCopy2(coords2d, mdlDrawList[nCountVertex + 2].vTexCoord);
					VectorCopy(pos, mdlDrawList[nCountVertex + 2].vPos);

					nCountVertex += 3, nTriPos++;
				}
			}
		}
		if (nCountMesh != 0) {
			pModel->pNext = (entitymodel_t*)qmalloc(sizeof(entitymodel_t));
			pModel = pModel->pNext;
		}
		qtexture_t *pTex = Texture_ForName(strTexName);
		pTex->inuse = false;

		int num_size = nCountVertex / 3;

		pModel->nSkinHeight = 4;
		pModel->nSkinWidth = 4;
		pModel->nTriCount = num_size;
		pModel->nModelPosition = 0;
		pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * num_size);
		pModel->nTextureBind[pModel->nNumTextures++] = pTex->texture_number;

		for (int i = 0; i < nCountVertex; i++) {
			VectorCopy(mdlDrawList[i].vPos, pModel->pTriList[i / 3].v[i % 3]);
			VectorCopy2(mdlDrawList[i].vTexCoord, pModel->pTriList[i / 3].st[i % 3]);
			ExtendBounds(mdlDrawList[i].vPos, vMin, vMax);
		}
		nCountMesh++;
	}
}
