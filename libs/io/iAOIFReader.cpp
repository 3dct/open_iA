// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAOIFReader.h"

#include "iAConnector.h"
#include "iALog.h"
#include "iAFileUtils.h"

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
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <ole2.h>
#include <ctime>
#include <sys/types.h>
#include <ctype.h>
#include <direct.h>
#include <codecvt>
/*
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
*/

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
	//size_t end; // not read anywhere?
	size_t id_len = m_time_id.size();
	if (begin != -1)
	{
		std::wstring t_num;
		size_t j;
		for (j = begin + id_len; j < name.size(); j++)
		{
			wchar_t c = name[j];
			if (iswdigit(c))
			{
				t_num.push_back(c);
			}
			else
			{
				break;
			}
		}
		if (t_num.size() > 0)
		{
			//end = j;
		}
		else
		{
			begin = -1;
		}
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
		for (size_t i = 0; i < list.size(); i++)
		{
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
	{
		m_cur_time = 0;
	}
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
	{
		ReadTifSequence(list.at(f));
	}
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
			//int num_l = -1;

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
				//num_l = WSTOI(wstr);
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
	std::ifstream is(m_path_name.c_str());
#else
	std::ifstream is(ws2s(m_path_name).c_str());
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
		LOG(lvlError, QString("OIF Reader: Could not open file %1, aborting loading (error message: %2).")
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
		case iAITKIO::ScalarType::UCHAR:     return read_image_template(file_name, static_cast<unsigned char>(0));      break;
		case iAITKIO::ScalarType::CHAR:      return read_image_template(file_name, static_cast<char>(0));               break;
		case iAITKIO::ScalarType::USHORT:    return read_image_template(file_name, static_cast<unsigned short>(0));     break;
		case iAITKIO::ScalarType::SHORT:     return read_image_template(file_name, static_cast<short>(0));              break;
		case iAITKIO::ScalarType::UINT:      return read_image_template(file_name, static_cast<unsigned int>(0));       break;
		case iAITKIO::ScalarType::INT:       return read_image_template(file_name, static_cast<int>(0));                break;
		case iAITKIO::ScalarType::ULONG:     return read_image_template(file_name, static_cast<unsigned long>(0));      break;
		case iAITKIO::ScalarType::LONG:      return read_image_template(file_name, static_cast<long>(0));               break;
		case iAITKIO::ScalarType::ULONGLONG: return read_image_template(file_name, static_cast<unsigned long long>(0)); break;
		case iAITKIO::ScalarType::LONGLONG:  return read_image_template(file_name, static_cast<long long>(0));          break;
		case iAITKIO::ScalarType::FLOAT:     return read_image_template(file_name, static_cast<float>(0));              break;
		case iAITKIO::ScalarType::DOUBLE:    return read_image_template(file_name, static_cast<double>(0));             break;
		case iAITKIO::ScalarType::UNKNOWNCOMPONENTTYPE:
		default:
			LOG(lvlError, "OIF Reader: Unknown component type");
			return iAOIFReaderHelper::TiffImgPtr();
		}
	}
	catch (itk::ExceptionObject &excep)
	{
		LOG(lvlError, QString("OIF Reader: Exception %1").arg(excep.what()));
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



void readOIF(QString const& filename, iAConnector* con, int channel,
	std::vector<vtkSmartPointer<vtkImageData>>* volumes)
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
		LOG(lvlError, "OIF reader: Neither channel number nor volume vector given!");
	}
}
