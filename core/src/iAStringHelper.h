#pragma once

class QString;
class QStringList;

#include "open_iA_Core_export.h"

open_iA_Core_API QStringList SplitPossiblyQuotedString(QString const & additionalArguments);

bool Str2Vec3D(QString const & str, double vec[3]);
QString Vec3D2String(double* vec);
