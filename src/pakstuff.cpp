#include "stdafx.h"
#include "qe3.h"
#include "pakstuff.h"
#include "pak/unzip.h"


struct searchpath_t {
	char            filename[1024];

	FILE *			handle;
	dpackfile_t *   files;
	int             numfiles;

	unzFile			zhandle; //
	searchpath_t *  next;
};




void AppendFileList(CStringArray &list, CString path, const CString &filter) {
	path.MakeLower();
	int size = filter.GetLength();
	if (path.Find(filter) != 0) {
		return;
	}
	path = path.Mid(size);
	int pos = path.Find('/');
	if (pos != -1) {
		path = path.Mid(0, pos);
	}
	if (path.IsEmpty()) {
		return;
	}
	for (int i = 0; i < list.GetCount(); i++) {
		const CString &name = list.GetAt(i);
		if (name == path) {
			return;
		}
	}
	list.Add(path);
}



searchpath_t *q_searchpath = nullptr;
CString g_cBaseName;

void WriteOutTest(byte *buffer, int len) {
	FILE *output = fopen("D:\\BuildCode\\quake3\\tools2\\test\\out.jpg", "wb");
	assert(output != nullptr);
	fwrite(buffer, sizeof(byte), len, output);
	fclose(output);
}



void InitPakFile() {
	g_cBaseName = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	g_cBaseName.MakeLower();
	g_cBaseName.Replace("\\", "/");
	AddSearchPath(g_cBaseName);
	g_cBaseName.AppendChar('/');

	char *modpath = ValueForKey(g_qeglobals.d_project_entity, "modpath");
	if (modpath[0] != 0) {
		AddSearchPath(modpath);
	}
}


void FindPAKFiles(const char *pBasePath) {
	char cWork[1024], exts[64];
	sprintf(cWork, "%s/*.*", pBasePath);

	struct _finddata_t fileinfo;
	int handle = _findfirst(cWork, &fileinfo);
	if (handle == -1) {
		return;
	}
	do {
		sprintf(cWork, "%s\\%s", pBasePath, fileinfo.name);
		ExtractFileExtension(cWork, exts);
		if (stricmp(exts, "pk3") == 0 || stricmp(exts, "pak") == 0) {
			AddSearchPath(cWork);
		}
	} while (_findnext(handle, &fileinfo) != -1);
	_findclose(handle);
}

bool AddSearchPath(const char *filename) {
	char exts[64];
	searchpath_t *search;
	unz_global_info global_info;
	unz_file_info zInfo;
	char cFilename[1024];

	ExtractFileExtension(filename, exts);
	DWORD attr = GetFileAttributes(filename);
	if (attr & FILE_ATTRIBUTE_DIRECTORY) {
		FindPAKFiles(filename);

		search = (searchpath_t *)qmalloc(sizeof(searchpath_t));
		Sys_Printf("Add search path: %s\n", filename);
	} else if (stricmp(exts, "pak") == 0) {
		FILE *handle = fopen(filename, "rb");
		if (!handle) {
			return false;
		}
		dpackheader_t header;
		fread(&header, sizeof(dpackheader_t), 1, handle);
		if (strncmp(header.id, "PACK", 4) != 0) {
			fclose(handle);
			return false;
		}
		// LittleLong
		search = (searchpath_t *)qmalloc(sizeof(searchpath_t));
		search->handle = handle;
		search->numfiles = header.dirlen / sizeof(dpackfile_t);
		search->files = (dpackfile_t *)qmalloc(sizeof(dpackfile_t) * search->numfiles);
		fseek(handle, header.dirofs, SEEK_SET);
		fread(search->files, sizeof(dpackfile_t), search->numfiles, handle);
		Sys_Printf("Load PAK: %s\n", filename);
	} else if (stricmp(exts, "pk3") == 0 || stricmp(exts, "zip") == 0) {
		unzFile zFile = unzOpen(filename);
		if (!zFile) {
			return false;
		}
		unzGetGlobalInfo(zFile, &global_info);
		search = (searchpath_t *)qmalloc(sizeof(searchpath_t));
		search->zhandle = zFile;
		search->numfiles = global_info.number_entry;
		Sys_Printf("Load PK3: %s\n", filename);
	} else {
		return false;
	}
	strcpy(search->filename, filename);
	search->next = q_searchpath;
	q_searchpath = search;
	return true;
}



int PakLoadFile(const char *filename, void **bufferptr) {
	char cWork[1024];
	unz_file_info zInfo;

	CString filepath = filename;
	filepath.Replace("\\", "/");
	filepath.MakeLower();
	filepath.Replace(g_cBaseName, "");

	for (searchpath_t *search = q_searchpath; search; search = search->next) {
		if (search->handle) {
			for (int i = 0; i < search->numfiles; i++) {
				dpackfile_t *file = &search->files[i];
				if (stricmp(file->name, filepath) == 0) {
					fseek(search->handle, file->filepos, SEEK_SET);
					byte *buffer = (byte *)qmalloc(file->filelen + 1);
					int rd = fread(buffer, sizeof(byte), file->filelen, search->handle);
					assert(rd == file->filelen);
					*bufferptr = buffer;
					return file->filelen;
				}
			}
			continue;
		}
		if (search->zhandle) {
			int nStatus = unzGoToFirstFile(search->zhandle);
			while (nStatus == UNZ_OK) {
				unzGetCurrentFileInfo(search->zhandle, &zInfo, cWork, sizeof(cWork), NULL, 0, NULL, 0);
				if (stricmp(cWork, filepath) == 0) {
					if (unzOpenCurrentFile(search->zhandle) == UNZ_OK) {
						byte *buffer = (byte *)qmalloc(zInfo.uncompressed_size + 1);
						int rd = unzReadCurrentFile(search->zhandle, buffer, zInfo.uncompressed_size);
						assert(rd == zInfo.uncompressed_size);
						*bufferptr = buffer;
						int cs = unzCloseCurrentFile(search->zhandle);
						assert(cs == UNZ_OK);
						return zInfo.uncompressed_size;
					}
				}
				nStatus = unzGoToNextFile(search->zhandle);
			}
			continue;
		}

		sprintf(cWork, "%s/%s", search->filename, filepath);
		FILE *handle = fopen(cWork, "rb");
		if (handle) {
			// Sys_Printf("Find (2): %s\n", filepath);
			fseek(handle, 0, SEEK_END);
			int size = ftell(handle);
			rewind(handle);

			void *buffer = qmalloc(size + 1);
			fread(buffer, sizeof(char), size, handle);
			fclose(handle);

			*bufferptr = buffer;
			return size;
		}
	}
	return -1;
}

void ClosePakFile() {
	searchpath_t *search = q_searchpath, *next;
	while (search) {
		next = search->next;
		search->numfiles = 0;
		if (search->zhandle) {
			unzClose(search->zhandle);
			search->zhandle = nullptr;
		}
		if (search->files) {
			free(search->files);
			search->files = nullptr;
		}
		if (search->handle) {
			fclose(search->handle);
			search->handle = nullptr;
		}
		free(search);
		search = next;
	}
	q_searchpath = nullptr;
}


int GetFileList(const char *filter, CStringArray &list) {
	char cWork[1024];

	CString dirname = filter;
	dirname.Replace("\\", "/");
	dirname.MakeLower();
	dirname.Replace(g_cBaseName, "");

	for (searchpath_t *search = q_searchpath; search; search = search->next) {
		if (search->handle) {
			for (int i = 0; i < search->numfiles; i++) {
				dpackfile_t *file = &search->files[i];
				AppendFileList(list, file->name, dirname);
			}
			continue;
		}
		if (search->zhandle) {
#if 1
			int nStatus = unzGoToFirstFile(search->zhandle);
			while (nStatus == UNZ_OK) {
				unz_file_info zInfo;
				unzGetCurrentFileInfo(search->zhandle, &zInfo, cWork, sizeof(cWork), NULL, 0, NULL, 0);
				AppendFileList(list, cWork, dirname);
				nStatus = unzGoToNextFile(search->zhandle);
			}
#endif
			continue;
		}

		struct _finddata_t fileinfo;
		sprintf(cWork, "%s/%s", search->filename, dirname);
		int handle = _findfirst(cWork, &fileinfo);
		if (handle != -1) {
			do {
				// fileinfo.attrib & _A_SUBDIR
				if (fileinfo.name[0] == '.')
					continue;
				sprintf(cWork, "%s/%s", dirname, fileinfo.name);
				AppendFileList(list, cWork, dirname);
			} while (_findnext(handle, &fileinfo) != -1);
			_findclose(handle);
		}
	}
	return -1;
}

