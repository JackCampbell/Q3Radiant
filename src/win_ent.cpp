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
#include "entityw.h"
#include "TexWnd.h"
#include "WaveOpen.h"

int rgIds[EntLast] = {
	IDC_E_LIST,
	IDC_E_COMMENT,
	IDC_CHECK1,
	IDC_CHECK2,
	IDC_CHECK3,
	IDC_CHECK4,
	IDC_CHECK5,
	IDC_CHECK6,
	IDC_CHECK7,
	IDC_CHECK8,
	IDC_CHECK9,
	IDC_CHECK10,
	IDC_CHECK11,
	IDC_CHECK12,
	IDC_E_PROPS,
	IDC_E_0,
	IDC_E_45,
	IDC_E_90,
	IDC_E_135,
	IDC_E_180,
	IDC_E_225,
	IDC_E_270,
	IDC_E_315,
	IDC_E_UP,
	IDC_E_DOWN,
	IDC_E_DELPROP,

	IDC_STATIC_KEY,
	IDC_E_KEY_FIELD,
	IDC_STATIC_VALUE,
	IDC_E_VALUE_FIELD,

	IDC_E_COLOR,

	IDC_BTN_ASSIGNSOUND,
	IDC_BTN_ASSIGNMODEL,
	IDC_E_CREATE,
	IDC_ANIM_LIST
};

HWND hwndEnt[EntLast];
CTabCtrl g_wndTabs;

int		inspector_mode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE

qboolean	multiple_entities;

entity_t	*edit_entity;


BOOL CALLBACK EntityWndProc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam);	// second message parameter

void SizeEntityDlg(int iWidth, int iHeight);
void AddProp();
void GetTexMods(void);


LRESULT(CALLBACK* OldFieldWindowProc) (HWND, UINT, WPARAM, LPARAM);
LRESULT(CALLBACK* OldEntityListWindowProc) (HWND, UINT, WPARAM, LPARAM);

/*
=========================
FieldWndProc

Just to handle tab and enter...
=========================
*/
BOOL CALLBACK FieldWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam) {
	switch (uMsg) {
	case WM_CHAR:
		if (LOWORD(wParam) == VK_TAB)
			return FALSE;
		if (LOWORD(wParam) == VK_RETURN)
			return FALSE;
		if (LOWORD(wParam) == VK_ESCAPE) {
			SetFocus(g_qeglobals.d_hwndCamera);
			return FALSE;
		}
		break;

	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_TAB) {
			if (hwnd == hwndEnt[EntKeyField]) {
				// SendMessage (hwndEnt[EntValueField], WM_SETTEXT, 0, (long)"");
				SetFocus(hwndEnt[EntValueField]);
			} else
				SetFocus(hwndEnt[EntKeyField]);
		}
		if (LOWORD(wParam) == VK_RETURN) {
			if (hwnd == hwndEnt[EntKeyField]) {
				SendMessage(hwndEnt[EntValueField], WM_SETTEXT, 0, (long)"");
				SetFocus(hwndEnt[EntValueField]);
			} else {
				AddProp();
				SetFocus(g_qeglobals.d_hwndCamera);
			}
		}
		break;
		//	case WM_NCHITTEST:
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		break;
	}
	return CallWindowProc(OldFieldWindowProc, hwnd, uMsg, wParam, lParam);
}


/*
=========================
EntityListWndProc

Just to handle enter...
=========================
*/
BOOL CALLBACK EntityListWndProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam) {
	switch (uMsg) {
	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_RETURN) {
			SendMessage(g_qeglobals.d_hwndEntity, WM_COMMAND, (CBN_DBLCLK << 16) + IDC_E_LIST, 0); // !!! dont work!!!
			return 0;
		}
		break;
	}
	return CallWindowProc(OldEntityListWindowProc, hwnd, uMsg, wParam, lParam);
}


/*
================
GetEntityControls

Finds the controls from the dialog and
moves them to the window
================
*/
void GetEntityControls(HWND ghwndEntity) {
	int i;

	for (i = 0; i < EntLast; i++) {
		if (i == EntList || i == EntProps || i == EntComment || i == EntAnimList)
			continue;
		if (i == EntKeyField || i == EntValueField)
			continue;
		hwndEnt[i] = GetDlgItem(ghwndEntity, rgIds[i]);
		if (hwndEnt[i]) {
			SetParent(hwndEnt[i], g_qeglobals.d_hwndEntity);
			SendMessage(hwndEnt[i], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
		}
	}


	// SetParent apears to not modify some internal state
	// on listboxes, so create it from scratch...

	hwndEnt[EntList] = CreateWindow("combobox", NULL,
		// LBS_STANDARD | LBS_WANTKEYBOARDINPUT | LBS_NOINTEGRALHEIGHT
		CBS_SORT | CBS_DROPDOWN | CBS_NOINTEGRALHEIGHT
		| WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		5, 5, 180, 99,
		g_qeglobals.d_hwndEntity,
		(HMENU)IDC_E_LIST,
		g_qeglobals.d_hInstance,
		NULL);
	if (!hwndEnt[EntList])
		Error("CreateWindow failed");

	hwndEnt[EntAnimList] = CreateWindow("combobox", NULL,
		CBS_SORT | CBS_DROPDOWN | CBS_NOINTEGRALHEIGHT
		| WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		5, 5, 180, 99, g_qeglobals.d_hwndEntity,
		(HMENU)IDC_ANIM_LIST, g_qeglobals.d_hInstance, NULL);
	if (!hwndEnt[EntAnimList])
		Error("CreateWindow failed");

	hwndEnt[EntProps] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
		LVS_REPORT | LVS_SINGLESEL | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_SORTASCENDING |
		WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(HMENU)IDC_E_PROPS,
		g_qeglobals.d_hInstance,
		NULL);
	if (!hwndEnt[EntProps])
		Error("CreateWindow failed");

	ListView_SetExtendedListViewStyle(hwndEnt[EntProps], LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = 150;
	lvc.pszText = "Key";
	ListView_InsertColumn(hwndEnt[EntProps], 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = 250;
	lvc.pszText = "Value";
	ListView_InsertColumn(hwndEnt[EntProps], 1, &lvc);


	hwndEnt[EntComment] = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", NULL,
		ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(HMENU)IDC_E_COMMENT,
		g_qeglobals.d_hInstance,
		NULL);
	if (!hwndEnt[EntComment])
		Error("CreateWindow failed");

	hwndEnt[EntKeyField] = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(HMENU)IDC_E_KEY_FIELD,
		g_qeglobals.d_hInstance,
		NULL);
	if (!hwndEnt[EntKeyField])
		Error("CreateWindow failed");

	hwndEnt[EntValueField] = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(HMENU)IDC_E_VALUE_FIELD,
		g_qeglobals.d_hInstance,
		NULL);
	if (!hwndEnt[EntValueField])
		Error("CreateWindow failed");

	g_wndTabs.SubclassDlgItem(IDC_TAB_MODE, CWnd::FromHandle(ghwndEntity));
	hwndEnt[EntTab] = g_wndTabs.GetSafeHwnd();
	g_wndTabs.InsertItem(0, "Groups");
	::SetParent(g_wndTabs.GetSafeHwnd(), g_qeglobals.d_hwndEntity);

	if (g_pParentWnd->CurrentStyle() > 0 && g_pParentWnd->CurrentStyle() < 3) {
		g_qeglobals.d_hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 
			NULL, ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
			5, 100, 180, 99, g_qeglobals.d_hwndEntity, (HMENU)IDC_E_STATUS,
			g_qeglobals.d_hInstance, NULL);
		if (!g_qeglobals.d_hwndEdit)
			Error("CreateWindow failed");
		g_wndTabs.InsertItem(0, "Console");
		g_wndTabs.InsertItem(0, "Textures");
	}
	g_wndTabs.InsertItem(0, "Entities");
	g_wndTabs.ShowWindow(SW_SHOW);

#if 0
	for (i=0 ; i<12 ; i++)
	{
		hwndEnt[EntCheck1 + i] = CreateWindow ("button", NULL, 
			BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
			5, 100, 180, 99,
			entwindow,
			(HMENU)IDC_E_STATUS,
			main_instance,
			NULL);
		if (!hwndEnt[EntCheck1 + i])
			Error ("CreateWindow failed");
	}
#endif
	SendMessage(hwndEnt[EntList], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(hwndEnt[EntProps], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(hwndEnt[EntComment], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(hwndEnt[EntKeyField], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(hwndEnt[EntValueField], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(hwndEnt[EntTab], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(hwndEnt[EntAnimList], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	if (g_pParentWnd->CurrentStyle() > 0 && g_pParentWnd->CurrentStyle() < 3)
		SendMessage(g_qeglobals.d_hwndEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}



/*
===============================================================

ENTITY WINDOW

===============================================================
*/


void FillClassList(void) {
	eclass_t	*pec;
	int			iIndex;

	SendMessage(hwndEnt[EntList], CB_RESETCONTENT, 0, 0);

	for (pec = eclass; pec; pec = pec->next) {
		iIndex = SendMessage(hwndEnt[EntList], CB_ADDSTRING, 0, (LPARAM)pec->name);
		SendMessage(hwndEnt[EntList], CB_SETITEMDATA, iIndex, (LPARAM)pec);
	}

}


/*
==============
WEnt_Create
==============
*/
void WEnt_Create(HINSTANCE hInstance) {
	WNDCLASS   wc;

	/* Register the camera class */
	memset(&wc, 0, sizeof(wc));

	wc.style = CS_NOCLOSE | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)EntityWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ENT_WINDOW_CLASS;

	RegisterClass(&wc);

	int nStyle = (g_pParentWnd->CurrentStyle() == QR_QE4) ? QE3_STYLE : QE3_STYLE2;
	g_qeglobals.d_hwndEntity = CreateWindow(ENT_WINDOW_CLASS,
		"Entity",
		nStyle,
		20,
		20,
		100,
		480,	// size

		g_qeglobals.d_hwndMain,	// parent
		0,		// no menu
		hInstance,
		NULL);

	if (!g_qeglobals.d_hwndEntity)
		Error("Couldn't create Entity window");
}

/*
==============
CreateEntityWindow
==============
*/
BOOL CreateEntityWindow(HINSTANCE hInstance) {
	HWND hwndEntityPalette;

	inspector_mode = W_ENTITY;

	WEnt_Create(hInstance);

	hwndEntityPalette = CreateDialog(hInstance, (char *)IDD_ENTITY, g_qeglobals.d_hwndMain, (DLGPROC)NULL);
	if (!hwndEntityPalette)
		Error("CreateDialog failed");

	GetEntityControls(hwndEntityPalette);
	DestroyWindow(hwndEntityPalette);

	OldFieldWindowProc = (WNDPROC)GetWindowLong(hwndEnt[EntKeyField], GWL_WNDPROC);
	SetWindowLong(hwndEnt[EntKeyField], GWL_WNDPROC, (long)FieldWndProc);
	SetWindowLong(hwndEnt[EntValueField], GWL_WNDPROC, (long)FieldWndProc);
	OldEntityListWindowProc = (WNDPROC)GetWindowLong(hwndEnt[EntList], GWL_WNDPROC);
	SetWindowLong(hwndEnt[EntList], GWL_WNDPROC, (long)EntityListWndProc);

	FillClassList();


	LoadWindowPlacement(g_qeglobals.d_hwndEntity, "EntityWindowPlace");
	ShowWindow(g_qeglobals.d_hwndEntity, SW_HIDE);
	SetInspectorMode(W_CONSOLE);

	return TRUE;
}

/*
==============
SetInspectorMode
==============
*/
void SetInspectorMode(int iType) {
	RECT rc;
	HMENU hMenu = GetMenu(g_qeglobals.d_hwndMain);

	if ((g_pParentWnd->CurrentStyle() == QR_SPLIT || g_pParentWnd->CurrentStyle() == QR_SPLITZ) && (iType == W_TEXTURE || iType == W_CONSOLE))
		return;


	// Is the caller asking us to cycle to the next window?

	if (iType == -1) {
		if (inspector_mode == W_ENTITY)
			iType = W_TEXTURE;
		else if (inspector_mode == W_TEXTURE)
			iType = W_CONSOLE;
		else if (inspector_mode == W_CONSOLE)
			iType = W_GROUP;
		else
			iType = W_ENTITY;
	}

	inspector_mode = iType;
	switch (iType) {

	case W_ENTITY:
		SetWindowText(g_qeglobals.d_hwndEntity, "Entity");
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_ENABLED | MF_BYCOMMAND);
		// entity is always first in the inspector
		g_wndTabs.SetCurSel(0);
		break;

	case W_TEXTURE:
		SetWindowText(g_qeglobals.d_hwndEntity, "Textures");
		g_pParentWnd->GetTexWnd()->FocusEdit();
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
		if (g_pParentWnd->CurrentStyle() > 0 && g_pParentWnd->CurrentStyle() < 3)
			g_wndTabs.SetCurSel(1);
		break;

	case W_CONSOLE:
		SetWindowText(g_qeglobals.d_hwndEntity, "Console");
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
		if (g_pParentWnd->CurrentStyle() > 0 && g_pParentWnd->CurrentStyle() < 3)
			g_wndTabs.SetCurSel(2);
		break;

	case W_GROUP:
		SetWindowText(g_qeglobals.d_hwndEntity, "Groups");
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
		if (g_pParentWnd->CurrentStyle() > 0 && g_pParentWnd->CurrentStyle() < 3)
			g_wndTabs.SetCurSel(3);
		else
			g_wndTabs.SetCurSel(1);
		break;


	default:
		break;
	}

	GetWindowRect(g_qeglobals.d_hwndEntity, &rc);
	SizeEntityDlg(rc.right - rc.left - 8, rc.bottom - rc.top - 20);


	//	InvalidateRect(entwindow, NULL, true);
	//	ShowWindow (entwindow, SW_SHOW);
	//	UpdateWindow (entwindow);

	HWND hFlag = (g_pParentWnd->CurrentStyle() == QR_QE4) ? HWND_TOP : HWND_TOPMOST;
	SetWindowPos(g_qeglobals.d_hwndEntity, hFlag, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOSIZE | SWP_NOMOVE);
	RedrawWindow(g_qeglobals.d_hwndEntity, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
}



int CALLBACK KeyCompare(LPARAM lhs, LPARAM rhs, LPARAM sort) {
	char rbuf[32], lbuf[32];

	const epair_t* lpep = (const epair_t*)lhs;
	const epair_t* rpep = (const epair_t*)rhs;
	return lstrcmpi(lpep->key, rpep->key);
}

// SetKeyValuePairs
//
// Reset the key/value (aka property) listbox and fill it with the 
// k/v pairs from the entity being edited.
//

void SetKeyValuePairs(bool bClearMD3) {
	epair_t	*pep;
	RECT	rc;
	char	sz[4096];
	LVITEM lv;
	if (edit_entity == NULL)
		return;

	// set key/value pair list
	ListView_DeleteAllItems(hwndEnt[EntProps]);
	// Walk through list and add pairs
	for (pep = edit_entity->epairs; pep; pep = pep->next) {
		int count = ListView_GetItemCount(hwndEnt[EntProps]);
		
		memset(&lv, 0x0, sizeof(lv));
		lv.iItem = count;
		lv.lParam = (LPARAM)pep;
		lv.mask = LVIF_TEXT | LVIF_PARAM;

		int index = ListView_InsertItem(hwndEnt[EntProps], &lv);
		ListView_SetItemText(hwndEnt[EntProps], index, 0, pep->key);
		ListView_SetItemText(hwndEnt[EntProps], index, 1, pep->value);
	}
	ListView_SortItems(hwndEnt[EntProps], KeyCompare, 0);

	// load anims
	if (edit_entity->md3Class) {
		SendMessage(hwndEnt[EntAnimList], CB_RESETCONTENT, 0, 0);
		SendMessage(hwndEnt[EntAnimList], CB_ADDSTRING, 0, (LPARAM)"");

		for (anim_t *pAnim = edit_entity->md3Class->pAnims; pAnim; pAnim = pAnim->pNext) {
			long iIndex = SendMessage(hwndEnt[EntAnimList], CB_ADDSTRING, 0, (LPARAM)pAnim->pszName);
			SendMessage(hwndEnt[EntAnimList], CB_SETITEMDATA, iIndex, (LPARAM)pAnim->pszName);

			if (edit_entity->strTestAnim == pAnim->pszName) {
				SendMessage(hwndEnt[EntList], CB_SETCURSEL, iIndex, 0);
			}
		}
	}
	
	if (bClearMD3) {
		edit_entity->md3Class = NULL;
		edit_entity->brushes.onext->bModelFailed = false;
	}
	Sys_UpdateWindows(W_CAMERA | W_XY);

}

// SetSpawnFlags
// 
// Update the checkboxes to reflect the flag state of the entity
//
void SetSpawnFlags(void) {
	int nFlags = IntForKey(edit_entity, "spawnflags", "0");
	
	for (int i = 0; i < MAX_FLAGS; i++) {
		int v = (nFlags & (1 << i) ) != 0;
		SendMessage(hwndEnt[EntCheck1 + i], BM_SETCHECK, v, 0);	
	}
}

// GetSpawnFlags
// 
// Update the entity flags to reflect the state of the checkboxes
//
void GetSpawnFlags(void) {
	int		i, v;
	char	szValue[32];
	

	int nFlags = 0;
	for (i = 0; i < MAX_FLAGS; i++) {
		int v = SendMessage(hwndEnt[EntCheck1 + i], BM_GETCHECK, 0, 0);
		nFlags |= v << i;
	}
	sprintf(szValue, "%i", nFlags);

	if (multiple_entities) {
		brush_t	*b;
		for (b = selected_brushes.next; b != &selected_brushes; b = b->next) {
			SetKeyValue(b->owner, "spawnflags", szValue);
		}
	} else {
		SetKeyValue(edit_entity, "spawnflags", szValue);
	}
	SetKeyValuePairs(true);
}

// UpdateSel
//
// Update the listbox, checkboxes and k/v pairs to reflect the new selection
//
BOOL UpdateSel(int iIndex, eclass_t *pec) {
	int		i;
	brush_t	*b;

	if (selected_brushes.next == &selected_brushes) {
		edit_entity = world_entity;
		multiple_entities = false;
	} else {
		edit_entity = selected_brushes.next->owner;
		for (b = selected_brushes.next->next; b != &selected_brushes; b = b->next) {
			if (b->owner != edit_entity) {
				multiple_entities = true;
				break;
			}
		}
	}

	if (iIndex != CB_ERR)
		SendMessage(hwndEnt[EntList], CB_SETCURSEL, iIndex, 0);

	if (pec == NULL)
		return TRUE;

	// Set up the description

	SendMessage(hwndEnt[EntComment], WM_SETTEXT, 0, (LPARAM)TranslateString(pec->comments));

	char buffer[24];
	for (i = 0; i < MAX_FLAGS; i++) {
		HWND hwnd = hwndEnt[EntCheck1 + i];
		char *name = pec->spawnflags[i].pstrName;
		if (name && name[0] != 0 && name[0] != '-') {
			EnableWindow(hwnd, TRUE);
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)name);
		} else {
			if (IsGame(GAME_HL)) {
				sprintf(buffer, "%d:%d", i, (int)powf(2, i));
				EnableWindow(hwnd, FALSE);
			} else if (i >= 8) {
				char *gametype[] = { "Easy", "Medium", "Hard", "Deathmatch" };
				strcpy(buffer, gametype[i - 8]);
				EnableWindow(hwnd, TRUE);
			} else {
				strcpy(buffer, "-");
				EnableWindow(hwnd, FALSE);
			}
			// disable check box
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
		}
	}

	SetSpawnFlags();
	SetKeyValuePairs();
	return TRUE;
}

BOOL UpdateEntitySel(eclass_t *pec) {
	int iIndex;

	iIndex = (int)SendMessage(hwndEnt[EntList], CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pec->name);

	return UpdateSel(iIndex, pec);
}

// CreateEntity
//
// Creates a new entity based on the currently selected brush and entity type.
//

void CreateEntity(void) {
	eclass_t *pecNew;
	entity_t *petNew;
	int i;
	HWND hwnd;
	char sz[1024];

	// check to make sure we have a brush

	if (selected_brushes.next == &selected_brushes) {
		MessageBox(g_qeglobals.d_hwndMain, "You must have a selected brush to create an entity", "info", 0);
		return;
	}


	// find out what type of entity we are trying to create

	hwnd = hwndEnt[EntList];

	i = SendMessage(hwnd, CB_GETCURSEL, 0, 0);

	if (i < 0) {
		MessageBox(g_qeglobals.d_hwndMain, "You must have a selected class to create an entity", "info", 0);
		return;
	}

	SendMessage(hwnd, CB_GETLBTEXT, i, (LPARAM)sz);

	if (!stricmp(sz, "worldspawn")) {
		MessageBox(g_qeglobals.d_hwndMain, "Can't create an entity with worldspawn.", "info", 0);
		return;
	}

	pecNew = Eclass_ForName(sz, false);

	// create it

	petNew = Entity_Create(pecNew);

	if (petNew == NULL) {
		MessageBox(g_qeglobals.d_hwndMain, "Failed to create entity.", "info", 0);
		return;
	}

	if (selected_brushes.next == &selected_brushes)
		edit_entity = world_entity;
	else
		edit_entity = selected_brushes.next->owner;

	SetKeyValuePairs(true);
	Select_Deselect();
	Select_Brush(edit_entity->brushes.onext);
	Sys_UpdateWindows(W_ALL);

}



/*
===============
AddProp

===============
*/
void AddProp() {
	char	key[4096];
	char	value[4096];

	if (edit_entity == NULL)
		return;

	// Get current selection text

	SendMessage(hwndEnt[EntKeyField], WM_GETTEXT, sizeof(key) - 1, (LPARAM)key);
	SendMessage(hwndEnt[EntValueField], WM_GETTEXT, sizeof(value) - 1, (LPARAM)value);

	if (multiple_entities) {
		brush_t	*b;

		for (b = selected_brushes.next; b != &selected_brushes; b = b->next)
			SetKeyValue(b->owner, key, value);
	} else
		SetKeyValue(edit_entity, key, value);

	// refresh the prop listbox
	SetKeyValuePairs(true);

	// if it's a plugin entity, perhaps we need to update some drawing parameters
	// NOTE: perhaps moving this code to a seperate func would help if we need it in other places
	// TODO: we need to call some update func in the IPluginEntity in case model name changes etc.
	// ( for the moment only bounding brush is updated ), see UpdateModelBrush in Ritual's Q3Radiant
	if (edit_entity->eclass->nShowFlags & ECLASS_PLUGINENTITY) {
		vec3_t	mins, maxs;
		edit_entity->pPlugEnt->GetBounds(mins, maxs);
		// replace old bounding brush by newly computed one
		// NOTE: this part is similar to Entity_BuildModelBrush in Ritual's Q3Radiant, it can be
		// usefull moved into a seperate func
		brush_t *b, *oldbrush;
		if (edit_entity->brushes.onext != &edit_entity->brushes)
			oldbrush = edit_entity->brushes.onext;
		b = Brush_Create(mins, maxs, &edit_entity->eclass->texdef);
		Entity_LinkBrush(edit_entity, b);
		Brush_Build(b, true);
		Select_Deselect();
		Brush_AddToList(edit_entity->brushes.onext, &selected_brushes);
		if (oldbrush)
			Brush_Free(oldbrush);
	}

}

/*
===============
DelProp

===============
*/
void DelProp(void) {
	char	sz[4096];

	if (edit_entity == NULL)
		return;

	// Get current selection text

	SendMessage(hwndEnt[EntKeyField], WM_GETTEXT, sizeof(sz) - 1, (LPARAM)sz);

	if (multiple_entities) {
		brush_t	*b;

		for (b = selected_brushes.next; b != &selected_brushes; b = b->next)
			DeleteKey(b->owner, sz);
	} else
		DeleteKey(edit_entity, sz);

	// refresh the prop listbox

	SetKeyValuePairs(true);
}

BOOL GetSelectAllCriteria(CString &strKey, CString &strVal) {
	char	sz[4096];
	int pos = ListView_GetNextItem(hwndEnt[EntProps], -1, LVNI_SELECTED);
	if (pos >= 0 && inspector_mode == W_ENTITY) {
		ListView_GetItemText(hwndEnt[EntProps], pos, 0, sz, sizeof(sz));
		SendMessage(hwndEnt[EntKeyField], WM_SETTEXT, 0, (LPARAM)sz);
		strKey = sz;

		ListView_GetItemText(hwndEnt[EntProps], pos, 1, sz, sizeof(sz));
		SendMessage(hwndEnt[EntValueField], WM_SETTEXT, 0, (LPARAM)sz);
		strVal = sz;
		return TRUE;
	}
	return FALSE;
}

/*
===============
EditProp

===============
*/
void EditProp(void) {
	char	sz[4096];
	if (edit_entity == NULL)
		return;

	int pos = ListView_GetNextItem(hwndEnt[EntProps], -1, LVNI_SELECTED);
	if (pos == -1) {
		return;
	}
	ListView_GetItemText(hwndEnt[EntProps], pos, 0, sz, sizeof(sz));
	SendMessage(hwndEnt[EntKeyField], WM_SETTEXT, 0, (LPARAM)sz);

	ListView_GetItemText(hwndEnt[EntProps], pos, 1, sz, sizeof(sz));
	SendMessage(hwndEnt[EntValueField], WM_SETTEXT, 0, (LPARAM)sz);
}


HDWP	defer;
int		col;
void MOVE(HWND e, int x, int y, int w, int h, HWND hwndPlacement = HWND_TOP, int swp = SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER) {
	//	defer=DeferWindowPos(defer,e,HWND_TOP,col+(x),y,w,h,SWP_SHOWWINDOW);
	//	MoveWindow (e, col+x, y, w, h, FALSE);
	SetWindowPos(e, hwndPlacement, col + x, y, w, h, swp);
}


void WrapSizer(HWND hwnd, int px, int py, int sx, int sy, int sWidth, int sHeight) {
	// x, y
	if (px < 0) {
		px = sWidth - labs(px);
	}
	if (py < 0) {
		py = sHeight - labs(py);
	}
	if (sx < 0) {
		sx = sWidth - labs(sx) - px;
	}
	if (sy < 0) {
		sy = sHeight - labs(sy) - py;
	}
	SetWindowPos(hwnd, HWND_TOP, px, py, sx, sy, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
	ShowWindow(hwnd, SW_SHOW);
}


/*
===============
SizeEnitityDlg

Positions all controls so that the active inspector
is displayed correctly and the inactive ones are
off the side
===============
*/
void SizeEntityDlg(int iWidth, int iHeight) {
	int y, x, xCheck, yCheck;
	int i, iRow;
	int	w, h;

	int pWidth = iWidth, pHeight = iHeight;

	if (iWidth < 32 || iHeight < 32)
		return;


	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 0, 0);

	for (int i = 0; i < EntLast; i++) {
		ShowWindow(hwndEnt[EntList + i], SW_HIDE);
	}
	ShowWindow(g_qeglobals.d_hwndGroup, SW_HIDE);
	ShowWindow(g_qeglobals.d_hwndEdit, SW_HIDE);
	ShowWindow(g_qeglobals.d_hwndTexture, SW_HIDE);

	
	if (inspector_mode == W_CONSOLE) {
		WrapSizer(g_qeglobals.d_hwndEdit, 4, 4, -10, -34, pWidth, pHeight);
	} else if (inspector_mode == W_TEXTURE) {
		WrapSizer(g_qeglobals.d_hwndTexture, 4, 4, -10, -34, pWidth, pHeight);
	} else if (inspector_mode == W_GROUP) {
		WrapSizer(g_qeglobals.d_hwndGroup, 4, 4, -10, -34, pWidth, pHeight);
	} else {
		WrapSizer(hwndEnt[EntList], 4, 4, -82, 20, pWidth, pHeight);
		WrapSizer(hwndEnt[EntCreateEntity], -80, 4, 70, 20, pWidth, pHeight);

		WrapSizer(hwndEnt[EntDir225], 4, -54, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDir180], 4, -72, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDir135], 4, -90, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDir270], 40, -54, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDir90], 40, -90, 36, 18, pWidth, pHeight);

		WrapSizer(hwndEnt[EntDir315], 76, -54, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDir0], 76, -72, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDir45], 76, -90, 36, 18, pWidth, pHeight);

		WrapSizer(hwndEnt[EntDirUp], 120, -81, 36, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntDirDown], 120, -63, 36, 18, pWidth, pHeight);

		WrapSizer(hwndEnt[EntAssignSounds], -74, -90, 60, 20, pWidth, pHeight);
		WrapSizer(hwndEnt[EntAssignModels], -74, -66, 60, 20, pWidth, pHeight);

		WrapSizer(hwndEnt[EntAnimList], 160, -90, -78, 20, pWidth, pHeight);

		WrapSizer(hwndEnt[EntDelProp], -54, -120, 40, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntValueLabel], 4, -118, 40, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntValueField], 48, -120, -60, 18, pWidth, pHeight);

		WrapSizer(hwndEnt[EntKeyLabel], 4, -138, 40, 18, pWidth, pHeight);
		WrapSizer(hwndEnt[EntKeyField], 48, -140, -14, 18, pWidth, pHeight);

		

		int center = pHeight / 2;
		WrapSizer(hwndEnt[EntProps], 4, center, -14, -144, pWidth, pHeight);

		int sep = 140;
		for (int i = 0; i < 12; i++) {
			int px = (i / 4) * sep + 4;
			int py = (-center - 80) + (i % 4) * 20;
			WrapSizer(hwndEnt[EntCheck1 + i], px, py, sep, 18, pWidth, pHeight);
		}

		WrapSizer(hwndEnt[EntComment], 4, 28, -14, -center - 82, pWidth, pHeight);
	}
	SetWindowPos(hwndEnt[EntTab], HWND_BOTTOM, 0, 0, pWidth, pHeight - 6, SWP_NOACTIVATE | SWP_NOCOPYBITS);
	ShowWindow(hwndEnt[EntTab], SW_SHOW);
	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 1, 0);
	// InvalidateRect(entwindow, NULL, TRUE);
}

void AssignSound() {
	CString strBasePath = ValueForKey(g_qeglobals.d_project_entity, "modpath");
	if (strBasePath.IsEmpty()) {
		strBasePath = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	}
	CString strPath, strKey = "noise";
	strPath.Format("%s\\sound\\", strBasePath);
	if (IsGame(GAME_HL) && edit_entity->eclass->nShowFlags & ECLASS_SOUND) {
		strKey = "message";
	}

	CWaveOpen dlgFile(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Sound files (*.wav)|*.wav||", g_pParentWnd);
	dlgFile.m_ofn.lpstrInitialDir = strPath;
	if (dlgFile.DoModal() == IDOK) {
		CString strValue = dlgFile.GetPathName();
		strValue.MakeLower();
		strValue.Replace("\\", "/");
		strValue.Replace(strBasePath + "/", "");

		SendMessage(hwndEnt[EntKeyField], WM_SETTEXT, 0, (LPARAM)strKey.GetBuffer());
		SendMessage(hwndEnt[EntValueField], WM_SETTEXT, 0, (LPARAM)strValue.GetBuffer());
		
		AddProp();
		g_pParentWnd->GetXYWnd()->SetFocus();
	}
}

void AssignModel() {
	CString strBasePath = ValueForKey(g_qeglobals.d_project_entity, "modpath");
	if (strBasePath.IsEmpty()) {
		strBasePath = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	}
	CString strPath, strFilter, strKey = "model";

	if (IsGame(GAME_HL) && edit_entity->eclass->nShowFlags & (ECLASS_BEAM | ECLASS_SPRITE)) {
		strPath.Format("%s\\sprites\\", strBasePath);
		strFilter = "Sprite files (*.spr)|*.spr||";
		if (edit_entity->eclass->nShowFlags & ECLASS_BEAM) {
			strKey = "texture";
		}
	} else if (IsGame(GAME_ID3)) {
		strPath.Format("%s\\models\\mapobjects\\", strBasePath);
		strFilter = "Model files (*.md3)|*.md3||";
	} else {
		Sys_Printf("Not implemets\n");
		return;
	}

	CFileDialog dlgFile(TRUE, NULL, NULL, OFN_OVERWRITEPROMPT, strFilter, g_pParentWnd);
	dlgFile.m_ofn.lpstrInitialDir = strPath;
	if (dlgFile.DoModal() == IDOK) {
		CString strValue = dlgFile.GetPathName();
		strValue.MakeLower();
		strValue.Replace("\\", "/");
		strValue.Replace(strBasePath + "/", "");
		
		SendMessage(hwndEnt[EntKeyField], WM_SETTEXT, 0, (LPARAM)strKey.GetBuffer());
		SendMessage(hwndEnt[EntValueField], WM_SETTEXT, 0, (LPARAM)strValue.GetBuffer());
		
		AddProp();
		edit_entity->md3Class = NULL;
		edit_entity->brushes.onext->bModelFailed = false;
		g_pParentWnd->GetXYWnd()->SetFocus();
	}
}

void SetAngle(entity_t *e, int deg) {
	char buffer[64];
	if (IsGame(GAME_HL)) {
		vec3_t angles;
		DegToAngle(deg, angles);
		sprintf(buffer, "%d %d %d", (int)angles[0], (int)angles[1], (int)angles[2]);
		SetKeyValue(e, "angles", buffer);
	} else {
		sprintf(buffer, "%d", deg);
		SetKeyValue(e, "angle", buffer);
	}
}

/*
=========================
EntityWndProc
=========================
*/
BOOL CALLBACK EntityWndProc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam)	// second message parameter
{
	LPNMHDR lpnmh = NULL;
	RECT	rc;

	GetClientRect(hwndDlg, &rc);

	switch (uMsg) {

	case WM_CHAR:
	{
		char c = toupper(LOWORD(wParam));
		// escape: hide the window
		if (c == 27)
			ShowWindow(hwndDlg, SW_HIDE);
		if (c == 'N')
			g_pParentWnd->PostMessage(WM_COMMAND, ID_VIEW_ENTITY, 0);
		else if (c == 'O')
			g_pParentWnd->PostMessage(WM_COMMAND, ID_VIEW_CONSOLE, 0);
		else if (c == 'T')
			g_pParentWnd->PostMessage(WM_COMMAND, ID_VIEW_TEXTURE, 0);
		else if (c == 'G')
			g_pParentWnd->PostMessage(WM_COMMAND, ID_VIEW_GROUPS, 0);
		else
			DefWindowProc(hwndDlg, uMsg, wParam, lParam);
		break;
	}

	case WM_NOTIFY:
		lpnmh = reinterpret_cast<LPNMHDR>(lParam);
		if (lpnmh->idFrom == IDC_E_PROPS && lpnmh->code == NM_CLICK) {
			EditProp();
		} else if (lpnmh->hwndFrom == g_wndTabs.GetSafeHwnd()) {
			if (lpnmh->code == TCN_SELCHANGE) {
				int n = g_wndTabs.GetCurSel();
				if (g_pParentWnd->CurrentStyle() == 2 || g_pParentWnd->CurrentStyle() == 1) {
					if (n == 0) {
						SetInspectorMode(W_ENTITY);
					} else if (n == 1) {
						SetInspectorMode(W_TEXTURE);
					} else if (n == 2) {
						SetInspectorMode(W_CONSOLE);
					} else {
						SetInspectorMode(W_GROUP);
					}
				} else {
					if (n == 0) {
						SetInspectorMode(W_ENTITY);
					} else if (n == 1) {
						SetInspectorMode(W_GROUP);
					}
				}
			}
		}
		break;

	case WM_SIZE:

		DefWindowProc(hwndDlg, uMsg, wParam, lParam);
		break;

	case WM_DESTROY:
		SaveWindowPlacement(g_qeglobals.d_hwndEntity, "EntityWindowPlace");
		DefWindowProc(hwndDlg, uMsg, wParam, lParam);
		break;

	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO	lpmmi;

		lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMinTrackSize.x = 320;
		lpmmi->ptMinTrackSize.y = 500;
	}
	return 0;

	case WM_WINDOWPOSCHANGING:
	{
		LPWINDOWPOS	lpwp;
		lpwp = (LPWINDOWPOS)lParam;

		DefWindowProc(hwndDlg, uMsg, wParam, lParam);

		lpwp->flags |= SWP_NOCOPYBITS;
		SizeEntityDlg(lpwp->cx - 8, lpwp->cy - 32);
		return 0;

	}
	return 0;


	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_E_CREATE:
			CreateEntity();
			SetFocus(g_qeglobals.d_hwndCamera);
			break;

		case IDC_BTN_ASSIGNSOUND:
			AssignSound();
			break;

		case IDC_BTN_ASSIGNMODEL:
			AssignModel();
			break;

		case IDC_E_DELPROP:
			DelProp();
			SetFocus(g_qeglobals.d_hwndCamera);
			break;

		case IDC_E_0:
			SetAngle(edit_entity, 0);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_45:
			SetAngle(edit_entity, 45);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_90:
			SetAngle(edit_entity, 90);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_135:
			SetAngle(edit_entity, 135);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_180:
			SetAngle(edit_entity, 180);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_225:
			SetAngle(edit_entity, 225);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_270:
			SetAngle(edit_entity, 270);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_315:
			SetAngle(edit_entity, 315);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_UP:
			SetAngle(edit_entity, -1);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;
		case IDC_E_DOWN:
			SetAngle(edit_entity, -2);
			SetFocus(g_qeglobals.d_hwndCamera);
			SetKeyValuePairs(true);
			break;

		case IDC_BTN_HIDE:
			::PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CAMERATOGGLE, 0);
			break;

		case IDC_CHECK1:
		case IDC_CHECK2:
		case IDC_CHECK3:
		case IDC_CHECK4:
		case IDC_CHECK5:
		case IDC_CHECK6:
		case IDC_CHECK7:
		case IDC_CHECK8:
		case IDC_CHECK9:
		case IDC_CHECK10:
		case IDC_CHECK11:
		case IDC_CHECK12:
			GetSpawnFlags();
			SetFocus(g_qeglobals.d_hwndCamera);
			break;

			/*
		case IDC_E_PROPS:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:

				EditProp();
				return TRUE;
			}
			break;
			*/
		case IDC_ANIM_LIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int iIndex = SendMessage(hwndEnt[EntAnimList], CB_GETCURSEL, 0, 0);
				edit_entity->strTestAnim = (char *)SendMessage(hwndEnt[EntAnimList], CB_GETITEMDATA, iIndex, 0);
			}
			break;
		case IDC_E_LIST:

			switch (HIWORD(wParam)) {

			case CBN_SELCHANGE:
			{
				int iIndex;
				eclass_t *pec;

				iIndex = SendMessage(hwndEnt[EntList], CB_GETCURSEL, 0, 0);
				pec = (eclass_t *)SendMessage(hwndEnt[EntList], CB_GETITEMDATA, iIndex, 0);

				UpdateSel(iIndex, pec);

				return TRUE;
				break;
			}

			case CBN_DBLCLK:
				CreateEntity();
				SetFocus(g_qeglobals.d_hwndCamera);
				break;
			}
			break;


		default:
			return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
		}

		return 0;
	}

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

