// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#if (not defined(__arm__) && not defined(__aarch64__))
#ifdef _WIN32
#include <intrin.h>
using uint32_t = unsigned __int32;

#else
#include <cstdint>
#endif
#endif

//! Retrieves and holds information on the CPU the program runs on.
class iACPUID
{
#if (not defined(__arm__) && not defined(__aarch64__))
	uint32_t regs[4];
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

#endif

public:

	static QString cpuVendor()
	{
#if (defined(__arm__) || defined(__aarch64__))
			return "ARM";
#else
			QString vendor;
			iACPUID cpuID(0);
			vendor += QString::fromLocal8Bit((const char *)&cpuID.EBX(), 4);
			vendor += QString::fromLocal8Bit((const char *)&cpuID.EDX(), 4);
			vendor += QString::fromLocal8Bit((const char *)&cpuID.ECX(), 4);
			return vendor;
#endif
	}

	static QString cpuBrand()
	{
#if (defined(__arm__) || defined(__aarch64__))
		return "Unknown";
#else
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
#endif
	}
};
