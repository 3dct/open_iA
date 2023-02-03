// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAttributes.h>
#include <iAFileIO.h>

#include <QMap>
#include <QString>
#include <QVariant>

#include <memory>

class QWidget;

//! provides a dialog for reading parameters, currently for a given file I/O type
//! ToDo: check whether it makes sense to use this or something similar for similar purposes,
//! e.g. for asking for filter parameters?
class iAFileParamDlg
{
public:
	virtual ~iAFileParamDlg();
	//! default implementation for asking file parameters
	//! The value contained in the values argument will be used as default values for the parameter,
	//! overwriting the default values provided in the parameters argument
	virtual bool askForParameters(QWidget* parent, iAAttributes const& parameters, QString const& ioName, QVariantMap& values, QString const & fileName) const;

	//! factory method, creating a parameter dialog for the given file I/O name:
	static iAFileParamDlg* get(QString const & ioName);

	//! register a dialog for a given I/O name:
	static void add(QString const& ioName, std::shared_ptr<iAFileParamDlg> dlg);

	//! set up a few custom file parameter dialogs
	static void setupDefaultFileParamDlgs();

	//! Convenience function for retrieving parameters for loading/saving a file.
	//! Note: in contrast to askForParameters, result is an out parameter only; the values in there will not be used as default
	static bool getParameters(QWidget* parent, iAFileIO const * io, iAFileIO::Operation op, QString const& fileName, QVariantMap& result);

private:
	static QMap<QString, std::shared_ptr<iAFileParamDlg>> m_dialogs;
};
