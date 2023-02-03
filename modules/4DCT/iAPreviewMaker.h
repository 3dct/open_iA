// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QString>

class iAPreviewMaker
{
public:
	static void		makeUsingType( QString fileName, QString thumbFileName );

private:
	template<typename TPixelType>
	static void		makeUsingType( QString filename, QString thumbFileName );
};
