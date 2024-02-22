// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAbortListener.h"

iASimpleAbortListener::iASimpleAbortListener() : m_aborted(false)
{
}

void iASimpleAbortListener::abort()
{
	m_aborted = true;
}

bool iASimpleAbortListener::isAborted() const
{
	return m_aborted;
}
