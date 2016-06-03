/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 

/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2014 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
/**
 * This file is used for compatibility across windows and mac/linux platforms.
 * This is specific to FLuoRender Code.
 * @author Brig Bagley
 * @version 4 March 2014
 */
#ifndef __COMPATIBILITY_H__
#define __COMPATIBILITY_H__

#ifdef _WIN32 //WINDOWS ONLY

#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include <ole2.h>
#include <time.h>
#include <sys/types.h>
#include <ctype.h>
#include <direct.h>
#include <codecvt>

#define GETCURRENTDIR _getcwd

#define FSEEK64     _fseeki64
#define SSCANF    sscanf

inline wchar_t GETSLASH() { return L'/'; }

inline std::wstring s2ws(const std::string& utf8) {
//    return std::wstring( str.begin(), str.end() );
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.from_bytes(utf8);
}

inline std::string ws2s(const std::wstring& utf16) {
//    return std::string( str.begin(), str.end() );
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.to_bytes(utf16);
}

inline FILE* FOPEN(FILE** fp, const char *fname, const char* mode) {
    fopen_s(fp,fname,mode);
	return *fp;
}

inline FILE* WFOPEN(FILE** fp, const wchar_t* fname, const wchar_t* mode) {
   _wfopen_s(fp,fname,mode);
   return *fp;
}

inline errno_t STRCPY(char* d, size_t n, const char* s) { return strcpy_s(d,n,s); }

inline errno_t STRNCPY(char* d, size_t n, const char* s, size_t x) {
   return strncpy_s(d,n,s,x);
}

inline errno_t STRCAT(char * d, size_t n, const char* s) {
   return strcat_s(d,n,s);
}

inline char* STRDUP(const char* s) { return _strdup(s); }

inline int SPRINTF(char* buf, size_t n, const char * fmt, ...) {
   va_list args;
   va_start(args,fmt);
   int r = vsprintf_s(buf,n,fmt,args);
   va_end(args);
   return r;
}

inline int WSTOI(std::wstring s) { return _wtoi(s.c_str()); }

inline double WSTOD(std::wstring s) { return _wtof(s.c_str()); }

inline int STOI(const char * s) { return atoi(s); }

inline double STOD(const char * s) { return atof(s); }

inline time_t TIME(time_t* n) { return _time32((__time32_t*)n); }

inline int CREATE_DIR(const wchar_t *f) { return CreateDirectory(f,NULL); }

inline uint32_t GET_TICK_COUNT() { return GetTickCount(); }

/*
void FIND_FILES(std::string path, std::string ext, std::vector<std::string> & list, int cur_batch, std::string regex = "")
{
    QDir dir(path);
    QFileInfoList files = dir.entryInfoList();
    list.clear();
    foreach(const QFileInfo &fi, files)
    {
        QString filename = fi.absoluteFilePath();
        list.push_back(filename);
    }
}
*/

inline void FIND_FILES(std::wstring m_path_name,
      std::wstring search_ext,
      std::vector<std::wstring> &m_batch_list,
      int &m_cur_batch, std::wstring regex = L"") {
   std::wstring search_path = m_path_name.substr(0,
         m_path_name.find_last_of(L'/')) + L'/';
   std::wstring search_str = regex + L"*" + search_ext;
   if (std::string::npos == search_str.find(m_path_name))
	   search_str = m_path_name + search_str;
   WIN32_FIND_DATAW FindFileData;
   HANDLE hFind;
   hFind = FindFirstFileW(search_str.c_str(), &FindFileData);
   if (hFind != INVALID_HANDLE_VALUE)
   {
      int cnt = 0;
      m_batch_list.clear();
      std::wstring name = search_path + FindFileData.cFileName;
      m_batch_list.push_back(name);
      if (name == m_path_name)
         m_cur_batch = cnt;
      cnt++;

      while (FindNextFileW(hFind, &FindFileData) != 0)
      {
         name = search_path + FindFileData.cFileName;
         m_batch_list.push_back(name);
         if (name == m_path_name)
            m_cur_batch = cnt;
         cnt++;
      }
   }
   FindClose(hFind);
}

#else // MAC OSX or LINUX

#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <vector>
#include <iostream>
//#include <locale>

#include <QString>

#define GETCURRENTDIR getcwd

#define FSEEK64     fseek

inline wchar_t GETSLASH() { return L'/'; }

inline std::wstring s2ws(const std::string& utf8) {
//    return std::wstring( str.begin(), str.end() );
//    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
//    return converter.from_bytes(utf8);
    QString str(utf8.c_str());
    return str.toStdWString();
}

inline std::string ws2s(const std::wstring& utf16) {
//    return std::string( str.begin(), str.end() );
//    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
//    return converter.to_bytes(utf16);
    QString str(QString::fromWCharArray(utf16.c_str()));
    return str.toStdString();
}

/*
inline int SSCANF(const char* buf, const char* fmt, ...){
   va_list args;
   va_start(args,fmt);
   int r = vsscanf(buf,fmt,args);
   va_end(args);
   return r;
}

inline int swprintf_s(wchar_t *buf, size_t n, const wchar_t* fmt, ...) {
   va_list args;
   va_start(args,fmt);
   int r = vswprintf(buf,n,fmt,args);
   va_end(args);
   return r;
}

inline void SetDoubleBuffered(bool) {};

inline char* STRCPY(char* d, size_t n, const char* s) { return strncpy(d,s,n-1); }

inline char* STRNCPY(char* d, size_t n, const char* s, size_t x) {
   return strncpy(d,s,n-1);
}

inline char* STRCAT(char * d, size_t n, const char* s) {
   return strncat(d,s,n-strlen(d)-1);
}

inline char* STRDUP(const char* s) { return strdup(s); }

inline TIFF* TIFFOpenW(std::wstring fname, const char* opt) {
   return TIFFOpen(ws2s(fname).c_str(),opt);
}

inline int SPRINTF(char* buf, size_t n, const char * fmt, ...) {
   va_list args;
   va_start(args,fmt);
   int r = vsprintf(buf,fmt,args);
   va_end(args);
   return r;
}
*/

inline int WSTOI(std::wstring s) { return atoi(ws2s(s).c_str()); }

inline double WSTOD(std::wstring s) { return atof(ws2s(s).c_str()); }

inline int STOI(const char * s) { return atoi(s); }

inline double STOD(const char * s) { return atof(s); }

inline time_t TIME(time_t* n) { return time(n); }

inline int CREATE_DIR(const char *f) { return mkdir(f, S_IRWXU | S_IRGRP | S_IXGRP); }

typedef union _LARGE_INTEGER {
   struct {
      unsigned int LowPart;
      long HighPart;
   };
   struct {
      unsigned int LowPart;
      long HighPart;
   } u;
   long long QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

inline void FIND_FILES(std::wstring m_path_name,
      std::wstring search_ext,
      std::vector<std::wstring> &m_batch_list,
      int &m_cur_batch, std::wstring regex = L"") {
   std::wstring search_path = m_path_name.substr(0,m_path_name.find_last_of(L'/')) + L'/';
   std::wstring regex_min;
   if(regex.find(search_path) != std::string::npos)
      regex_min = regex.substr(search_path.length(),regex.length() - search_path.length());
   else
      regex_min = regex;
   DIR* dir;
   struct dirent *ent;
   if ((dir = opendir(ws2s(search_path).c_str())) != NULL) {
      int cnt = 0;
      m_batch_list.clear();
      while((ent = readdir(dir)) != NULL) {
         std::string file(ent->d_name);
         std::wstring wfile = s2ws(file);
         //check if it contains the string.
         if (wfile.find(search_ext) != std::string::npos &&
               wfile.find(regex_min) != std::string::npos) {
            std::string ss = ent->d_name;
            std::wstring f = s2ws(ss);
            std::wstring name;
            if(f.find(search_path) == std::string::npos)
               name = search_path + f;
            else
               name = f;
            m_batch_list.push_back(name);
            if (name == m_path_name)
               m_cur_batch = cnt;
            cnt++;
         }
      }
   }
}

inline FILE* WFOPEN(FILE ** fp, const wchar_t* filename, const wchar_t* mode) {
   *fp = fopen(ws2s(std::wstring(filename)).c_str(),
         ws2s(std::wstring(mode)).c_str());
   return *fp;
}

inline FILE* FOPEN(FILE ** fp, const char* filename, const char* mode) {
   *fp = fopen(filename,mode);
   return *fp;
}

inline uint32_t GET_TICK_COUNT() {
   struct timeval ts;
   gettimeofday(&ts, NULL);
   return ts.tv_sec * 1000 + ts.tv_usec / 1000;
}

//LINUX SPECIFIC
#ifdef _LINUX
#endif
//MAC OSX SPECIFIC
#ifdef _DARWIN
#endif

#endif //END_IF_DEF__WIN32__

#endif //END__COMPATIBILITY_H__
