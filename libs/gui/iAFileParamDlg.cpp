/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include <iAFileParamDlg.h>

#include <iAParameterDlg.h>

iAFileParamDlg* iAFileParamDlg::get(QString const& ioName)
{
	if (m_dialogs.contains(ioName))
	{
		return m_dialogs[ioName].get();
	}
	static auto defaultDialog = std::make_shared<iAFileParamDlg>();
	return defaultDialog.get();
}

//! register a dialog for a given I/O name:
void iAFileParamDlg::add(QString const& ioName, std::shared_ptr<iAFileParamDlg> dlg)
{
	m_dialogs.insert(ioName, dlg);
}

void iAFileParamDlg::setupDefaultFileParamDlgs()
{

}

//! default method
bool iAFileParamDlg::askForParameters(QWidget* parent, iAAttributes const& parameters, QMap<QString, QVariant>& values) const
{
	iAAttributes dlgParams;
	for (auto param : parameters)
	{
		QSharedPointer<iAAttributeDescriptor> p(param->clone());
		if (p->valueType() == iAValueType::Categorical)
		{
			QStringList comboValues = p->defaultValue().toStringList();
			QString storedValue = values[p->name()].toString();
			selectOption(comboValues, storedValue);
			p->setDefaultValue(comboValues);
		}
		else
		{
			p->setDefaultValue(values[p->name()]);
		}
		dlgParams.push_back(p);
	}
	iAParameterDlg dlg(parent, "Parameters", dlgParams);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	values = dlg.parameterValues();
	return true;
}

QMap<QString, std::shared_ptr<iAFileParamDlg>> iAFileParamDlg::m_dialogs;

