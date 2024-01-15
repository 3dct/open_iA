// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QString>

//! Reads a file containing key-value pairs, separated by colon (:);
//! with one pair per line, and optional end-line comments (everything after the first '%' character in the line is ignored).
//! Keys and values are trimmed (i.e. leading and trailing whitespace is removed)
//! @param fileName the name of the file to read
//! @return a map of key-value pairs
QMap<QString, QString> readSettingsFile(QString const& fileName);
