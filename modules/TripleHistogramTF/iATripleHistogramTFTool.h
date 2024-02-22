// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iATool.h"

class dlg_tf_2mod;
class dlg_tf_3mod;

class iATripleHistogramTFTool : public iATool
{
public:
	iATripleHistogramTFTool(iAMainWindow* mainWnd, iAMdiChild* child);
	void start2TF();
	void start3TF();
private:
	dlg_tf_2mod *m_tf_2mod;
	dlg_tf_3mod *m_tf_3mod;
};
