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
#include "PrefsDlg.h"
#include <direct.h>  
#include <sys\stat.h> 

QEGlobals_t  g_qeglobals;

void WINAPI QE_CheckOpenGLForErrors(void) {
	CString strMsg;
	int i = qglGetError();
	if (i != GL_NO_ERROR) {
		if (i == GL_OUT_OF_MEMORY) {
			//strMsg.Format("OpenGL out of memory error %s\nDo you wish to save before exiting?", qgluErrorString((GLenum)i));
			if (MessageBox(g_qeglobals.d_hwndMain, strMsg, "Q3Radiant Error", MB_YESNO) == IDYES) {
				Map_SaveFile(NULL, false);
			}
			exit(1);
		} else {
			// strMsg.Format("Warning: OpenGL Error %s\n ", qgluErrorString((GLenum)i));
			strMsg.Format("Warning: OpenGL Error: 0x%04x\n", i);
			Sys_Printf(strMsg.GetBuffer(0));
		}
	}
}


char *ExpandReletivePath(char *p) {
	static char	temp[1024];
	char	*base;

	if (!p || !p[0])
		return NULL;
	if (p[0] == '/' || p[0] == '\\')
		return p;

	base = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	sprintf(temp, "%s/%s", base, p);
	return temp;
}

char *copystring(char *s) {
	char	*b;
	b = (char*)malloc(strlen(s) + 1);
	strcpy(b, s);
	return b;
}


bool DoesFileExist(const char* pBuff, long& lSize) {
	CFile file;
	if (file.Open(pBuff, CFile::modeRead | CFile::shareDenyNone)) {
		lSize += file.GetLength();
		file.Close();
		return true;
	}
	return false;
}


void Map_Snapshot() {
	CString strMsg;
	// we need to do the following
	// 1. make sure the snapshot directory exists (create it if it doesn't)
	// 2. find out what the lastest save is based on number
	// 3. inc that and save the map
	CString strOrgPath, strOrgFile;
	ExtractPath_and_Filename(currentmap, strOrgPath, strOrgFile);
	AddSlash(strOrgPath);
	strOrgPath += "snapshots";
	bool bGo = true;
	struct _stat Stat;
	if (_stat(strOrgPath, &Stat) == -1) {
		bGo = (_mkdir(strOrgPath) != -1);
	}
	AddSlash(strOrgPath);
	if (bGo) {
		int nCount = 0;
		long lSize = 0;
		CString strNewPath = strOrgPath;
		strNewPath += strOrgFile;
		CString strFile;
		while (bGo) {
			strFile.Format("%s.%i", strNewPath, nCount);
			bGo = DoesFileExist(strFile, lSize);
			nCount++;
		}
		// strFile has the next available slot
		Map_SaveFile(strFile.GetBuffer(0), false);
		Sys_SetTitle(currentmap);
		if (lSize > 12 * 1024 * 1024) // total size of saves > 4 mb
		{
			Sys_Printf("The snapshot files in the [%s] directory total more than 4 megabytes. You might consider cleaning the directory up.\n", strOrgPath);
		}
	} else {
		strMsg.Format("Snapshot save failed.. unabled to create directory\n%s\n", strOrgPath);
		g_pParentWnd->MessageBox(strMsg);
	}
}
/*
===============
QE_CheckAutoSave

If five minutes have passed since making a change
and the map hasn't been saved, save it out.
===============
*/


void QE_CheckAutoSave(void) {
	static clock_t s_start;
	clock_t        now;
	char autosave[1024];
	now = clock();

	sprintf(autosave, "%s/maps/autosave.map", ValueForKey(g_qeglobals.d_project_entity, "basepath"));

	if (modified != 1 || !s_start) {
		s_start = now;
		return;
	}

	if (now - s_start > (CLOCKS_PER_SEC * 60 * g_PrefsDlg.m_nAutoSave)) {

		if (g_PrefsDlg.m_bAutoSave) {
			CString strMsg = g_PrefsDlg.m_bSnapShots ? "Autosaving snapshot..." : "Autosaving...";
			Sys_Printf(strMsg.GetBuffer(0));
			Sys_Printf("\n");
			Sys_Status(strMsg.GetBuffer(0), 0);

			// only snapshot if not working on a default map
			if (g_PrefsDlg.m_bSnapShots && stricmp(currentmap, "unnamed.map") != 0) {
				Map_Snapshot();
			} else {
				Map_SaveFile(autosave, false);
			}

			Sys_Status("Autosaving...Saved.", 0);
			modified = 2;
		} else {
			Sys_Printf("Autosave skipped...\n");
			Sys_Status("Autosave skipped...", 0);
		}
		s_start = now;
	}
}


int BuildShortPathName(const char* pPath, char* pBuffer, int nBufferLen) {
	char *pFile = NULL;
	int nResult = GetFullPathName(pPath, nBufferLen, pBuffer, &pFile);
	nResult = GetShortPathName(pPath, pBuffer, nBufferLen);
	if (nResult == 0)
		strcpy(pBuffer, pPath);                     // Use long filename
	return nResult;
}



const char *g_pPathFixups[] =
{
	"basepath",
	"remotebasepath",
	"entitypath",
	// "texturepath",
	// "autosave",
	// "mapspath"
};

const int g_nPathFixupCount = sizeof(g_pPathFixups) / sizeof(const char*);

/*
===========
QE_LoadProject
===========
*/
qboolean QE_LoadProject(char *projectfile) {
	char	*data;

	Sys_Printf("QE_LoadProject (%s)\n", projectfile);

	if (LoadFileNoCrash(projectfile, (void **)&data) == -1)
		return false;

	g_strProject = projectfile;

	CString strData = data;
	free(data);

	CString strQ2Path = g_PrefsDlg.m_strQuake2;
	CString strQ2File;
	ExtractPath_and_Filename(g_PrefsDlg.m_strQuake2, strQ2Path, strQ2File);
	AddSlash(strQ2Path);


	
	char* pBuff = new char[1024];
	/*
	BuildShortPathName(strQ2Path, pBuff, 1024);
	FindReplace(strData, "__Q2PATH", pBuff);
	BuildShortPathName(g_strAppPath, pBuff, 1024);
	FindReplace(strData, "__QERPATH", pBuff);

	char* pFile;
	if (GetFullPathName(projectfile, 1024, pBuff, &pFile)) {
		g_PrefsDlg.m_strLastProject = pBuff;
		BuildShortPathName(g_PrefsDlg.m_strLastProject, pBuff, 1024);
		g_PrefsDlg.m_strLastProject = pBuff;
		g_PrefsDlg.SavePrefs();

		ExtractPath_and_Filename(pBuff, strQ2Path, strQ2File);
		int nLen = strQ2Path.GetLength();
		if (nLen > 0) {
			if (strQ2Path[nLen - 1] == '\\')
				strQ2Path.SetAt(nLen - 1, '\0');
			char *pBuffer = strQ2Path.GetBufferSetLength(_MAX_PATH + 1);
			int n = strQ2Path.ReverseFind('\\');
			if (n >= 0)
				pBuffer[n + 1] = '\0';
			strQ2Path.ReleaseBuffer();
			FindReplace(strData, "__QEPROJPATH", strQ2Path);
		}
	}*/

	QE_ProjectList(strData);
	if (!g_qeglobals.d_project_entity) {
		Error("Couldn't parse %s", projectfile);
	}

	/*
	for (int i = 0; i < g_nPathFixupCount; i++) {
		char *pPath = ValueForKey(g_qeglobals.d_project_entity, g_pPathFixups[i]);
		if (pPath[0] != '\\' && pPath[0] != '/') {
			if (GetFullPathName(pPath, 1024, pBuff, &pFile)) {
				SetKeyValue(g_qeglobals.d_project_entity, g_pPathFixups[i], pBuff);
			}
		}
	}
	delete[] pBuff;
	*/

	// set here some default project settings you need
	if (strlen(ValueForKey(g_qeglobals.d_project_entity, "brush_primit")) == 0) {
		SetKeyValue(g_qeglobals.d_project_entity, "brush_primit", "0");
	}
	g_PrefsDlg.m_bHiColorTextures = IsGame(GAME_ID3) || IsGame(GAME_KINGPIN);
	g_qeglobals.m_bBrushPrimitMode = IntForKey(g_qeglobals.d_project_entity, "brush_primit");


	Eclass_InitForSourceDirectory(ValueForKey(g_qeglobals.d_project_entity, "entitypath"));
	FillClassList();		// list in entity window

	Map_New();


	FillTextureMenu();
	FillBSPMenu();

	return true;
}

/*
===========
QE_SaveProject
===========
*/
//extern char	*bsp_commands[256];

qboolean QE_SaveProject(const char* pProjectFile) {
	//char	filename[1024];
	FILE	*fp;
	epair_t	*ep;

	//sprintf (filename, "%s\\%s.prj", g_projectdir, g_username);

	if (!(fp = fopen(pProjectFile, "w+")))
		Error("Could not open project file!");

	fprintf(fp, "{\n");
	for (ep = g_qeglobals.d_project_entity->epairs; ep; ep = ep->next)
		fprintf(fp, "\"%s\" \"%s\"\n", ep->key, ep->value);
	fprintf(fp, "}\n");

	fclose(fp);

	return TRUE;
}




void ConnectToMultiManager(const CString &target, entity_t *e1) {
	float nextValue = 1.0f;
	for (int i = 0; i < 8; i++) {
		CString strTemp = target, strValue;
		if (i != 0) {
			strTemp.Format("%s#%d", target, i);
		}
		char *pValue = ValueForKey(e1, strTemp);
		if (pValue[0] != 0) {
			float value = atof(pValue);
			nextValue = Q_MAX(nextValue, value);
			continue;
		}
		strValue.Format("%.2f", nextValue + 0.1f);
		SetKeyValue(e1, strTemp, strValue);
		break;
	}
}

/*
===========
QE_KeyDown
===========
*/
#define	SPEED_MOVE	32
#define	SPEED_TURN	22.5


/*
===============
ConnectEntities

Sets target / targetname on the two entities selected
from the first selected to the secon
===============
*/
void ConnectEntities(void) {
	entity_t	*e1, *e2, *e;
	CString target;
	
	int prefix_size = 0;

	if (g_qeglobals.d_select_count != 2) {
		Sys_Status("Must have two brushes selected.", 0);
		Sys_Beep();
		return;
	}

	e1 = g_qeglobals.d_select_order[0]->owner;
	e2 = g_qeglobals.d_select_order[1]->owner;

	if (e1 == world_entity || e2 == world_entity) {
		Sys_Status("Can't connect to the world.", 0);
		Sys_Beep();
		return;
	}

	if (e1 == e2) {
		Sys_Status("Brushes are from same entity.", 0);
		Sys_Beep();
		return;
	}
	if (!(IsGame(GAME_HL) && e1->eclass->nShowFlags & ECLASS_MULTI_MANAGER)) {
		target = ValueForKey(e1, "target");
	}
	if (target.IsEmpty()) {
		target = ValueForKey(e2, "targetname");
		if (target.IsEmpty()) {
			// make a unique target value
			MakeEntityTargetName(e2, target);
		}
	}
	if (IsGame(GAME_HL) && e1->eclass->nShowFlags & ECLASS_MULTI_MANAGER) {
		ConnectToMultiManager(target, e1);
	} else {
		SetKeyValue(e1, "target", target);
	}
	SetKeyValue(e2, "targetname", target);
	Sys_UpdateWindows(W_XY | W_CAMERA);

	Select_Deselect();
	Select_Brush(g_qeglobals.d_select_order[1]);
}

qboolean QE_SingleBrush(bool bQuiet) {
	if ((selected_brushes.next == &selected_brushes)
		|| (selected_brushes.next->next != &selected_brushes)) {
		if (!bQuiet) {
			Sys_Printf("Error: you must have a single brush selected\n");
		}
		return false;
	}
	if (selected_brushes.next->owner->eclass->fixedsize) {
		if (!bQuiet) {
			Sys_Printf("Error: you cannot manipulate fixed size entities\n");
		}
		return false;
	}

	return true;
}

void QE_Init(void) {
	/*
	** initialize variables
	*/
	g_qeglobals.d_gridsize = 8;
	g_qeglobals.d_showgrid = true;

	/*
	** other stuff
	*/
	Texture_Init(true);
	//Cam_Init ();
	//XY_Init ();
	Z_Init();
	Terrain_Init();
}

void WINAPI QE_ConvertDOSToUnixName(char *dst, const char *src) {
	while (*src) {
		char c = tolower(*src);
		if (c == '\\')
			*dst = '/';
		else
			*dst = c;
		dst++; src++;
	}
	*dst = 0;
}

int g_numbrushes, g_numentities;

void QE_CountBrushesAndUpdateStatusBar(void) {
	static int      s_lastbrushcount, s_lastentitycount;
	static qboolean s_didonce;

	//entity_t   *e;
	brush_t	   *b, *next;

	g_numbrushes = 0;
	g_numentities = 0;

	if (active_brushes.next != NULL) {
		for (b = active_brushes.next; b != NULL && b != &active_brushes; b = next) {
			next = b->next;
			if (b->brush_faces) {
				if (!b->owner->eclass->fixedsize)
					g_numbrushes++;
				else
					g_numentities++;
			}
		}
	}
	/*
		if ( entities.next != NULL )
		{
		for ( e = entities.next ; e != &entities && g_numentities != MAX_MAP_ENTITIES ; e = e->next)
		{
		g_numentities++;
		}
		}
		*/
	if (((g_numbrushes != s_lastbrushcount) || (g_numentities != s_lastentitycount)) || (!s_didonce)) {
		Sys_UpdateStatusBar();

		s_lastbrushcount = g_numbrushes;
		s_lastentitycount = g_numentities;
		s_didonce = true;
	}
}

char		com_token[1024];
qboolean	com_eof;

/*
================
I_FloatTime
================
*/
double I_FloatTime(void) {
	time_t	t;

	time(&t);

	return t;
#if 0
	// more precise, less portable
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
#endif
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse(char *data) {
	int		c;
	int		len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ') {
		if (c == 0) {
			com_eof = true;
			return NULL;			// end of file;
		}
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/') {
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"') {
		data++;
		do {
			c = *data++;
			if (c == '\"') {
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		} while (1);
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':') {
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do {
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':')
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}



/*
=============================================================================

MISC FUNCTIONS

=============================================================================
*/


int		argc;
char	*argv[MAX_NUM_ARGVS];

/*
============
ParseCommandLine
============
*/
void ParseCommandLine(char *lpCmdLine) {
	argc = 1;
	argv[0] = "programname";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS)) {
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine) {
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine) {
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}
}



/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm(char *check) {
	int             i;

	for (i = 1; i < argc; i++) {
		if (stricmp(check, argv[i]))
			return i;
	}

	return 0;
}




/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex(char *hex) {
	char    *str;
	int    num;

	num = 0;
	str = hex;

	while (*str) {
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str - '0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str - 'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str - 'A';
		else
			Error("Bad hex number: %s", hex);
		str++;
	}

	return num;
}


int ParseNum(char *str) {
	if (str[0] == '$')
		return ParseHex(str + 1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex(str + 2);
	return atol(str);
}




char *CopyString(char *token) {
	char *str = (char *)qmalloc(strlen(token) + 1);
	strcpy(str, com_token);
	return str;
}


char *Lex_ReadToken(char *&script_p, bool rest_on_line, bool copy_name, bool is_file) {
	int c, len = 0;
	com_token[0] = '\0';
	const char *mark = script_p; // check
	if (!script_p)
		return nullptr;

	// skip whitespace
skipwhite:
	while ((c = *script_p) <= ' ') {
		if (rest_on_line && c == '\n') {
			return nullptr;
		}
		if (c == 0) {
			com_eof = true;
			return nullptr;			// end of file;
		}
		script_p++;
	}

	// skip // comments
	if (c == '/' && script_p[1] == '/') {
		while (*script_p && *script_p != '\n')
			script_p++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"') {
		script_p++;
		do {
			c = *script_p++;
			if (c == '\"') {
				com_token[len] = 0;
				if (copy_name) {
					return CopyString(com_token);
				}
				return com_token;
			}
			com_token[len++] = c;
		} while (c != 0);
		return nullptr;
	}
	// parse numbers
	if ( isdigit(c) || (c == '.' && isdigit(script_p[1])) ) {
		if (c == '.') {
			com_token[len++] = '0';
		}
		do {
			com_token[len++] = c;
			script_p++;
			c = *script_p;
			if (c == 0) {
				return nullptr;
			}
		} while (isdigit(c) || c == '.');
		if (copy_name) {
			return CopyString(com_token);
		}
		com_token[len] = 0;
		return com_token;
	}


	// parse a regular word
	if (isalpha(c) || c == '_') {
		do {
			com_token[len++] = c;
			script_p++;
			c = *script_p;
			if (c == 0) {
				return nullptr;
			}
		} while (isalnum(c) || c == '_' || ( is_file && (c == '/' || c == '.') ));
		com_token[len] = 0;
		if (copy_name) {
			return CopyString(com_token);
		}
		return com_token;
	}

	// parse single characters
	if (ispunct(c)) {
		com_token[0] = c;
		com_token[1] = 0;
		script_p++;
		return com_token;
	}
	Sys_Printf("Parse Error: %32s\n", mark);
	return com_token;
}



bool Lex_ExpectToken(char *&src, const char *expect, bool rest_on_line) {
	char *token = Lex_ReadToken(src, rest_on_line, false);
	if (!token) {
		return false;
	}
	return strcmp(token, expect) == 0;
}

bool Lex_CheckToken(char *&src, const char *expect, bool rest_on_line) {
	char *mark = src;
	char *token = Lex_ReadToken(src, rest_on_line, false);
	if (!token) {
		return false;
	}
	if (strcmp(token, expect) == 0) {
		return true;
	}
	src = mark;
	return false;
}

bool Lex_PeekToken(char *&src, const char *expect, bool rest_on_line) {
	char *mark = src;
	char *token = Lex_ReadToken(src, rest_on_line, false);
	if (!token) {
		return false;
	}
	src = mark;
	if (strcmp(token, expect) == 0) {
		return true;
	}
	return false;
}

float Lex_FloatValue(char *&src, bool rest_on_line) {
	char line[64] = { 0 };

	if (Lex_CheckToken(src, "-", true)) {
		strcat(line, "-");
	}
	char *token = Lex_ReadToken(src, rest_on_line, false);
	strcat(line, token);
	return atof(line);
}

int Lex_IntValue(char *&src, bool rest_on_line) {
	char *token = Lex_ReadToken(src, rest_on_line, false);
	if (!token) {
		return false;
	}
	int sign = 1;
	if (strcmp(token, "-") == 0) {
		sign = -1, token = Lex_ReadToken(src, rest_on_line, false);
	}
	return sign * atol(token);
}

bool Lex_ParseVector(char *&src, vec3_t vec) {
	if (!Lex_ExpectToken(src, "(", true)) {
		return false;
	}
	vec[0] = Lex_FloatValue(src, true);
	vec[1] = Lex_FloatValue(src, true);
	vec[2] = Lex_FloatValue(src, true);
	return Lex_ExpectToken(src, ")", true);
}

bool Lex_NextLine(char *&src) {
	while (*src) {
		char ch = *src++;
		if (ch == '\n') {
			return true;
		}
	}
	return false;
}

bool Lex_SkipToken(char *&src, const char*match) {
	while (*src) {
		char *token = Lex_ReadToken(src);
		if (!token) {
			break;
		}
		if (strcmp(token, match) == 0) {
			return true;
		}
	}
	return false;
}




struct game_t g_lstGames[] = {
	{ GAME_Q1, "Quake1", nullptr },
	{ GAME_Q2, "Quake2", nullptr },
	{ GAME_Q3, "Quake3", nullptr },
	{ GAME_WOLF, "Wolfenstein", nullptr },
	{ GAME_HEXEN2, "Hexen2", nullptr },
	{ GAME_HL, "HalfLife", nullptr },
	{ GAME_ET, "ET", nullptr },
	{ GAME_KINGPIN, "Kingpin", nullptr },
	{ GAME_EF, "EliteForce", nullptr },
	{ GAME_JK2, "JK2", nullptr },
	{ GAME_SOF2, "SOF2", nullptr },
	{ GAME_MOHAA, "MOHAA", nullptr },
	{ GAME_COD, "COD", nullptr },
	{ 0, nullptr, nullptr }
};

int g_activeGame = GAME_Q3;

game_t *FindGameId(const CString &name) {
	for (int i = 0; g_lstGames[i].name; i++) {
		if (name == g_lstGames[i].name) {
			return &g_lstGames[i];
		}
	}
	return nullptr;
}

bool IsGame(int flags) {
	assert(g_activeGame != -1);
	return (flags & g_activeGame) != 0;
}

void QE_ProjectList(CString &strData) {
	StartTokenParsing(strData.GetBuffer(0));
	
	g_qeglobals.d_project_entity = nullptr;
	while (1) {
		entity_t *ent = Entity_Parse(true);
		if (!ent) {
			break;
		}
		char *name = ValueForKey(ent, "game");
		game_t *game = FindGameId(name);
		if (game) {
			game->entity = ent;
			continue;
		}
	}
	game_t *game = nullptr;
	do {
		CString name = g_PrefsDlg.m_strWhatGame;
		game = FindGameId(name);
		if (game->entity) {
			break;
		}
		if (g_PrefsDlg.DoModal() == IDCANCEL) {
			Error("Cancelled by the user.!!!");
		}
	} while (true);
	g_qeglobals.d_project_entity = game->entity;
	g_activeGame = game->id;
}