/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#ifdef _WIN32
#include <intrin.h>
typedef unsigned __int32  uint32_t;

#else
#include <cstdint>
#endif

class iACPUID {
	uint32_t regs[4];

public:
	explicit iACPUID(unsigned i)
	{
#ifdef _WIN32
		__cpuid((int *)regs, (int)i);

#else
		asm volatile
		("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		 : "a" (i), "c" (0));
		// ECX is set to zero for CPUID function 4
#endif
	}

	const uint32_t &EAX() const {return regs[0];}
	const uint32_t &EBX() const {return regs[1];}
	const uint32_t &ECX() const {return regs[2];}
	const uint32_t &EDX() const {return regs[3];}


	static QString cpuVendor()
	{
		QString vendor;
		iACPUID cpuID(0);
		vendor += QString::fromLocal8Bit((const char *)&cpuID.EBX(), 4);
		vendor += QString::fromLocal8Bit((const char *)&cpuID.EDX(), 4);
		vendor += QString::fromLocal8Bit((const char *)&cpuID.ECX(), 4);

		return vendor;
	}

	static QString cpuBrand()
	{
		iACPUID cpuID(0x80000000);
		if (cpuID.EAX() >= 0x80000004)
		{
			QString brand;
			cpuID = iACPUID(0x80000002);
			brand += QString::fromLatin1((const char *)&cpuID.EAX(), 16);
			cpuID = iACPUID(0x80000003);
			brand += QString::fromLatin1((const char *)&cpuID.EAX(), 16);
			cpuID = iACPUID(0x80000004);
			brand += QString::fromLatin1((const char *)&cpuID.EAX(), 16);
			return brand;
		}
		return QString();
	}
};
