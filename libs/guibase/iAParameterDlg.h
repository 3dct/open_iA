// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAAttributes.h"
#include "iAValueType.h"

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include <memory>

class iAAttributeDescriptor;
class iAMainWindow;
class iAMdiChild;

class QDialogButtonBox;
class QGridLayout;
class QString;
class QWidget;

//! Dialog asking the user for some given parameters.
class iAguibase_API iAParameterDlg : public QDialog
{
	Q_OBJECT
public:
	//! Create dialog with the given parameters.
	//! @param parent the parent widget
	//! @param title  the dialog title
	//! @param parameters list of parameters (name, type, value, range, ...)
	//! @param descr an optional description text, displayed on top of the dialog
	iAParameterDlg(QWidget* parent, QString const& title, iAAttributes const & parameters, QString const& descr = QString());
	QVariantMap parameterValues() const;  // make const &, cache
	//QVariant value(QString const& key) const;
	void showROI();
	int exec() override;
	void setSourceMdi(iAMdiChild* child, iAMainWindow* mainWnd);
	QWidget* paramWidget(QString const & key);
	void setOKEnabled(bool enabled);
	void setValue(QString const& key, QVariant const& value);
	QWidget* mainWidget();
	QGridLayout* mainLayout();

private slots:
	void updatedROI(QVariant value);
	void sourceChildClosed();
	void selectFilter();

private:
	QWidget * m_container, * m_mainWidget;
	int m_roi[6];
	QVector<qsizetype> m_filterWithParameters;
	iAMdiChild * m_sourceMdiChild;
	iAMainWindow * m_mainWnd;
	bool m_sourceMdiChildClosed;
	QVector<QWidget*> m_widgetList;
	iAAttributes m_parameters;
	QDialogButtonBox* m_buttonBox;
	QGridLayout * m_mainLayout;

	void updateROIPart(QString const& partName, QVariant value);
};

#include "iAMainWindow.h"

//! Generic editing of settings, and applying them, if dialog was not cancelled
template <typename Obj>
bool editSettingsDialog(iAAttributes const& attr, QVariantMap const& curVal, QString name, Obj& obj, void (Obj::* applyFun)(QVariantMap const&))
{
	iAAttributes dlgAttr = combineAttributesWithValues(attr, curVal);
	iAParameterDlg dlg(iAMainWindow::get(), name, dlgAttr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	(obj.*applyFun)(dlg.parameterValues());
	return true;
}
