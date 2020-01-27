/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAOIFReader.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "io/iAFileUtils.h"

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageIOFactory.h>

#include <vtkImageData.h>

#include <QString>

#include <string>
#include <vector>

/*
Parts of this file taken from the source code of FluoRender
http://www.sci.utah.edu/software/fluorender.html
FluoRender is under the MIT License

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

// BEGIN COMPATIBILITY

// some code for compatibility across windows and mac/linux platforms.
// This is specific to FLuoRender Code.
// @author Brig Bagley
// @version 4 March 2014
#ifdef _WIN32 //WINDOWS ONLY
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <ole2.h>
#include <ctime>
#include <sys/types.h>
#include <ctype.h>
#include <direct.h>
#include <codecvt>

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

inline int WSTOI(std::wstring s) { return _wtoi(s.c_str()); }

inline double WSTOD(std::wstring s) { return _wtof(s.c_str()); }

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
#include <iostream>

inline std::wstring s2ws(const std::string& utf8)
{
	QString str(utf8.c_str());
	return str.toStdWString();
}

inline std::string ws2s(const std::wstring& utf16)
{
	QString str(QString::fromWCharArray(utf16.c_str()));
	return str.toStdString();
}

inline int WSTOI(std::wstring s) { return atoi(ws2s(s).c_str()); }

inline double WSTOD(std::wstring s) { return atof(ws2s(s).c_str()); }

inline void FIND_FILES(std::wstring m_path_name,
	std::wstring search_ext,
	std::vector<std::wstring> &m_batch_list,
	int &m_cur_batch, std::wstring regex = L"") {
	std::wstring search_path = m_path_name.substr(0, m_path_name.find_last_of(L'/')) + L'/';
	std::wstring regex_min;
	if (regex.find(search_path) != std::string::npos)
		regex_min = regex.substr(search_path.length(), regex.length() - search_path.length());
	else
		regex_min = regex;
	DIR* dir;
	struct dirent *ent;
	if ((dir = opendir(ws2s(search_path).c_str())) != nullptr) {
		int cnt = 0;
		m_batch_list.clear();
		while ((ent = readdir(dir)) != nullptr) {
			std::string file(ent->d_name);
			std::wstring wfile = s2ws(file);
			//check if it contains the string.
			if (wfile.find(search_ext) != std::string::npos &&
				wfile.find(regex_min) != std::string::npos) {
				std::string ss = ent->d_name;
				std::wstring f = s2ws(ss);
				std::wstring name;
				if (f.find(search_path) == std::string::npos)
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
#endif
// END COMPATIBILITY


//! Helper class for reading .oif files. Adapted from FluoRenderer code
class iAOIFReaderHelper
{
protected:
	std::wstring m_id_string;  //!< the path and file name used to read files
	int m_resize_type;         //!< 0: no resizing; 1: padding; 2: resampling
	int m_resample_type;       //!< 0: nearest neighbour; 1: linear
	int m_alignment;           //!< padding alignment

	//! @{ 3D Batch
	bool m_batch;
	std::vector<std::wstring> m_batch_list;
	int m_cur_batch;
	//! @}

	std::wstring m_path_name;
public:
	typedef itk::Image<unsigned short, 2> TiffImgType;
	typedef TiffImgType::Pointer TiffImgPtr;
	typedef itk::Image<unsigned short, 3> ResultImgType;
	typedef ResultImgType::Pointer ResultImgPtr;

	iAOIFReaderHelper();

	void SetFile(std::string &file);
	void SetFile(std::wstring &file);
	void SetTimeId(std::wstring &id);
	std::wstring GetTimeId();
	void Preprocess();
	void SetBatch(bool batch);
	int LoadBatch(int index);
	std::wstring GetCurName(int t, int c);

	std::wstring GetPathName() { return m_path_name; }
	std::wstring GetDataName() { return m_data_name; }
	int GetTimeNum() { return m_time_num; }
	int GetCurTime() { return m_cur_time; }
	int GetChanNum() { return m_chan_num; }
	double GetExcitationWavelength(int chan);
	int GetSliceNum() { return m_slice_num; }
	int GetXSize() { return m_x_size; }
	int GetYSize() { return m_y_size; }
	bool IsSpcInfoValid() { return m_valid_spc; }
	double GetXSpc() { return m_xspc; }
	double GetYSpc() { return m_yspc; }
	double GetZSpc() { return m_zspc; }
	double GetMaxValue() { return m_max_value; }
	double GetScalarScale() { return m_scalar_scale; }
	bool GetBatch() { return m_batch; }
	int GetBatchNum() { return (int)m_batch_list.size(); }
	int GetCurBatch() { return m_cur_batch; }
	void Load();
	ResultImgPtr GetResult(int chanIdx);

private:
	void Read(int t, int c, bool get_max);
	TiffImgPtr ReadTiffImage(const QString & file_name);

	std::vector<ResultImgPtr> m_result;

	std::wstring m_data_name;
	std::wstring m_subdir_name;

	int m_type;	//!< 0-time data in a single file; 1-time data in a file sequence
	typedef std::vector<std::wstring> ChannelInfo;  //!< slices form a channel
	typedef std::vector<ChannelInfo> DatasetInfo;   //!< channels form dataset
	struct TimeDataInfo
	{
		int filenumber;                    //!< if type is 1, file number for time data
		std::wstring filename;             //!< if type is 1, file name for current time data
		std::wstring subdirname;           //!< subdirectory name
		DatasetInfo dataset;               //!< a list of the channels
	};
	std::vector<TimeDataInfo> m_oif_info;  //!< time data form the complete oif dataset
	int m_oif_t;                           //!< current time point in oib info for reading

	int m_time_num;
	int m_cur_time;
	int m_chan_num;
	struct WavelengthInfo
	{
		int chan_num;
		double wavelength;
	};
	std::vector<WavelengthInfo> m_excitation_wavelength_list;
	int m_slice_num;
	int m_x_size;
	int m_y_size;
	bool m_valid_spc;
	double m_xspc;
	double m_yspc;
	double m_zspc;
	double m_max_value;
	double m_scalar_scale;

	static bool oif_sort(const TimeDataInfo& info1, const TimeDataInfo& info2);
	void ReadSingleOif();
	void ReadSequenceOif();
	void ReadTifSequence(std::wstring file_name, int t = 0);
	void ReadOif();
	void ReadOifLine(std::wstring oneline);
	void ReadTiff(char* pbyData, unsigned short *val, int z);

	//axis count
	int axis_num;
	int cur_axis;
	//channel count
	int chan_num;
	int cur_chan;
	//axis info
	std::wstring axis_code;
	std::wstring pix_unit;
	std::wstring max_size;
	std::wstring start_pos;
	std::wstring end_pos;
	std::wstring light_type;

	std::wstring m_time_id; //! time sequence id
};



iAOIFReaderHelper::iAOIFReaderHelper()
{
	m_time_num = 0;
	m_cur_time = -1;
	m_chan_num = 0;
	m_slice_num = 0;
	m_x_size = 0;
	m_y_size = 0;

	m_valid_spc = false;
	m_xspc = 0.0;
	m_yspc = 0.0;
	m_zspc = 0.0;

	m_max_value = 0.0;
	m_scalar_scale = 1.0;

	m_batch = false;
	m_cur_batch = -1;

	m_time_id = L"_T";
	m_type = 0;
	m_oif_t = 0;
}

void iAOIFReaderHelper::SetFile(std::string &file)
{
	if (!file.empty())
	{
		if (!m_path_name.empty())
			m_path_name.clear();
		m_path_name.assign(file.length(), L' ');
		copy(file.begin(), file.end(), m_path_name.begin());

		m_data_name = m_path_name.substr(m_path_name.find_last_of(L"/") + 1);
	}
	m_id_string = m_path_name;
}

void iAOIFReaderHelper::SetFile(std::wstring &file)
{
	m_path_name = file;
	m_data_name = m_path_name.substr(m_path_name.find_last_of(L"/") + 1);
	m_id_string = m_path_name;
}

void iAOIFReaderHelper::Preprocess()
{
	m_type = 0;
	m_oif_info.clear();

	//separate path and name
	int64_t pos = m_path_name.find_last_of(L"/");
	if (pos == -1)
		return;
	std::wstring path = m_path_name.substr(0, pos + 1);
	std::wstring name = m_path_name.substr(pos + 1);
	//extract time sequence string
	int64_t begin = name.find(m_time_id);
	size_t end; // not read anywhere?
	size_t id_len = m_time_id.size();
	if (begin != -1)
	{
		std::wstring t_num;
		size_t j;
		for (j = begin + id_len; j < name.size(); j++)
		{
			wchar_t c = name[j];
			if (iswdigit(c))
				t_num.push_back(c);
			else break;
		}
		if (t_num.size() > 0)
			end = j;
		else
			begin = -1;
	}

	if (begin == -1)
	{
		ReadSingleOif();
	}
	else
	{
		//search time sequence files
		std::vector<std::wstring> list;
		int tmp = 0;
		FIND_FILES(path, L".oif", list, tmp, name.substr(0, begin + id_len + 1));
		for (size_t i = 0; i < list.size(); i++) {
			size_t start_idx = list.at(i).find(m_time_id) + id_len;
			size_t end_idx = list.at(i).find(L".oif");
			size_t size = end_idx - start_idx;
			std::wstring fileno = list.at(i).substr(start_idx, size);
			TimeDataInfo info;
			info.filenumber = WSTOI(fileno);
			info.filename = list.at(i);
			m_oif_info.push_back(info);
		}

		if (m_oif_info.size() > 0)
		{
			m_type = 1;
			std::sort(m_oif_info.begin(), m_oif_info.end(), iAOIFReaderHelper::oif_sort);
			ReadSequenceOif();
		}
		else
		{
			m_oif_info.clear();
			ReadSingleOif();
		}
	}

	ReadOif();

	m_time_num = int(m_oif_info.size());
	if (m_type == 0)
		m_cur_time = 0;
	m_chan_num = m_time_num > 0 ? int(m_oif_info[0].dataset.size()) : 0;
	m_slice_num = m_chan_num > 0 ? int(m_oif_info[0].dataset[0].size()) : 0;
}

bool iAOIFReaderHelper::oif_sort(const TimeDataInfo& info1, const TimeDataInfo& info2)
{
	return info1.filenumber < info2.filenumber;
}

void iAOIFReaderHelper::ReadSingleOif()
{
	m_subdir_name = m_path_name + L".files/";
	std::vector<std::wstring> list;
	int tmp;
	FIND_FILES(m_subdir_name, L".tif", list, tmp);
	//read file sequence
	for (size_t f = 0; f < list.size(); f++)
		ReadTifSequence(list.at(f));
}

void iAOIFReaderHelper::ReadSequenceOif()
{
	for (int i = 0; i < (int)m_oif_info.size(); i++)
	{
		std::wstring path_name = m_oif_info[i].filename;
		m_oif_info[i].subdirname = path_name + L".files/";

		if (path_name == m_path_name)
			m_cur_time = i;

		m_subdir_name = path_name + L".files/";
		std::vector<std::wstring> list;
		FIND_FILES(m_subdir_name, L".tif", list, m_oif_t);
		//read file sequence
		for (size_t f = 0; f < list.size(); f++)
			ReadTifSequence(list.at(f), i);
	}
}

void iAOIFReaderHelper::SetTimeId(std::wstring &id)
{
	m_time_id = id;
}

std::wstring iAOIFReaderHelper::GetTimeId()
{
	return m_time_id;
}

void iAOIFReaderHelper::SetBatch(bool batch)
{
	if (batch)
	{
		//read the directory info
		std::wstring search_path = m_path_name.substr(0, m_path_name.find_last_of(L"/")) + L"/";
		FIND_FILES(search_path, L".oif", m_batch_list, m_cur_batch);
		m_batch = true;
	}
	else
		m_batch = false;
}

int iAOIFReaderHelper::LoadBatch(int index)
{
	int result = -1;
	if (index >= 0 && index < (int)m_batch_list.size())
	{
		m_path_name = m_batch_list[index];
		Preprocess();
		result = index;
		m_cur_batch = result;
	}
	else
		result = -1;

	return result;
}

void iAOIFReaderHelper::ReadTifSequence(std::wstring file_name, int t)
{
	size_t line_size = file_name.size();
	if (file_name.substr(line_size - 3, 3) == L"tif")
	{
		//interpret file_name
		int64_t pos;
		int64_t pos_ = file_name.find_last_of(L'_');
		if (pos_ != -1)
		{
			size_t j;
			std::wstring wstr;
			int num_c = -1;
			int num_z = -1;
			int num_t = -1;
			int num_l = -1;

			//read channel number 'C'
			pos = file_name.find(L'C', pos_ + 1);
			if (pos != -1)
			{
				for (j = pos + 1; j < file_name.size(); j++)
				{
					if (iswdigit(file_name[j]))
						wstr.push_back(file_name[j]);
					else
						break;
				}
				num_c = WSTOI(wstr);
				wstr.clear();
			}
			//read z number 'Z'
			pos = file_name.find(L'Z', pos_ + 1);
			if (pos != -1)
			{
				for (j = pos + 1; j < file_name.size(); j++)
				{
					if (iswdigit(file_name[j]))
						wstr.push_back(file_name[j]);
					else
						break;
				}
				num_z = WSTOI(wstr);
				wstr.clear();
			}
			//read time number 'T'
			pos = file_name.find(L'T', pos_ + 1);
			if (pos != -1)
			{
				for (j = pos + 1; j < file_name.size(); j++)
				{
					if (iswdigit(file_name[j]))
						wstr.push_back(file_name[j]);
					else
						break;
				}
				num_t = WSTOI(wstr);
				wstr.clear();
			}
			//read lambda number 'L'
			pos = file_name.find(L'L', pos_ + 1);
			if (pos != -1)
			{
				for (j = pos + 1; j < file_name.size(); j++)
				{
					if (iswdigit(file_name[j]))
						wstr.push_back(file_name[j]);
					else
						break;
				}
				num_l = WSTOI(wstr);
				wstr.clear();
			}

			//add info to the list
			num_c = num_c == -1 ? 0 : num_c - 1;
			num_t = num_t == -1 ? t : num_t - 1;
			num_z = num_z == -1 ? 1 : num_z;
			if (num_z > 0)
			{
				num_z--;
				//allocate
				if (m_type == 0)
				{
					if (int(m_oif_info.size()) < num_t + 1)
						m_oif_info.resize(num_t + 1);
					if (int(m_oif_info[num_t].dataset.size()) < num_c + 1)
						m_oif_info[num_t].dataset.resize(num_c + 1);
					if (int(m_oif_info[num_t].dataset[num_c].size()) < num_z + 1)
						m_oif_info[num_t].dataset[num_c].resize(num_z + 1);
					//add
					m_oif_info[num_t].dataset[num_c][num_z] = file_name;
				}
				else
				{
					//if (num_t == 0)
					{
						if (int(m_oif_info[num_t].dataset.size()) < num_c + 1)
							m_oif_info[num_t].dataset.resize(num_c + 1);
						if (int(m_oif_info[num_t].dataset[num_c].size()) < num_z + 1)
							m_oif_info[num_t].dataset[num_c].resize(num_z + 1);
						//add
						m_oif_info[num_t].dataset[num_c][num_z] = file_name;
					}
				}
			}
		}
	}
}

void iAOIFReaderHelper::ReadOif()
{
	//read oif file
#ifdef _WIN32
	ifstream is(m_path_name.c_str());
#else
	ifstream is(ws2s(m_path_name).c_str());
#endif
	std::wstring oneline;
	if (is.is_open())
	{
		//reset
		m_excitation_wavelength_list.clear();
		m_x_size = 0;
		m_y_size = 0;
		m_xspc = 0.0;
		m_yspc = 0.0;
		m_zspc = 0.0;
		//axis count
		axis_num = -1;
		cur_axis = -1;
		//channel count
		chan_num = -1;
		cur_chan = -1;
		//axis info
		axis_code.clear();
		pix_unit.clear();
		max_size.clear();
		start_pos.clear();
		end_pos.clear();

		while (!is.eof())
		{
			wchar_t c;
			is.read(((char*)(&c)), 1);
			if (!is.eof())
				is.read(((char*)(&c)) + 1, 1);
			if (c != L'\x0D' && c != L'\n')
				oneline.push_back(c);
			else
			{
				if (!oneline.empty())
					ReadOifLine(oneline);
				oneline.clear();
			}
		}
		is.close();
	}

	if (m_xspc > 0.0 && m_xspc<100.0 &&
		m_yspc>0.0 && m_yspc < 100.0)
	{
		m_valid_spc = true;
		if (m_zspc <= 0.0 || m_zspc > 100.0)
			m_zspc = std::max(m_xspc, m_yspc);
	}
	else
	{
		m_valid_spc = false;
		m_xspc = 1.0;
		m_yspc = 1.0;
		m_zspc = 1.0;
	}
}

void iAOIFReaderHelper::ReadOifLine(std::wstring oneline)
{
	//process
	if (oneline.substr(0, 6) == L"[Axis ")
	{
		axis_num++;
	}
	else
	{
		if (axis_num > -1)
		{
			size_t pos = oneline.find(L'=');
			std::wstring str1 = oneline.substr(0, oneline.find_last_not_of(L' ', pos));
			std::wstring str2 = oneline.substr(oneline.find_first_not_of(L' ', pos + 1));

			if (str1 == L"AxisCode")
			{
				if (cur_axis != axis_num)
				{
					cur_axis = axis_num;
					axis_code.clear();
					pix_unit.clear();
					max_size.clear();
					start_pos.clear();
					end_pos.clear();
				}
				axis_code = str2;
			}
			else if (str1 == L"PixUnit")
			{
				if (cur_axis != axis_num)
				{
					cur_axis = axis_num;
					axis_code.clear();
					pix_unit.clear();
					max_size.clear();
					start_pos.clear();
					end_pos.clear();
				}
				pix_unit = str2;
			}
			else if (str1 == L"MaxSize")
			{
				if (cur_axis != axis_num)
				{
					cur_axis = axis_num;
					axis_code.clear();
					pix_unit.clear();
					max_size.clear();
					start_pos.clear();
					end_pos.clear();
				}
				max_size = str2;
			}
			else if (str1 == L"StartPosition")
			{
				if (cur_axis != axis_num)
				{
					cur_axis = axis_num;
					axis_code.clear();
					pix_unit.clear();
					max_size.clear();
					start_pos.clear();
					end_pos.clear();
				}
				start_pos = str2;
			}
			else if (str1 == L"EndPosition")
			{
				if (cur_axis != axis_num)
				{
					cur_axis = axis_num;
					axis_code.clear();
					pix_unit.clear();
					max_size.clear();
					start_pos.clear();
					end_pos.clear();
				}
				end_pos = str2;
			}
		}
	}
	if (oneline.substr(0, 9) == L"[Channel ")
	{
		light_type.clear();
		chan_num++;
	}
	else
	{
		if (chan_num > -1)
		{
			size_t pos = oneline.find(L'=');
			std::wstring str1 = oneline.substr(0, oneline.find_last_not_of(L' ', pos));
			std::wstring str2 = oneline.substr(oneline.find_first_not_of(L' ', pos + 1));
			std::wstring str3 = L"Transmitted Light";
			if (str1 == L"LightType") {
				light_type = str2;
				if (light_type.find(str3) != std::wstring::npos) {
					for (int i = m_excitation_wavelength_list.size() - 1; i >= 0; i--) {
						if (m_excitation_wavelength_list.at(i).chan_num == cur_chan) {
							m_excitation_wavelength_list.at(i).wavelength = -1;
							break;
						}
					}
				}
			}
			else if (str1 == L"ExcitationWavelength")
			{
				if (cur_chan != chan_num)
				{
					cur_chan = chan_num;
					WavelengthInfo info;
					info.chan_num = cur_chan;
					info.wavelength = WSTOD(str2);
					if (light_type == L"Transmitted Light")
						info.wavelength = -1;
					m_excitation_wavelength_list.push_back(info);
				}
			}
		}
	}

	//axis
	if (!axis_code.empty() &&
		!pix_unit.empty() &&
		!max_size.empty() &&
		!start_pos.empty() &&
		!end_pos.empty())
	{
		//calculate
		double spc = 0.0;
		double dmax = WSTOD(max_size);
		if (dmax > 0.0)
			spc = fabs((WSTOD(end_pos) -
				WSTOD(start_pos))) /
			dmax;
		if ((int64_t)pix_unit.find(L"nm") != -1)
			spc /= 1000.0;
		if ((int64_t)axis_code.find(L"X") != -1)
		{
			m_x_size = WSTOI(max_size);
			m_xspc = spc;
		}
		else if ((int64_t)axis_code.find(L"Y") != -1)
		{
			m_y_size = WSTOI(max_size);
			m_yspc = spc;
		}
		else if ((int64_t)axis_code.find(L"Z") != -1)
			m_zspc = spc;

		axis_code.clear();
		pix_unit.clear();
		max_size.clear();
		start_pos.clear();
		end_pos.clear();
	}
}

double iAOIFReaderHelper::GetExcitationWavelength(int chan)
{
	for (int i = 0; i < (int)m_excitation_wavelength_list.size(); i++)
	{
		if (m_excitation_wavelength_list[i].chan_num == chan)
			return m_excitation_wavelength_list[i].wavelength;
	}
	return 0.0;
}

std::wstring iAOIFReaderHelper::GetCurName(int t, int c)
{
	return m_oif_info[t].dataset[c][0];
}

template<typename T>
typename iAOIFReaderHelper::TiffImgPtr read_image_template(QString const & f, T)
{
	typedef itk::ImageFileReader<iAOIFReaderHelper::TiffImgType> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName(getLocalEncodingFileName(f));
	reader->Update();
	return reader->GetOutput();
}

iAOIFReaderHelper::TiffImgPtr iAOIFReaderHelper::ReadTiffImage(QString const & file_name)
{
	itk::ImageIOBase::Pointer imageIO;
	QString errorMsg;
	try
	{
		imageIO = itk::ImageIOFactory::CreateImageIO(getLocalEncodingFileName(file_name).c_str(), itk::ImageIOFactory::ReadMode);
	}
	catch (itk::ExceptionObject& e)
	{
		imageIO = 0;
		errorMsg = e.what();
	}

	if (!imageIO)
	{
		DEBUG_LOG(QString("OIF Reader: Could not open file %1, aborting loading (error message: %2).")
			.arg(file_name)
			.arg(errorMsg));
		return iAOIFReaderHelper::TiffImgPtr();
	}

	try
	{
		imageIO->SetFileName(getLocalEncodingFileName(file_name).c_str());
		imageIO->ReadImageInformation();
		auto pixelType = imageIO->GetComponentType();
		switch (pixelType) // This filter handles all types
		{
		case itk::ImageIOBase::UCHAR:	return read_image_template(file_name, static_cast<unsigned char>(0));  break;
		case itk::ImageIOBase::CHAR:	return read_image_template(file_name, static_cast<char>(0));  break;
		case itk::ImageIOBase::USHORT:	return read_image_template(file_name, static_cast<unsigned short>(0));  break;
		case itk::ImageIOBase::SHORT:	return read_image_template(file_name, static_cast<short>(0));  break;
		case itk::ImageIOBase::UINT:	return read_image_template(file_name, static_cast<unsigned int>(0));  break;
		case itk::ImageIOBase::INT:		return read_image_template(file_name, static_cast<int>(0));  break;
		case itk::ImageIOBase::ULONG:	return read_image_template(file_name, static_cast<unsigned long>(0));  break;
		case itk::ImageIOBase::LONG:	return read_image_template(file_name, static_cast<long>(0));  break;
#if ITK_VERSION_MAJOR > 4 || (ITK_VERSION_MAJOR == 4 && ITK_VERSION_MINOR > 12)
		case itk::ImageIOBase::ULONGLONG:return read_image_template(file_name, static_cast<unsigned long long>(0));  break;
		case itk::ImageIOBase::LONGLONG:return read_image_template(file_name, static_cast<long long>(0));  break;
#endif
		case itk::ImageIOBase::FLOAT:	return read_image_template(file_name, static_cast<float>(0));  break;
		case itk::ImageIOBase::DOUBLE:	return read_image_template(file_name, static_cast<double>(0));  break;
		case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
		default:
			DEBUG_LOG("OIF Reader: Unknown component type");
			return iAOIFReaderHelper::TiffImgPtr();
		}
	}
	catch (itk::ExceptionObject &excep)
	{
		DEBUG_LOG(QString("OIF Reader: Exception %1").arg(excep.what()));
	}
	return iAOIFReaderHelper::TiffImgPtr();
}

void iAOIFReaderHelper::Read(int t, int c, bool /*get_max*/)
{
	ResultImgType::IndexType origin;
	origin.Fill(0);
	ResultImgType::SizeType size;
	size[0] = m_x_size;
	size[1] = m_y_size;
	size[2] = m_oif_info[0].dataset[0].size();
	double spacing[3];
	spacing[0] = m_xspc / 1000;
	spacing[1] = m_yspc / 1000;
	spacing[2] = m_zspc / 1000;
	ResultImgType::RegionType region(origin, size);
	m_result.push_back(iAOIFReaderHelper::ResultImgType::New());
	int curIdx = m_result.size() - 1;
	m_result[curIdx]->SetRegions(region);
	m_result[curIdx]->SetSpacing(spacing);
	m_result[curIdx]->Allocate();
	if (t >= 0 && t < m_time_num &&
		c >= 0 && c < m_chan_num &&
		m_slice_num>0 &&
		m_x_size>0 &&
		m_y_size > 0)
	{
		//read the channel
		ChannelInfo *cinfo = &m_oif_info[t].dataset[c];
		for (int i = 0; i<int(cinfo->size()); i++)
		{
			std::wstring file_name = (*cinfo)[i];
			TiffImgPtr tiff = ReadTiffImage(QString::fromStdWString(file_name));

			for (int x = 0; x < m_x_size; ++x)
			{
				for (int y = 0; y < m_y_size; ++y)
				{
					TiffImgType::IndexType sourceIdx;
					sourceIdx[0] = x;
					sourceIdx[1] = y;
					ResultImgType::IndexType destIdx;
					destIdx[0] = x;
					destIdx[1] = y;
					destIdx[2] = i;
					m_result[curIdx]->SetPixel(destIdx, tiff->GetPixel(sourceIdx));
				}
			}
		}
	}

	if (m_max_value > 0.0)
		m_scalar_scale = 65535.0 / m_max_value;

	m_cur_time = t;
}

void iAOIFReaderHelper::Load()
{
	iAOIFReaderHelper::ResultImgPtr result;
	int chanNum = GetChanNum();
	for (int i = 0; i < chanNum; i++)
	{
		//		bool valid_spc = IsSpcInfoValid();
		Read(GetCurTime(), i, true);
	}
}

iAOIFReaderHelper::ResultImgPtr iAOIFReaderHelper::GetResult(int chanIdx)
{
	return m_result[chanIdx];
}






void readOIF(QString const & filename, iAConnector* con, int channel,
	std::vector<vtkSmartPointer<vtkImageData> > * volumes)
{
	iAOIFReaderHelper reader;
	auto wfn = filename.toStdWString();
	reader.SetFile(wfn);
	std::wstring timeId(L"_T");
	reader.SetTimeId(timeId);
	reader.Preprocess();
	reader.Load();
	if (volumes)
	{
		con->setImage(reader.GetResult(0));
		con->modified();
		for (int i = 0; i < reader.GetChanNum(); ++i)
		{
			vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
			iAConnector con2;
			con2.setImage(reader.GetResult(i));
			image->DeepCopy(con2.vtkImage());
			volumes->push_back(image);
		}
	}
	else if (channel >= 0 && channel < reader.GetChanNum())
	{
		con->setImage(reader.GetResult(channel));
		con->modified();
	}
	else
	{
		DEBUG_LOG("OIF reader: Neither channel number nor volume vector given!");
	}
}
