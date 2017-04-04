#pragma once

#include "iANameMapper.h"

#include <QStringList>

class iAListNameMapper : public iANameMapper
{
public:
	iAListNameMapper(QStringList const & names) :
		m_names(names)
	{}
	virtual QString GetName(int idx) const
	{
		return m_names[idx];
	}

	virtual int GetIdx(QString const & name, bool & ok) const
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

	virtual int size() const
	{
		return m_names.size();
	}
private:
	QStringList m_names;
};
