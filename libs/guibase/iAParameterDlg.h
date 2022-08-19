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
#pragma once

#include "iAguibase_export.h"

#include "iAValueType.h"

#include <QDialog>
#include <QMap>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>
#include <QVector>

class iAAttributeDescriptor;
class iAMainWindow;
class iAMdiChild;

class QDialogButtonBox;
class QString;
class QWidget;

//! Dialog asking the user for some given parameters.
class iAguibase_API iAParameterDlg : public QDialog
{
	Q_OBJECT
public:
	using ParamListT = QVector<QSharedPointer<iAAttributeDescriptor>>;
	//! Create dialog with the given parameters.
	//! @param parent the parent widget
	//! @param title  the dialog title
	//! @param parameters list of parameters (name, type, value, range, ...)
	//! @param descr an optional description text, displayed on top of the dialog
	iAParameterDlg(QWidget* parent, QString const& title, ParamListT parameters, QString const& descr = QString());
	QVariantMap parameterValues() const;  // make const &, cache
	//QVariant value(QString const& key) const;
	void showROI();
	int exec() override;
	void setSourceMdi(iAMdiChild* child, iAMainWindow* mainWnd);
	QWidget* paramWidget(QString const & key);
	void setOKEnabled(bool enabled);
	void setValue(QString const & key, QVariant const & value);

private slots:
	void updatedROI(QVariant value);
	void sourceChildClosed();
	void selectFilter();

private:
	QWidget * m_container;
	int m_roi[6];
	QVector<int> m_filterWithParameters;
	iAMdiChild * m_sourceMdiChild;
	iAMainWindow * m_mainWnd;
	bool m_sourceMdiChildClosed;
	QVector<QWidget*> m_widgetList;
	ParamListT m_parameters;
	QDialogButtonBox* m_buttonBox;

	void updateROIPart(QString const& partName, QVariant value);
};

iAguibase_API void addParameter(iAParameterDlg::ParamListT& params, QString const& name, iAValueType valueType,
	QVariant defaultValue = 0.0, double min = std::numeric_limits<double>::lowest(),
	double max = std::numeric_limits<double>::max());