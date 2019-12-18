#pragma once
#include <QFile>

class QString; 

//namespace histExp {
//	class HistData 
//	{
//	public: 
//		HistData() {
//			m_data_thres.reserve(1000);
//			m_data_freq.reserve(1000);
//
//		}
//
//		const std::vector<double>& getThresholds() const {
//			return m_data_thres; 
//		};
//		const std::vector<double>& getFrequencies() const {
//			return m_data_freq; 
//		
//		};
//	
//		void insertElem(double thres, double freq) {
//			m_data_freq.push_back(freq);
//			m_data_thres.push_back(thres); 
//		
//		};
//
//	private:
//		std::vector<double> m_data_thres; 
//		std::vector<double> m_data_freq; 
//	};
//};


class Loader
{
public:
	Loader();
	~Loader();

	bool loadCSV(const QString &Fname);

	const std::vector<double> &getFrequencies() {
		return this->m_frequencies;
	}

	const std::vector<double> &getGreyValues() {
		return m_greyValues; 
	}

private:
	std::vector<double> m_frequencies; 
	std::vector<double> m_greyValues; 

};

