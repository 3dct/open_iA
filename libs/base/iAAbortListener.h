// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

class iAbase_API iAAbortListener
{
public:
	virtual void abort() =0;
};

//! a simple implementation of an iAAbortListener -
//! it holds a boolean flag that is set by the abort method.
//! Users can determine whether abort was called through the isAborted method
//! Implementation can be found in iAJobListView.cpp
class iAbase_API iASimpleAbortListener : public iAAbortListener
{
public:
	iASimpleAbortListener();
	void abort() override;
	bool isAborted() const;
private:
	bool m_aborted;
};
