#include "iAStringHelper.h"

#include "iASimpleTester.h"

#include <QStringList>

std::ostream& operator<<(std::ostream& o, const QString & s)
{
	o << s.toStdString();
	return o;
}

BEGIN_TEST
	// special case: empty quote at beginning
	QStringList split = SplitPossiblyQuotedString("\"\" a b");
	TestEqual(split.size(), 2);
	TestEqual(split.at(0), QString("a"));
	TestEqual(split.at(1), QString("b"));
	// special case: no space between two quoted strings:
	QStringList split2 = SplitPossiblyQuotedString("\"a\"\"b\"");
	TestEqual(split2.size(), 2);
	TestEqual(split2.at(0), QString("a"));
	TestEqual(split2.at(1), QString("b"));

	// special case: empty quote at end
	QStringList split3 = SplitPossiblyQuotedString("a b \"\"");
	TestEqual(split3.size(), 2);
	TestEqual(split3.at(0), QString("a"));
	TestEqual(split3.at(1), QString("b"));

	// special case: space at end of quoted string:
	QStringList split4 = SplitPossiblyQuotedString("a \"b     \"");
	TestEqual(split4.size(), 2);
	TestEqual(split4.at(0), QString("a"));
	TestEqual(split4.at(1), QString("b     "));

	// "normal" case
	QStringList split5 = SplitPossiblyQuotedString("a \"b c\" d");
	TestEqual(split5.size(), 3);
	TestEqual(split5.at(0), QString("a"));
	TestEqual(split5.at(1), QString("b c"));
	TestEqual(split5.at(2), QString("d"));
END_TEST