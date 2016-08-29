#pragma once

class QString;
class QStringList;

#include "open_iA_Core_export.h"

open_iA_Core_API QStringList SplitPossiblyQuotedString(QString const & additionalArguments);
