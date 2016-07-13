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
#pragma once

#ifdef _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;

#else
#include <stdint.h>
#endif

namespace CPUID {
    
class CPUID {
    uint32_t regs[4];
    
public:
    explicit CPUID(unsigned i)
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
};

inline QString GetCPUVendor()
{
    QString vendor;
    CPUID cpuID(0);
    vendor += QString::fromLocal8Bit( (const char *)&cpuID.EBX(), 4 );
    vendor += QString::fromLocal8Bit( (const char *)&cpuID.EDX(), 4 );
    vendor += QString::fromLocal8Bit( (const char *)&cpuID.ECX(), 4 );
    
    return vendor;
}
    
inline QString GetCPUBrand()
{
    CPUID cpuID(0x80000000);
    if (cpuID.EAX() >= 0x80000004)
    {
        QString brand;
        cpuID = CPUID(0x80000002);
        brand += QString::fromLatin1( (const char *)&cpuID.EAX(), 16 );
        cpuID = CPUID(0x80000003);
        brand += QString::fromLatin1( (const char *)&cpuID.EAX(), 16 );
        cpuID = CPUID(0x80000004);
        brand += QString::fromLatin1( (const char *)&cpuID.EAX(), 16 );
        return brand;
    }
    return QString();
}
    
} //namespace CPUID
