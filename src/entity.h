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
// entity.h

void Eclass_InitForSourceDirectory(char *path);
eclass_t *Eclass_ForName(char *name, qboolean has_brushes);

// forward declare this one
class IPluginEntity;

typedef struct entity_s {
	struct entity_s	*prev, *next;
	brush_t			brushes;					// head/tail of list
	int				undoId, redoId, entityId;	// used for undo/redo
	vec3_t			origin;
	eclass_t *		eclass;
	epair_t *		epairs;
	eclass_t *		md3Class;
	vec3_t			vDecalOrigin;
	vec3_t			vDecalDir;
	vec3_t			vBeamEnd;
	int				nBeamTime;
	char *			strTestAnim;
	IPluginEntity *	pPlugEnt;
	vec3_t			vRotation;   // valid for misc_models only
	vec3_t			vScale;      // valid for misc_models only
} entity_t;

char *	ValueForKey(entity_t *ent, const char *key, const char *defs = "");
void	SetKeyValue(entity_t *ent, const char *key, const char *value);
void 	SetKeyValue(epair_t *&e, const char *key, const char *value);
void 	DeleteKey(entity_t *ent, const char *key);
void 	DeleteKey(epair_t *&e, const char *key);
float	FloatForKey(entity_t *ent, const char *key, const char *defs = "0.0");
int		IntForKey(entity_t *ent, const char *key, const char *defs = "0");
void 	GetVectorForKey(entity_t *ent, const char *key, vec3_t vec);

void		Entity_Free(entity_t *e);
void		Entity_FreeEpairs(entity_t *e);
int			Entity_MemorySize(entity_t *e);
entity_t	*Entity_Parse(qboolean onlypairs, brush_t* pList = NULL);
void		Entity_Write(entity_t *e, FILE *f, qboolean use_region);
void		Entity_WriteSelected(entity_t *e, FILE *f);
void		Entity_WriteSelected(entity_t *e, CMemFile*);
entity_t	*Entity_Create(eclass_t *c);
entity_t	*Entity_Clone(entity_t *e);
void		Entity_AddToList(entity_t *e, entity_t *list);
void		Entity_RemoveFromList(entity_t *e);

void		Entity_LinkBrush(entity_t *e, brush_t *b);
void		Entity_UnlinkBrush(brush_t *b);
entity_t *	FindEntity(char *pszKey, char *pszValue);
entity_t *	FindEntityInt(char *pszKey, int iValue);
entity_t *	FindEntityForKey(entity_t *e, char *pszKey);

int GetUniqueTargetId(int iHint);
qboolean Eclass_HasModel(eclass_t *e, vec3_t &vMin, vec3_t &vMax);
eclass_t* GetCachedModel(entity_t *pEntity, const char *pName, vec3_t &vMin, vec3_t &vMax);
eclass_t *GetCachedModel(const char *pName, vec3_t &vMin, vec3_t &vMax, int nFlags);

void Load_ASEModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, int nModelIndex);
void Load_MDCModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nModelIndex);
void Load_MD3Model(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nModelIndex);
void Load_MD2Model(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nModelIndex);
void Load_MDXModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nMeshId);
void Load_BSPModel_v29(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, int nModelIndex);
void Load_MDLModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
void Load_SPRModel_v1(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
void Load_SPRModel_v2(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, bool bIsEditor);
void Load_DecalModel(entitymodel *&pModel, qtexture_t *qtex, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
void Model_SpriteView(entitymodel *&pModel, vec3_t angles, vec3_t origin, vec3_t &vMin, vec3_t &vMax);
void Model_BeamView(entitymodel *&pModel, vec3_t start, vec3_t end, int scale, int scroll, vec3_t &vMin, vec3_t &vMax);
void Model_DecalView(entitymodel *&pModel, vec3_t normal, vec3_t &vMin, vec3_t &vMax);
int PROG_LoadSkin(byte *pic, int width, int height, bool bIsAlpha);
void Load_MDSModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
void Load_MD4Model(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
void Load_MDMModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
void Load_MDLModel_v10(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e);
//Timo : used for parsing epairs in brush primitive
epair_t* ParseEpair(void);
char *ValueForKey(epair_t *&e, const char *key);

anim_t *FindAnimState(eclass_t *pEclass, char *pszName, bool makeAnim, int nFrameCount);
void WOLF_FindIdleFrame(eclass_t *e, int &nStartFrame, int &nNumFrame);


