#pragma once
#include <QFile>

class QString; 

class Loader
{
public:
	Loader();
	~Loader();

	bool loadCSV(const QString &Fname);

	std::vector<double> &getFrequencies() {
		return this->m_frequencies;
	}

	std::vector<double> &getGreyValues() {
		return m_greyValues; 
	}

private:
	std::vector<double> m_frequencies; 
	std::vector<double> m_greyValues; 

};

