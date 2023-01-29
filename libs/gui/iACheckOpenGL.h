// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class QString;

//! Check whether the available OpenGL version is sufficient
//! to render things properly on this machine with Qt / VTK
bool checkOpenGLVersion(QString & msg);
