// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iANameMapper.h"
#include "iabase_export.h"

#include <QStringList>

//! Maps numbers to names via a given list.
class iAbase_API iAListNameMapper : public iANameMapper
{
public:
	iAListNameMapper(QStringList const & names) :
		m_names(names)
	{}
	QString name(int idx) const override
	{
		return m_names[idx];
	}

	int GetIdx(QString const & name, bool & ok) const override
	{
		for (int i = 0; i < m_names.size(); ++i)
		{
			if (m_names[i] == name)
			{
				ok = true;
				return i;
			}
		}
		ok = false;
		return -1;
	}

	int size() const override
	{
		return static_cast<int>(m_names.size());
	}
private:
	QStringList m_names;
};
