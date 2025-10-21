
#ifndef _PAKSTUFF_H_
#define _PAKSTUFF_H_

#include <windows.h>
#ifdef __cplusplus
extern "C"
{
#endif


void InitPakFile();
int PakLoadFile(const char *filename, void **bufferptr);
bool AddSearchPath(const char *filename);
void ClosePakFile();
int GetFileList(const char *filter, CStringArray &list);

void AppendFileList(CStringArray &list, CString path, const CString &filter);


#ifdef __cplusplus
}
#endif

#endif
