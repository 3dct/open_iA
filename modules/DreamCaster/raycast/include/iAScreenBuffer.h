// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADreamCasterCommon.h"

//! ScreenBuffer, representing screen surface. Contains image's pixel buffer and size data.
class iAScreenBuffer
{
public:
	iAScreenBuffer( int width, int height );
	~iAScreenBuffer();
	//! Get pixel buffer.
	unsigned int* buffer() { return m_buffer; }
	//! Clear pixel buffer with black color.
	void clear();
private:
	//! Get screen's width.
	int width() const { return m_width; }
	//! Get screen's height.
	int height() const { return m_height; }

	unsigned int* m_buffer;
	int m_width, m_height;
	int m_bufSize;
};
