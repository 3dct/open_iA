// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class iAElementSelectionListener
{
public:
	virtual ~iAElementSelectionListener();
	virtual void ElementEnter(int elementIdx) =0;
	virtual void ElementLeave(int elementIdx) =0;
};
