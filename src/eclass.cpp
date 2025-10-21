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
#include "stdafx.h"
#include "qe3.h"
#include "io.h"
//#include "qertypes.h"

eclass_t	*eclass = NULL;
eclass_t	*eclass_bad = NULL;
char		eclass_directory[1024];


// md3 cache for misc_models
eclass_t *g_md3Cache = NULL;

/*

the classname, color triple, and bounding box are parsed out of comments
A ? size means take the exact brush size.

- QUAKED <classname> (0 0 0) ?
- QUAKED <classname> (0 0 0) (-8 -8 -8) (8 8 8)

Flag names can follow the size description:

- QUAKED func_door (0 .5 .8) ? START_OPEN STONE_SOUND DOOR_DONT_LINK GOLD_KEY SILVER_KEY

*/

void FreeEntityModel(entitymodel_t *&model) {
	entitymodel_t *pNext;
	while (model) {
		pNext = model->pNext;
		if (model->pTriList) {
			free(model->pTriList);
		}
		free(model);
		model = pNext;
	}
	model = nullptr;
}


void FreeEpirs(epair_t *&epairs) {
	epair_t	*next, *ep;

	for (ep = epairs; ep; ep = next) {
		next = ep->next;
		free(ep->key);
		free(ep->value);
		free(ep);
	}
	epairs = nullptr;
}

void FreeAnims(anim_t *&pAnim) {
	while (pAnim) {
		anim_t *pNext = pAnim->pNext;
		free(pAnim->pszName);
		pAnim = pNext;
	}
	pAnim = nullptr;
}

void CleanEntityList(eclass_t *&pList) {
	while (pList) {
		eclass_t* pTemp = pList->next;

		entitymodel *model = pList->model;
		if (model) {
			FreeEntityModel(model);
		}

		FreeEpirs(pList->epairs);
		FreeAnims(pList->pAnims);

		if (pList->modelpath)
			free(pList->modelpath);
		if (pList->skinpath)			// PGM
			free(pList->skinpath);		// PGM

		free(pList->name);
		free(pList->comments);
		free(pList);
		pList = pTemp;
	}
	pList = NULL;

}

void RemoveAnimIndex(char *pos) {
	do {
		if (isdigit(*pos)) {
			*pos = 0;
			break;
		}
		pos++;
	} while (*pos);
}

anim_t *FindAnimState(eclass_t *pEclass, char *pszName) {
	for (anim_t *pAnim = pEclass->pAnims; pAnim; pAnim = pAnim->pNext) {
		if (strcmp(pAnim->pszName, pszName) == 0) {
			return pAnim;
		}
	}
	return nullptr;
}


anim_t *AllocAnimState(eclass_t *pEclass, char *pszName) {
	anim_t *pAnim = (anim_t *)qmalloc(sizeof(pAnim));
	pAnim->pszName = strdup(pszName);
	ResetBound(pAnim->vMin, pAnim->vMax);
	pAnim->pNext = pEclass->pAnims;
	pEclass->pAnims = pAnim;
	return pAnim;
}


void CleanUpEntities() {
	CleanEntityList(eclass);
	CleanEntityList(g_md3Cache);

	eclass = NULL;
	if (eclass_bad) {
		free(eclass_bad->name);
		free(eclass_bad->comments);
		free(eclass_bad);
		eclass_bad = NULL;
	}
}

void ExtendBounds(vec3_t v, vec3_t &vMin, vec3_t &vMax) {
	for (int i = 0; i < 3; i++) {
		vec_t f = v[i];

		if (f < vMin[i]) {
			vMin[i] = f;
		}

		if (f > vMax[i]) {
			vMax[i] = f;
		}
	}
}

void ResetBound(vec3_t &vMin, vec3_t &vMax) {
	vMin[0] = vMin[1] = vMin[2] = 99999;
	vMax[0] = vMax[1] = vMax[2] = -99999;
}

// FIXME: this code is a TOTAL clusterfuck
//
void LoadModel(const char *pLocation, eclass_t *e, vec3_t &vMin, vec3_t &vMax, entitymodel *&pModel, int nModelIndex) {
	// this assumes a path only and uses tris.md2
	// for the model and skin.pcx for the skin
	char exts[24];
	char cfilename[1024];
	byte *buffer;

	ResetBound(vMin, vMax);
	
	if (IsGame(GAME_HL) && e->nShowFlags & ECLASS_DECAL) {
		qtexture_t *texture = Texture_ForName(pLocation);
		Load_DecalModel(pModel, texture, vMin, vMax, e);
		return;
	}

	QE_ConvertDOSToUnixName(cfilename, pLocation);
	ExtractFileExtension(cfilename, exts);
	
	Sys_Printf("Loading model %s...", cfilename);
	int len = PakLoadFile(cfilename, (void **)&buffer);
	if (len == -1 && IsGame(GAME_HL) && strcmp(exts, "spr") == 0) {
		sprintf(cfilename, "%s/sprites/%s", g_strAppPath, pLocation);
		len = LoadFile(cfilename, (void **)&buffer);
		if (len != -1) {
			pModel->bIsEditor = true;
		}
	}
	if (len == -1 && IsGame(GAME_ET | GAME_WOLF) && strcmp(exts, "md3") == 0) {
		StripExtension(cfilename);
		strcat(cfilename, ".mdc");
		len = PakLoadFile(cfilename, (void **)&buffer);
		if (len != -1) {
			ExtractFileExtension(cfilename, exts);
		}
	}
	if (len == -1) {
		Sys_Printf(" failed.\n");
		return;
	}
	Sys_Printf(" successful.\n");

	if (IsGame(GAME_ID3)) {
		if (strcmp(exts, "md3") == 0) {
			Load_MD3Model(pModel, buffer, vMin, vMax, e, nModelIndex);
		} else if (strcmp(exts, "mdc") == 0) {
			Load_MDCModel(pModel, buffer, vMin, vMax, e, nModelIndex); // !?
		} else if (strcmp(exts, "ase") == 0) {
			Load_ASEModel(pModel, buffer, vMin, vMax, nModelIndex);
		}
	} else if (IsGame(GAME_Q2) && strcmp(exts, "md2") == 0) {
		Load_MD2Model(pModel, buffer, vMin, vMax, e, nModelIndex);
	} else if (IsGame(GAME_Q1 | GAME_HX2)) {
		if (strcmp(exts, "mdl") == 0) {
			Load_MDLModel(pModel, buffer, vMin, vMax, nModelIndex);
		} else if (strcmp(exts, "spr") == 0) {
			Load_SPRModel_v1(pModel, buffer, vMin, vMax, e);
		} else if (strcmp(exts, "bsp") == 0) {
			Load_BSPModel_v29(pModel, buffer, vMin, vMax, nModelIndex);
		}
	} else if (IsGame(GAME_HL)) {
		if (strcmp(exts, "spr") == 0) {
			Load_SPRModel_v2(pModel, buffer, vMin, vMax, e);
		}
	}
	free(buffer);
}

char	*debugname;

eclass_t *Eclass_InitFromText(char *source) {
	eclass_t *e;
	char	color[128];
	
	source += strlen("/*QUAKED ");

	e = (eclass_t *)qmalloc(sizeof(*e));
	e->name = debugname = Lex_ReadToken(source, true, true);
	if (!Lex_ParseVector(source, e->color)) {
		return nullptr;
	}
	sprintf(color, "(%f %f %f)", e->color[0], e->color[1], e->color[2]);
	e->texdef.SetName(color);
	if (Lex_CheckToken(source, "?")) {
		e->fixedsize = false;
	} else {
		e->fixedsize = true;
		if (!Lex_ParseVector(source, e->mins)) {
			return nullptr;
		}
		if (!Lex_ParseVector(source, e->maxs)) {
			return nullptr;
		}
	}
	for (int i = 0; i < MAX_FLAGS; i++) {
		char *name = Lex_ReadToken(source, true);
		if (!name) {
			break;
		}
		strcpy(e->flagnames[i], name);
	}
	Lex_NextLine(source);

	char *mark = source;
	int len = 0;
	while (*source && !(*source == '*' && source[1] == '/')) {
		source++, len++;
	}
	e->comments = (char*)qmalloc(len + 1);
	memcpy(e->comments, mark, len);

	mark = e->comments;
	while (true) {
		char *token = Lex_ReadToken(mark);
		if (!token) {
			break;
		}
		CString strKey = token;
		if (!Lex_CheckToken(mark, "=")) {
			continue;
		}
		if (strKey == "model") {
			e->modelpath = Lex_ReadToken(mark, true, true);
		} else if (strKey == "skin") {
			e->skinpath = Lex_ReadToken(mark, true, true);
		} else if (strKey == "frame") {
			e->nFrame = Lex_IntValue(mark, true);
		} else if (strKey == "flagmodels") {
			int index = Lex_IntValue(mark, true);
			e->flagmodels[index] = Lex_ReadToken(mark, true, true);
		} else if (strKey == "editor") {
			epair_t *ep = (epair_t *)qmalloc(sizeof(epair_t));
			ep->key = Lex_ReadToken(mark, true, true);
			ep->value = Lex_ReadToken(mark, true, true);
			ep->next = e->epairs;
			e->epairs = ep;
		}
	}

	// setup show flags
	e->nShowFlags = 0;
	if (strcmpi(e->name, "light") == 0) {
		e->nShowFlags |= ECLASS_LIGHT;
	}

	if (IsGame(GAME_WOLF | GAME_ET) && strcmpi(e->name, "lightJunior") == 0) {
		e->nShowFlags |= ECLASS_LIGHT;
	}
	if (IsGame(GAME_HL)) {
		if (strcmp(e->name, "env_sprite") == 0) {
			e->nShowFlags |= ECLASS_SPRITE;
		}
		if (strcmp(e->name, "env_beam") == 0) {
			e->nShowFlags |= ECLASS_BEAM;
		}
		if (strcmp(e->name, "infodecal") == 0) {
			e->nShowFlags |= ECLASS_DECAL;
		}
		if (strcmp(e->name, "multi_manager") == 0) {
			e->nShowFlags |= ECLASS_MULTI_MANAGER;
		}
		if (strcmp(e->name, "multisource") == 0) {
			e->nShowFlags |= ECLASS_MULTISOURCE;
		}
		if (strcmp(e->name, "ambient_generic") == 0) {
			e->nShowFlags |= ECLASS_SOUND;
		}
		if (strcmp(e->name, "light_spot") == 0 || strcmp(e->name, "light_environment") == 0) {
			e->nShowFlags |= ECLASS_LIGHT;
		}
	}
	if (IsGame(GAME_Q1)) {
		if (strcmp(e->name, "light_globe") == 0) {
			e->nShowFlags |= ECLASS_SPRITE;
		}
		if (strcmp(e->name, "info_intermission") == 0) {
			e->nShowFlags |= ECLASS_INTERMISSION;
		}
		if (strcmpi(e->name, "light_fluoro") == 0 || strcmpi(e->name, "light_fluorospark") == 0) {
			e->nShowFlags |= ECLASS_LIGHT;
		}
	}
	if (IsGame(GAME_Q2)) {
		if (strcmp(e->name, "target_spawner") == 0) {
			e->nShowFlags |= ECLASS_TARGET_SPAWNER;
		}
	}

	if (strcmpi(e->name, "path") == 0) {
		e->nShowFlags |= ECLASS_PATH;
	}
	if (strcmpi(e->name, "misc_model") == 0 || strcmpi(e->name, "misc_gamemodel") == 0) {
		e->nShowFlags |= ECLASS_MISCMODEL;
	}
	return e;
}

qboolean Eclass_HasModel(eclass_t *e, vec3_t &vMin, vec3_t &vMax) {
	if (e->modelpath != NULL) {
		if (e->model == NULL) {
			e->model = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
		}
		CStringArray lstModels;
		SplitList(e->modelpath, ";", lstModels);

		entitymodel *model = e->model;
		for (int i = 0; i < lstModels.GetCount(); i++) {
			if (i != 0) {
				model->pNext = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
				model = model->pNext;
			}
			CString pModelPath = lstModels.GetAt(i);
			LoadModel(pModelPath, e, vMin, vMax, model, i);
		}

		// at this poitn vMin and vMax contain the min max of the model
		// which needs to be centered at origin 0, 0, 0

		VectorSnap(vMin);
		VectorSnap(vMax);

		int max = 0;
		for(int i = 0; i < 3; i++) {
			if (vMax[i] - vMin[i] < 2) {
				vMin[i] -= 1, vMax[i] += 1;
			}
			max = Q_MAX(vMax[i], max);
		}

		free(e->modelpath);
		e->modelpath = NULL;
	}
	return (e->model != NULL && e->model->nTriCount > 0);
}


void EClass_InsertSortedList(eclass_t *&pList, eclass_t *e) {
	eclass_t *s;
	if (!pList) {
		pList = e;
		return;
	}
	s = pList;
	if (stricmp(e->name, s->name) < 0) {
		e->next = s;
		pList = e;
		return;
	}
	do {
		if (!s->next || stricmp(e->name, s->next->name) < 0) {
			e->next = s->next;
			s->next = e;
			return;
		}
		s = s->next;
	} while (1);
}

/*
=================
Eclass_InsertAlphabetized
=================
*/
void Eclass_InsertAlphabetized(eclass_t *e) {
#if 1
	EClass_InsertSortedList(eclass, e);
#else
	eclass_t	*s;
	if (!eclass) {
		eclass = e;
		return;
	}
	s = eclass;
	if (stricmp(e->name, s->name) < 0) {
		e->next = s;
		eclass = e;
		return;
	}
	do {
		if (!s->next || stricmp(e->name, s->next->name) < 0) {
			e->next = s->next;
			s->next = e;
			return;
		}
		s = s->next;
	} while (1);
#endif
}


/*
=================
Eclass_ScanFile
=================
*/

qboolean parsing_single = false;
qboolean eclass_found;
eclass_t *eclass_e;
//#ifdef BUILD_LIST
extern bool g_bBuildList;
CString strDefFile;
//#endif
void Eclass_ScanFile(char *filename) {
	int		size;
	char	*data;
	eclass_t	*e;
	int		i;
	char    temp[1024];

	QE_ConvertDOSToUnixName(temp, filename);

	Sys_Printf("ScanFile: %s\n", temp);

	// BUG
	size = LoadFile(filename, (void**)&data);
	eclass_found = false;
	for (i = 0; i < size; i++)
		if (!strncmp(data + i, "/*QUAKED", 8)) {

			//#ifdef BUILD_LIST
			if (g_bBuildList) {
				CString strDef = "";
				int j = i;
				while (1) {
					strDef += *(data + j);
					if (*(data + j) == '/' && *(data + j - 1) == '*')
						break;
					j++;
				}
				strDef += "\r\n\r\n\r\n";
				strDefFile += strDef;
			}
			//#endif
			e = Eclass_InitFromText(data + i);
			if (e)
				Eclass_InsertAlphabetized(e);
			else
				printf("Error parsing: %s in %s\n", debugname, filename);

			// single ?
			eclass_e = e;
			eclass_found = true;
			if (parsing_single)
				break;
		}

	free(data);
}



void Eclass_InitForSourceDirectory(char *path) {
	struct _finddata_t fileinfo;
	int		handle;
	char	filename[1024];
	char	filebase[1024];
	char    temp[1024];
	char	*s;

	QE_ConvertDOSToUnixName(temp, path);

	Sys_Printf("Eclass_InitForSourceDirectory: %s\n", temp);

	strcpy(filebase, path);
	s = filebase + strlen(filebase) - 1;
	while (*s != '\\' && *s != '/' && s != filebase)
		s--;
	*s = 0;

	CleanUpEntities();
	eclass = NULL;
	//#ifdef BUILD_LIST
	if (g_bBuildList)
		strDefFile = "";
	//#endif
	handle = _findfirst(path, &fileinfo);
	if (handle != -1) {
		do {
			sprintf(filename, "%s\\%s", filebase, fileinfo.name);
			Eclass_ScanFile(filename);
		} while (_findnext(handle, &fileinfo) != -1);

		_findclose(handle);
	}

	//#ifdef BUILD_LIST
	if (g_bBuildList) {
		CFile file;
		if (file.Open("c:\\entities.def", CFile::modeCreate | CFile::modeWrite)) {
			file.Write(strDefFile.GetBuffer(0), strDefFile.GetLength());
			file.Close();
		}
	}
	//#endif

	eclass_bad = Eclass_InitFromText("/*QUAKED UNKNOWN_CLASS (0 0.5 0) ?");
}

eclass_t *Eclass_ForName(char *name, qboolean has_brushes) {
	eclass_t	*e;
	char		init[1024];

#ifdef _DEBUG
	// grouping stuff, not an eclass
	if (strcmp(name, "group_info") == 0)
		Sys_Printf("WARNING: unexpected group_info entity in Eclass_ForName\n");
#endif

	if (!name)
		return eclass_bad;

	for (e = eclass; e; e = e->next) {
		if (!strcmp(name, e->name)) {
			return e;
		}
	}

	// create a new class for it
	if (has_brushes) {
		sprintf(init, "/*QUAKED %s (0 0.5 0) ?\nNot found in source.\n*/", name);
		e = Eclass_InitFromText(init);
	} else {
		sprintf(init, "/*QUAKED %s (0 0.5 0) (-8 -8 -8) (8 8 8)\nNot found in source.\n*/", name);
		e = Eclass_InitFromText(init);
	}

	Eclass_InsertAlphabetized(e);

	return e;
}

eclass_t *GetCachedModel(const char *pName, vec3_t &vMin, vec3_t &vMax, int nFlags) {
	eclass_t *e = NULL;

	for (e = g_md3Cache; e; e = e->next) {
		if (!strcmp(pName, e->name)) {
			VectorCopy(e->mins, vMin);
			VectorCopy(e->maxs, vMax);
			return e;
		}
	}
	e = (eclass_t*)qmalloc(sizeof(*e));
	memset(e, 0, sizeof(*e));
	e->name = strdup(pName);
	e->modelpath = strdup(pName);
	e->skinpath = nullptr;
	e->nShowFlags = nFlags;
	e->color[0] = e->color[2] = 0.85f;

	if (Eclass_HasModel(e, vMin, vMax)) {
		EClass_InsertSortedList(g_md3Cache, e);
		VectorCopy(vMin, e->mins);
		VectorCopy(vMax, e->maxs);
		return e;
	}

	return NULL;
}

eclass_t* GetCachedModel(entity_t *pEntity, const char *pName, vec3_t &vMin, vec3_t &vMax) {
	eclass_t *e = NULL;
	if (pName == NULL || strlen(pName) == 0) {
		return NULL;
	}
	e = GetCachedModel(pName, vMin, vMax, pEntity->eclass->nShowFlags);
	if (!e) {
		return nullptr;
	}
	pEntity->md3Class = e;
	return e;
}


void Load_ASEModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, int nModelIndex) {
	pModel->nModelPosition = 0;
	pModel->nTextureBind[0] = -1;
	pModel->nNumTextures = 0;
	pModel->pNext = nullptr;

	vec3_t *pVertex = nullptr;
	vec3_t *pTexCoord = nullptr;
	qtexture_t *pMaterials[8];
	int num_materials = 0;

	char *script_p = (char *)buffer;
	while (true) {
		char *token = Lex_ReadToken(script_p);
		if (!token) {
			break;
		}
		if (strcmp(token, "*") == 0) {
			char *token = Lex_ReadToken(script_p);
			if (strcmp(token, "MATERIAL_COUNT") == 0) {
				int size = Lex_IntValue(script_p, true);
				assert(size < 8);
			} else if (strcmp(token, "MATERIAL_NAME") == 0) {
				const char *skin = Lex_ReadToken(script_p, true);
				pMaterials[num_materials++] = Texture_ForName(skin);
			} else if (strcmp(token, "MESH_NUMVERTEX") == 0) {
				int num_vertices = Lex_IntValue(script_p, true);
				pVertex = (vec3_t *)qmalloc(sizeof(vec3_t) * num_vertices);
			} else if (strcmp(token, "MESH_VERTEX") == 0) {
				int index = Lex_IntValue(script_p, true);
				vec3_t &pos = pVertex[index];
				pos[0] = Lex_FloatValue(script_p, true);
				pos[1] = Lex_FloatValue(script_p, true);
				pos[2] = Lex_FloatValue(script_p, true);
#if 0
			} else if (strcmp(token, "MESH_NUMFACES") == 0) {
				int size = Lex_IntValue(script_p, true);
				pIndices = (unsigned short *)qmalloc(sizeof(unsigned short) * 4 * size);
			} else if (strcmp(token, "MESH_FACE") == 0) {
				int index = Lex_IntValue(script_p, true);
				Lex_CheckToken(script_p, ":", true);

				int num = 0;
				while (true) {
					char *token = Lex_ReadToken(script_p, true);
					if (token[0] != ('A' + num)) {
						break;
					}
					Lex_ExpectToken(script_p, ":", true);
					unsigned short value = Lex_IntValue(script_p, true);
					if (num > 2) {
						pIndices[num_indices++] = pIndices[0];
						pIndices[num_indices++] = pIndices[num_indices - 1];
					}
					pIndices[num_indices++] = value;
					num++;
				}
#else
			} else if (strcmp(token, "MESH_NUMTVERTEX") == 0) {
				int num_textcoord = Lex_IntValue(script_p, true);
				pTexCoord = (vec3_t *)qmalloc(sizeof(vec3_t) * num_textcoord);
			} else if (strcmp(token, "MESH_TVERT") == 0) {
				int index = Lex_IntValue(script_p, true);
				vec3_t &pos = pTexCoord[index];
				pos[0] = Lex_FloatValue(script_p, true);
				pos[1] = Lex_FloatValue(script_p, true);
				pos[2] = Lex_FloatValue(script_p, true);
			} else if (strcmp(token, "MESH_NUMTVFACES") == 0) {
				int num_indices = Lex_IntValue(script_p, true);

				pModel->nTriCount = num_indices;
				pModel->pTriList = (trimodel_t *)qmalloc(sizeof(trimodel_t) * num_indices);
			} else if (strcmp(token, "MESH_TFACE") == 0) {
				int index = Lex_IntValue(script_p, true);

				int tris[3];
				tris[0] = Lex_IntValue(script_p, true);
				tris[1] = Lex_IntValue(script_p, true);
				tris[2] = Lex_IntValue(script_p, true);

				for (int i = 0; i < 3; i++) {
					int px = tris[i];
					vec3_t &pos = pVertex[px];
					vec3_t &coord = pTexCoord[px];
					
					int rev = 2 - i;
					for (int j = 0; j < 3; j++) {
						pModel->pTriList[index].v[rev][j] = pos[j];
						if (j != 2) {
							pModel->pTriList[index].st[rev][j] = coord[j];
						}
					}
					ExtendBounds(pModel->pTriList[index].v[rev], vMin, vMax);
				}
#endif
			} else if (strcmp(token, "MATERIAL_REF") == 0) {
				int index = Lex_IntValue(script_p, true);
				pModel->nTextureBind[pModel->nNumTextures++] = pMaterials[index]->texture_number;
				pModel->nSkinWidth = pMaterials[index]->width;
				pModel->nSkinHeight = pMaterials[index]->height;
				pModel->nModelIndex = nModelIndex;

				// next surface
				pModel->pNext = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
				pModel = pModel->pNext;
				free(pVertex);
				free(pTexCoord);
			}
		}
		Lex_NextLine(script_p);
	}
	// Sys_Printf(" Conversion from ASE failed.\n");
}

void Load_MDCModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nModelIndex) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.offset = 0;
	stream.size = -1;

	mdcHeader_t *header = stream.GetPtr<mdcHeader_t>();

	int nSurfaceOffset = header->ofsSurfaces;
	for (int s = 0; s < header->numSurfaces; s++) {

		mdcSurface_t *pSurface = stream.GetOffset<mdcSurface_t>(nSurfaceOffset);

		md3St_t *pST = stream.GetOffset<md3St_t>(nSurfaceOffset + pSurface->ofsSt);
		md3Triangle_t *pTris = stream.GetOffset<md3Triangle_t>(nSurfaceOffset + pSurface->ofsTriangles);
		md3XyzNormal_t *pXyz = stream.GetOffset<md3XyzNormal_t>(nSurfaceOffset + pSurface->ofsXyzNormals);
		int nTris = pSurface->numTriangles;

		if (e->nFrame < pSurface->numBaseFrames) { // next frame
			pXyz += (e->nFrame * pSurface->numVerts);
		}

		if (s != 0) {
			pModel->pNext = (entitymodel_t*)qmalloc(sizeof(entitymodel_t));
			pModel = pModel->pNext;
		}
		pModel->nModelPosition = 0;
		pModel->pTriList = new trimodel[nTris];
		pModel->nTriCount = nTris;
		pModel->nModelIndex = nModelIndex;

		int nStart = 0;
		for (int i = 0; i < nTris; i++) {
			for (int k = 0; k < 3; k++) {
				int index = pTris[i].indexes[k];
				for (int j = 0; j < 3; j++) {
					pModel->pTriList[nStart].v[k][j] = pXyz[index].xyz[j] * MD3_XYZ_SCALE;
				}
				pModel->pTriList[nStart].st[k][0] = pST[pTris[i].indexes[k]].st[0];
				pModel->pTriList[nStart].st[k][1] = pST[pTris[i].indexes[k]].st[1];
				ExtendBounds(pModel->pTriList[nStart].v[k], vMin, vMax);
			}
			nStart++;
		}
		md3Shader_t *pShader = stream.GetOffset<md3Shader_t>(nSurfaceOffset + pSurface->ofsShaders);
		qtexture_t *q = Texture_ForName(pShader->name);
		if (q != nullptr) {
			pModel->nTextureBind[pModel->nNumTextures++] = q->texture_number;
		} else {
			Sys_Printf("Model skin load failed on texture %s\n", pShader->name);
		}
		nSurfaceOffset += pSurface->ofsEnd;
	}
}

void Load_MD3Model(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nModelIndex) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.offset = 0;
	stream.size = -1;

	md3Header_t *header = stream.GetPtr<md3Header_t>();
	
	int nSurfaceOffset = header->ofsSurfaces;
	for(int s = 0; s < header->numSurfaces; s++) {

		md3Surface_t *pSurface = stream.GetOffset<md3Surface_t>(nSurfaceOffset);
		
		md3St_t *pST = stream.GetOffset<md3St_t>(nSurfaceOffset + pSurface->ofsSt);
		md3Triangle_t *pTris = stream.GetOffset<md3Triangle_t>(nSurfaceOffset + pSurface->ofsTriangles);
		md3XyzNormal_t *pXyz = stream.GetOffset<md3XyzNormal_t>(nSurfaceOffset + pSurface->ofsXyzNormals);
		int nTris = pSurface->numTriangles;

		if (e->nFrame < pSurface->numFrames) { // next frame
			pXyz += (e->nFrame * pSurface->numVerts);
		}

		if (s != 0) {
			pModel->pNext = (entitymodel_t*)qmalloc(sizeof(entitymodel_t));
			pModel = pModel->pNext;
		}
		pModel->nModelPosition = 0;
		pModel->pTriList = new trimodel[nTris];
		pModel->nTriCount = nTris;
		pModel->nModelIndex = nModelIndex;

		int nStart = 0;
		for (int i = 0; i < nTris; i++) {
			for (int k = 0; k < 3; k++) {
				int index = pTris[i].indexes[k];
				for (int j = 0; j < 3; j++) {
					pModel->pTriList[nStart].v[k][j] = pXyz[index].xyz[j] * MD3_XYZ_SCALE;
				}
				pModel->pTriList[nStart].st[k][0] = pST[pTris[i].indexes[k]].st[0];
				pModel->pTriList[nStart].st[k][1] = pST[pTris[i].indexes[k]].st[1];
				ExtendBounds(pModel->pTriList[nStart].v[k], vMin, vMax);
			}
			nStart++;
		}
		md3Shader_t *pShader = stream.GetOffset<md3Shader_t>(nSurfaceOffset + pSurface->ofsShaders);
		qtexture_t *q = Texture_ForName(pShader->name);
		if (q != nullptr) {
			pModel->nTextureBind[pModel->nNumTextures++] = q->texture_number;
		} else {
			Sys_Printf("Model skin load failed on texture %s\n", pShader->name);
		}
		nSurfaceOffset += pSurface->ofsEnd;
	}
}

void Load_MD2Model(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e, int nModelIndex) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.offset = 0;
	stream.size = -1;

	dmdl_t *header = stream.GetPtr<dmdl_t>();

	dstvert_t *pST = stream.GetOffset<dstvert_t>(header->ofs_st);
	CString strSkin = stream.GetOffset<char>(header->ofs_skins);
	dtriangle_t *pTris = stream.GetOffset<dtriangle_t>(header->ofs_tris);
	// daliasframe_t *frame = stream.GetOffset<daliasframe_t>(header->ofs_frames);
	int nTris = header->num_tris;


	if (strSkin[0] == -128) {
		if (e->skinpath != nullptr) {
			strSkin = e->skinpath;
		} else {
			strSkin = e->modelpath;
			strSkin.Replace("tris.md2", "razor.pcx");
		}
	}
	int nSkinWidth, nSkinHeight;
	int nTex = Texture_LoadSkin(strSkin.GetBuffer(), &nSkinWidth, &nSkinHeight);
	if (nTex == -1) {
		Sys_Printf("Model skin load failed on texture %s\n", strSkin);
	}
	// daliasframe_t *frame = stream.GetOffset<daliasframe_t>(header->ofs_frames);
	for (int nFrame = 0; nFrame < header->num_frames; nFrame++) {
		daliasframe_t *frame = stream.GetOffset<daliasframe_t>(header->ofs_frames + header->framesize * nFrame);
		// Sys_Printf("Frame: %s\n", frame->name);

		if (strncmp(frame->name, "stand", 5) != 0) {
			continue;
		}
		pModel->nSkinWidth = nSkinWidth;
		pModel->nSkinHeight = nSkinHeight;
		pModel->nModelPosition = 0;
		pModel->pTriList = new trimodel[nTris];
		pModel->nTriCount = nTris;
		pModel->nModelIndex = nModelIndex;
		pModel->nFrameIndex = nFrame;
		pModel->nTextureBind[pModel->nNumTextures++] = nTex;
		for (int i = 0; i < nTris; i++) {
			dtriangle_t *tri = &pTris[i];

			for (int k = 0; k < 3; k++) {
				for (int j = 0; j < 3; j++) {
					int index = tri->index_xyz[k];
					pModel->pTriList[i].v[k][j] = (frame->verts[index].v[j] * frame->scale[j] + frame->translate[j]);
				}

				pModel->pTriList[i].st[k][0] = float(pST[tri->index_st[k]].s) / pModel->nSkinWidth;
				pModel->pTriList[i].st[k][1] = float(pST[tri->index_st[k]].t) / pModel->nSkinHeight;
				ExtendBounds(pModel->pTriList[i].v[k], vMin, vMax);
			}
		}
		break;
	}
}




#define VectorCopy2(a,b) {b[0]=a[0];b[1]=a[1];}

void Load_BSPModel_v29(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, int nModelIndex) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.offset = 0;
	stream.size = -1;

	lump_t *pos;
	q1_dheader_t *header = stream.GetPtr<q1_dheader_t>(1);
	// header->ident // TODO check bsp

	pos = &header->lumps[BSP29_LUMP_VERTEXES];
	int num_vertex = pos->filelen / sizeof(q1_dvertex_t);
	q1_dvertex_t *vertexes = stream.GetOffset<q1_dvertex_t>(pos->fileofs, num_vertex);

	pos = &header->lumps[BSP29_LUMP_EDGES];
	int num_edges = pos->filelen / sizeof(q1_dedge_t);
	q1_dedge_t *edges = stream.GetOffset<q1_dedge_t>(pos->fileofs, num_edges);

	pos = &header->lumps[BSP29_LUMP_TEXINFO];
	int num_texinfo = pos->filelen / sizeof(q1_texinfo_t);
	q1_texinfo_t *texinfos = stream.GetOffset<q1_texinfo_t>(pos->fileofs, num_texinfo);

	pos = &header->lumps[BSP29_LUMP_FACES];
	int num_faces = pos->filelen / sizeof(q1_dface_t);
	q1_dface_t *faces = stream.GetOffset<q1_dface_t>(pos->fileofs, num_faces);

	pos = &header->lumps[BSP29_LUMP_SURFEDGES];
	int num_surfedge = pos->filelen / sizeof(int);
	int *surfedges = stream.GetOffset<int>(pos->fileofs, num_surfedge);

	pos = &header->lumps[BSP29_LUMP_TEXTURES];
	stream.offset = pos->fileofs;
	int num_miptex = stream.GetType<int>();
	int *offset_list = stream.GetPtr<int>(num_miptex);

	q1_miptex_t *miptex_list[64]; // <-- num_miptex
	assert(num_miptex < 64);
	for (int i = 0; i < num_miptex; i++) {
		stream.offset = pos->fileofs + offset_list[i];
		q1_miptex_t *miptex = stream.GetPtr<q1_miptex_t>();

		extern qtexture_t *Texture_LoadQTexture(q1_miptex_t *qtex);
		qtexture_t *q = Texture_LoadQTexture(miptex);
		strncpy(q->name, miptex->name, sizeof(q->name) - 1);
		q->inuse = false;
		q->next = g_qeglobals.d_qtextures;
		g_qeglobals.d_qtextures = q;

		miptex_list[i] = miptex;
	}

	entitymodel *pBase = pModel;
	for (int i = 0; i < num_faces; i++) {
		q1_dface_t *face = &faces[i];
		q1_texinfo_t *texinfo = &texinfos[face->texinfo];
		q1_miptex_t *miptex = miptex_list[texinfo->miptex];

		qtexture_t *q = Texture_ForName(miptex->name);

		pModel->nSkinHeight = miptex->height;
		pModel->nSkinWidth = miptex->width;
		pModel->nTriCount = face->numedges - 2;
		pModel->nModelPosition = 0;
		pModel->nModelIndex = nModelIndex;
		pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * pModel->nTriCount);
		pModel->nTextureBind[pModel->nNumTextures++] = q->texture_number;

		float st[2];
		for (int j = 0; j < face->numedges; j++) {
			int ledge = surfedges[face->firstedge + j];
			int sedge = labs(ledge);

			q1_dedge_t *edge = &edges[sedge]; // remove sign
			q1_dvertex_t *vertex = &vertexes[ledge >= 0 ? edge->v[0] : edge->v[1]];

			vec3_t pos;
			// VectorScale(vertex->point, 4.0f / 3, pos);
			VectorCopy(vertex->point, pos);

			st[0] = (DotProduct(vertex->point, texinfo->vecs[0]) + texinfo->vecs[0][3]) / miptex->width;
			st[1] = (DotProduct(vertex->point, texinfo->vecs[1]) + texinfo->vecs[1][3]) / miptex->height;

			if (j <= 2) {
				VectorCopy(pos, pModel->pTriList[0].v[j]);
				VectorCopy2(st, pModel->pTriList[0].st[j]);
			} else {
				int index = j - 2;
				VectorCopy(pModel->pTriList[0].v[0], pModel->pTriList[index].v[0]);
				VectorCopy2(pModel->pTriList[0].st[0], pModel->pTriList[index].st[0]);

				VectorCopy(pModel->pTriList[index - 1].v[2], pModel->pTriList[index].v[1]);
				VectorCopy2(pModel->pTriList[index - 1].st[2], pModel->pTriList[index].st[1]);

				VectorCopy(pos, pModel->pTriList[index].v[2]);
				VectorCopy2(st, pModel->pTriList[index].st[2]);

			}
			ExtendBounds(vertex->point, vMin, vMax);
		}
		pModel->pNext = reinterpret_cast<entitymodel_t*>(qmalloc(sizeof(entitymodel_t)));
		pModel = pModel->pNext;
	}
#if 0
	if ((vMin[0] + vMin[1] + vMin[2]) == 0) {
		vec3_t vTransform;
		VectorScale(vMax, -0.5f, vTransform);
		ResetBound(vMin, vMax);
		while (pBase) {
			for (int i = 0; i < pBase->nTriCount; i++) {
				for (int j = 0; j < 3; j++) {
					VectorAdd(pBase->pTriList[i].v[j], vTransform, pBase->pTriList[i].v[j]);
					ExtendBounds(pBase->pTriList[i].v[j], vMin, vMax);
				}
			}
			pBase = pBase->pNext;
		}
	}
#endif
}

void Load_MDLFrame(aliashdr_t *header, stvert_t *verts, triangle_t *tris, trivertx_t *frame, entitymodel *&pModel, vec3_t &vMin, vec3_t &vMax) {
	for (int j = 0; j < header->numtris; j++) {
		for (int i = 0; i < 3; i++) {
			int vertex = tris[j].vertindex[i];
			float s = verts[vertex].s;
			float t = verts[vertex].t;
			// https://quake.by/tutorials/?id=64
			if (verts[vertex].onseam && !tris[j].facesfront) {
				s += header->skinwidth * 0.5f;
			}
			pModel->pTriList[j].st[i][0] = float(s + 0.5f) / header->skinwidth;
			pModel->pTriList[j].st[i][1] = float(t + 0.5f) / header->skinheight;

			vec3_t pos;
			for (int k = 0; k < 3; k++) {
				int unpack = frame[vertex].position[k];
				pos[k] = (header->scale[k] * unpack) + header->origin[k];
			}
			VectorCopy(pos, pModel->pTriList[j].v[i]);
			ExtendBounds(pos, vMin, vMax);
		}
	}
}

void Load_MDLModel(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, int nModelIndex) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.offset = 0;
	stream.size = 1;

	// https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_5.htm#MDS
	aliashdr_t *header = stream.GetPtr<aliashdr_t>(1);
	if (header->ident != ID_Q1_MDLHEADER || header->version == 0) {
		return;
	}

	pModel->nSkinHeight = header->skinheight;
	pModel->nSkinWidth = header->skinwidth;
	pModel->nTriCount = header->numtris;
	pModel->nModelPosition = 0;
	pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * header->numtris);
	pModel->nModelIndex = nModelIndex;

	for (int i = 0; i < header->numskins; i++) {
		const int skin_size = header->skinwidth * header->skinheight;
		int group = stream.GetType<int>();
		if (group == 0) {
			byte *skin = stream.GetPtr<byte>(skin_size);
			pModel->nTextureBind[pModel->nNumTextures++] = PROG_LoadSkin(skin, header->skinwidth, header->skinheight, false);
		} else {
			int num_skin = stream.GetType<int>();
			float *time = stream.GetPtr<float>(num_skin);
			for (int j = 0; j < num_skin; j++) {
				byte *skin = stream.GetPtr<byte>(skin_size);
				pModel->nTextureBind[pModel->nNumTextures++] = PROG_LoadSkin(skin, header->skinwidth, header->skinheight, false);
			}
		}
	}

	stvert_t *verts = stream.GetPtr<stvert_t>(header->numverts, false);
	triangle_t *tris = stream.GetPtr<triangle_t>(header->numtris, false);

	trivertx_t *load_frames = nullptr;
	for (int i = 0; i < header->numframes; i++) {
		long type = stream.GetType<long>();
		if (type == 0) {
			trivertx_t min = stream.GetType<trivertx_t>();
			trivertx_t max = stream.GetType<trivertx_t>();
			char *name = stream.GetPtr<char>(16);
			trivertx_t *frames = stream.GetPtr<trivertx_t>(header->numverts);
			// Sys_Printf("-- Anim Single: %s\n", name);
			if (!load_frames || strcmp(name, "stand1") == 0 || strcmp(name, "idle1") == 0) {
				load_frames = frames;
			}
		} else {
			int num_frames = stream.GetType<int>();
			trivertx_t bound_min = stream.GetType<trivertx_t>();
			trivertx_t bound_max = stream.GetType<trivertx_t>();
			float *time = stream.GetPtr<float>(num_frames);
			for (int j = 0; j < num_frames; j++) {
				trivertx_t min = stream.GetType<trivertx_t>();
				trivertx_t max = stream.GetType<trivertx_t>();
				char *name = stream.GetPtr<char>(16);
				trivertx_t *frames = stream.GetPtr<trivertx_t>(header->numverts);
				// Sys_Printf("-- Anim Group: %s\n", name);
				if (!load_frames || strcmp(name, "stand") == 0 || strcmp(name, "idle") == 0) {
					load_frames = frames;
				}
			}
		}
	}
	Load_MDLFrame(header, verts, tris, load_frames, pModel, vMin, vMax);
}



void Load_SPRModel_v1(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.offset = 0;
	stream.size = 1;

	q1_msprite_t *header = stream.GetPtr<q1_msprite_t>();
	if (header->ident != ID_Q1_SPRITEHEADER || header->version != 1) {
		return;
	}
	
	pModel->nSkinHeight = header->maxheight;
	pModel->nSkinWidth = header->maxwidth;
	pModel->nTriCount = 2;
	pModel->nModelPosition = 0;
	pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * pModel->nTriCount);
	pModel->nNumTextures = 0;
	pModel->bIsSprite = true;
	pModel->nSpriteType = header->type;
	e->nShowFlags |= ECLASS_SPRITE;

	for (int i = 0; i < header->numframes; i++) {
		int group = stream.GetType<int>();
		long nb = 1;
		if (group != 0) {
			nb = stream.GetType<int>();
			float *times = stream.GetPtr<float>(nb);
		}
		for (int j = 0; j < nb; j++) {
			q1_msprframe_t *frame = stream.GetPtr<q1_msprframe_t>();
			byte *pic = stream.GetPtr<byte>(frame->width * frame->height);
			pModel->nTextureBind[pModel->nNumTextures++] = PROG_LoadSkin(pic, frame->width, frame->height, true);
		}
	}
	vec3_t zero;
	VectorSet(zero, 0, 0, 0);
	Model_SpriteView(pModel, zero, zero, vMin, vMax);
}


void Load_SPRModel_v2(entitymodel *&pModel, byte *buffer, vec3_t &vMin, vec3_t &vMax, eclass_t *e) {
	readbuf_t stream;
	stream.buffer = buffer;
	stream.size = 0;
	stream.offset = 0;

	hl_msprite_t *header = stream.GetPtr<hl_msprite_t>();
	if (strncmp(header->id, "IDSP", 4) != 0) {
		return;
	}
	assert(header->version == 2);

	pModel->nSkinHeight = header->maxheight;
	pModel->nSkinWidth = header->maxwidth;
	pModel->nTriCount = 2;
	pModel->nModelPosition = 0;
	pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * pModel->nTriCount);
	pModel->nNumTextures = 0;
	pModel->bIsSprite = true;
	pModel->nSpriteType = header->type;
	if (pModel->bIsEditor) {
		pModel->nSkinHeight = 12;
		pModel->nSkinWidth = 12;
		pModel->nSpriteType = 2;
	}
	e->nShowFlags |= ECLASS_SPRITE;

	typedef enum {
		SPR_NORMAL = 0,
		SPR_ADDITIVE,
		SPR_INDEXALPHA,
		SPR_ALPHTEST,
	} drawtype_t;

	unsigned short pal_size = stream.GetType<short>();
	byte *pal = stream.GetPtr<byte>(pal_size * 3);
	byte *out = (byte *)qmalloc(header->maxwidth * header->maxheight * 4);
	for (int i = 0; i < header->numframes; i++) {
		hl_mspriteframe_t *frame = stream.GetPtr<hl_mspriteframe_t>();

		int size = frame->width * frame->height;
		byte *src = stream.GetPtr<byte>(size);

		for (int j = 0; j < size; j++) {
			int index = src[j]; // 255 is transparent
			if (index == 255 && header->tex_format == SPR_ALPHTEST) {
				out[j * 4 + 0] = 0;
				out[j * 4 + 1] = 0;
				out[j * 4 + 2] = 0;
				out[j * 4 + 3] = 0;
			} else {
				out[j * 4 + 0] = pal[index * 3 + 0];
				out[j * 4 + 1] = pal[index * 3 + 1];
				out[j * 4 + 2] = pal[index * 3 + 2];
				out[j * 4 + 3] = 255;
			}
		}
		pModel->nTextureBind[pModel->nNumTextures++] = SPR_LoadFrame(out, frame->width, frame->height);
	}
	free(out);

	vec3_t zero;
	VectorSet(zero, 0, 0, 0);
	Model_SpriteView(pModel, zero, zero, vMin, vMax);
}



void Model_BeamView(entitymodel *&pModel, vec3_t start, vec3_t end, int scale, int scroll, vec3_t &vMin, vec3_t &vMax) {
	const camera_t &camera = g_pParentWnd->GetCamera()->Camera();
	vec3_t dir, vforward, vright, vup, pos[4], st[4];

	VectorSubtract(camera.origin, start, vforward);
	VectorNormalize(vforward);

	VectorSubtract(end, start, dir);
	float len = VectorNormalize(dir);
	VectorClear(start);
	VectorMA(start, len, dir, end);


	CrossProduct(dir, vforward, vup);

	VectorMA(start, scale, vup, pos[0]);
	VectorMA(end, scale, vup, pos[1]);
	VectorMA(end, -scale, vup, pos[2]);
	VectorMA(start, -scale, vup, pos[3]);
	VectorSet(st[0], 1.0f, 0.0f + scroll, 0.0f);
	VectorSet(st[1], 1.0f, 1.0f + scroll, 0.0f);
	VectorSet(st[2], 0.0f, 1.0f + scroll, 0.0f);
	VectorSet(st[3], 0.0f, 0.0f + scroll, 0.0f);


	VectorCopy(pos[3], pModel->pTriList[0].v[0]);
	VectorCopy(pos[2], pModel->pTriList[0].v[1]);
	VectorCopy(pos[1], pModel->pTriList[0].v[2]);
	VectorCopy(pos[3], pModel->pTriList[1].v[0]);
	VectorCopy(pos[1], pModel->pTriList[1].v[1]);
	VectorCopy(pos[0], pModel->pTriList[1].v[2]);
	for (int i = 0; i < 2; i++) {
		pModel->pTriList[0].st[0][i] = st[3][i];
		pModel->pTriList[0].st[1][i] = st[2][i];
		pModel->pTriList[0].st[2][i] = st[1][i];
		pModel->pTriList[1].st[0][i] = st[3][i];
		pModel->pTriList[1].st[1][i] = st[1][i];
		pModel->pTriList[1].st[2][i] = st[0][i];
	}
	for (int i = 0; i < 4; i++) {
		ExtendBounds(pos[i], vMin, vMax);
	}
}

void Model_SpriteView(entitymodel *&pModel, vec3_t angles, vec3_t origin, vec3_t &vMin, vec3_t &vMax) {
	vec3_t up, forward, right, pos[4], st[4];

	const camera_t &camera = g_pParentWnd->GetCamera()->Camera();
	if (pModel->nSpriteType == SPR_ORIENTED) {
		AngleVectors(angles, forward, right, up);
	} else if (pModel->nSpriteType == SPR_FACING_UPRIGHT) {
		VectorCopy(camera.vpn, forward);
		VectorSet(right, origin[1] - forward[1], -(origin[0] - forward[0]), 0.0f);
		VectorSet(up, 0, 0, 1);
		VectorNormalize(right);
	} else if (pModel->nSpriteType == SPR_FWD_PARALLEL_UPRIGHT) {
		VectorCopy(camera.vpn, forward);
		if (fabs(forward[0]) > 0.999f) {
			return;
		}
		VectorSet(right, forward[1], -forward[0], 0.0f);
		VectorSet(up, 0, 0, 1);
		VectorNormalize(right);
	} else if (pModel->nSpriteType == SPR_FWD_PARALLEL_ORIENTED) {
		float sr, cr, angle = angles[ROLL] * (Q_PI / 180.0f);
		sr = sinf(angle), cr = cosf(angle);
		for (int i = 0; i < 3; i++) {
			right[i] = camera.vright[i] * cr + camera.vup[i] * sr;
			up[i] = camera.vright[i] * -sr + camera.vup[i] * cr;
		}
	} else {
		VectorCopy(camera.vright, right);
		VectorCopy(camera.vup, up);
	}
	int width = pModel->nSkinWidth;
	int height = pModel->nSkinHeight;

	memset(pos, 0x0, sizeof(pos));
	memset(st, 0x0, sizeof(st));

	for (int i = 0; i < 3; i++) {
		pos[0][i] = (right[i] * width *  0.5f) + (up[i] * height *  0.5f);
		pos[1][i] = (right[i] * width *  0.5f) + (up[i] * height * -0.5f);
		pos[2][i] = (right[i] * width * -0.5f) + (up[i] * height * -0.5f);
		pos[3][i] = (right[i] * width * -0.5f) + (up[i] * height *  0.5f);
		if (i == 0) {
			st[3][i] = -1.0f;
			st[2][i] = -1.0f;
		} else if (i == 1) {
			st[3][i] = -1.0f;
			st[0][i] = -1.0f;
		}
	}

	VectorCopy(pos[3], pModel->pTriList[0].v[0]);
	VectorCopy(pos[2], pModel->pTriList[0].v[1]);
	VectorCopy(pos[1], pModel->pTriList[0].v[2]);
	VectorCopy(pos[3], pModel->pTriList[1].v[0]);
	VectorCopy(pos[1], pModel->pTriList[1].v[1]);
	VectorCopy(pos[0], pModel->pTriList[1].v[2]);
	for (int i = 0; i < 2; i++) {
		pModel->pTriList[0].st[0][i] = st[3][i];
		pModel->pTriList[0].st[1][i] = st[2][i];
		pModel->pTriList[0].st[2][i] = st[1][i];
		pModel->pTriList[1].st[0][i] = st[3][i];
		pModel->pTriList[1].st[1][i] = st[1][i];
		pModel->pTriList[1].st[2][i] = st[0][i];
	}
	for (int i = 0; i < 4; i++) {
		ExtendBounds(pos[i], vMin, vMax);
	}
}

void Load_DecalModel(entitymodel *&pModel, qtexture_t *qtex, vec3_t &vMin, vec3_t &vMax, eclass_t *e) {
	pModel->nSkinHeight = qtex->height;
	pModel->nSkinWidth = qtex->width;
	pModel->nTriCount = 2;
	pModel->nModelPosition = 0;
	pModel->pTriList = (trimodel *)qmalloc(sizeof(trimodel) * pModel->nTriCount);
	pModel->nNumTextures = 0;
	pModel->bIsDecal = true;
	pModel->nTextureBind[pModel->nNumTextures++] = qtex->texture_number;
	e->nShowFlags |= ECLASS_DECAL;

	vec3_t normal;
	VectorSet(normal, 0, 1, 0);
	Model_DecalView(pModel, normal, vMin, vMax);
}

void Model_DecalView(entitymodel *&pModel, vec3_t normal, vec3_t &vMin, vec3_t &vMax) {
	vec3_t forward, right, up;

	VectorSet(up, 0, 0, 1);
	VectorCopy(normal, forward);
	if (VectorCompare(forward, up)) {
		VectorSet(up, 0, 1, 0);
	}
	CrossProduct(forward, up, right);

	vec3_t pos[4], st[4] = { 0 };
	for (int i = 0; i < 3; i++) {
		pos[0][i] = (right[i] * pModel->nSkinWidth *  0.5f) + (up[i] * pModel->nSkinHeight *  0.5f) + (forward[i] * 0.01f);
		pos[1][i] = (right[i] * pModel->nSkinWidth *  0.5f) + (up[i] * pModel->nSkinHeight * -0.5f) + (forward[i] * 0.01f);
		pos[2][i] = (right[i] * pModel->nSkinWidth * -0.5f) + (up[i] * pModel->nSkinHeight * -0.5f) + (forward[i] * 0.01f);
		pos[3][i] = (right[i] * pModel->nSkinWidth * -0.5f) + (up[i] * pModel->nSkinHeight *  0.5f) + (forward[i] * 0.01f);
		if (i == 0) {
			st[3][i] = 1.0f;
			st[2][i] = 1.0f;
		} else if (i == 1) {
			st[3][i] = -1.0f;
			st[0][i] = -1.0f;
		}
	}
	VectorCopy(pos[3], pModel->pTriList[0].v[0]);
	VectorCopy(pos[2], pModel->pTriList[0].v[1]);
	VectorCopy(pos[1], pModel->pTriList[0].v[2]);
	VectorCopy(pos[3], pModel->pTriList[1].v[0]);
	VectorCopy(pos[1], pModel->pTriList[1].v[1]);
	VectorCopy(pos[0], pModel->pTriList[1].v[2]);
	for (int i = 0; i < 2; i++) {
		pModel->pTriList[0].st[0][i] = st[3][i];
		pModel->pTriList[0].st[1][i] = st[2][i];
		pModel->pTriList[0].st[2][i] = st[1][i];
		pModel->pTriList[1].st[0][i] = st[3][i];
		pModel->pTriList[1].st[1][i] = st[1][i];
		pModel->pTriList[1].st[2][i] = st[0][i];
	}
	for (int i = 0; i < 4; i++) {
		ExtendBounds(pos[i], vMin, vMax);
	}
}