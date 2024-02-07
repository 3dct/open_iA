// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../include/iADreamCasterCommon.h"
#include "../include/iAScreenBuffer.h"

#include <algorithm>    // for std::fill

//namespace Raytracer {

iAScreenBuffer::iAScreenBuffer(int width, int height) :
	m_width(width),
	m_height(height),
	m_bufSize(m_width* m_height)
{
	m_buffer = new unsigned int[m_bufSize];
	clear();
}

iAScreenBuffer::~iAScreenBuffer()
{
	delete[] m_buffer;
}

void iAScreenBuffer::clear()
{
	std::fill(m_buffer, m_buffer + m_bufSize, 0);
}

//}; // namespace Raytracer
