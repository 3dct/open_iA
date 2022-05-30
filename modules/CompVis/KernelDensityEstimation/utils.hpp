#ifndef KDE_UTILS_HPP
#define KDE_UTILS_HPP

#include <KernelDensityEstimation/KernelDensityEstimation>

namespace utils
{
    /**
     * \class GaussLogPost
     *
     * \brief A class for computing pdf of a multi-variate Gaussian distribution
     *
     * \tparam _realScalarType real floating point type
     *
     * This class has two functions. The first is to compute the log-pdf of a
     * multi-variate Gaussian distribution. The sencond is to generate samples
     * from a multi-variate Gaussian distribution.
     */
    template<typename _realScalarType>
    class GaussLogPost
    {
    public:
        typedef _realScalarType realScalarType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,Eigen::Dynamic> realMatrixType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,1> realVectorType;
        typedef Eigen::LLT<realMatrixType> LLTType;
        typedef typename realVectorType::Index indexType;

        /**
         * \brief A constructor that sets up the class for genrating samples
         * \param mu mean
         * \param sigmaInv inverse of the covariance matrix
         * \param seed seed for the random number generator
         */
        GaussLogPost(realVectorType const& mu,realMatrixType const& sigmaInv,unsigned long seed)
        :mMu(mu),mSigmaInv(sigmaInv),mGen(seed)
        {
            assert(mMu.rows() == mSigmaInv.rows() );
            assert(mSigmaInv.rows() == mSigmaInv.cols() );

            //find the matrix sigma and its Cholesky decomposition
            LLTType lltOfSigmaInv(mSigmaInv.inverse());

            assert(lltOfSigmaInv.info() == Eigen::Success);

            // find the Cholesky
            mChol = lltOfSigmaInv.matrixL();
        }

        /**
         * \breif A function for computing the log-pdf
         * \param x the point at which probability is sought
         * \return prbability at \a x
         */
        realScalarType compute(realVectorType const& x) const
        {
            return -0.5*(mMu-x).transpose()*mSigmaInv*(mMu-x);
        }

        /**
         * \brief Generate samples from the multi-variate Gaussian
         * \return a sample from the multi-variate Gaussian
         */
        realVectorType generate()
        {
            // generate a vector from N(0,1)
            realVectorType x(mMu.rows());
            for(indexType i=0;i<x.rows();++i)
            {
                x(i) = mNormDist(mGen);
            }

            // rotates the samples
            x = mMu + mChol*x;

            return x;
        }
    private:
        realVectorType mMu; /**< mean */
        realMatrixType mSigmaInv; /**< inverse of the covariance matrix */

        std::mt19937 mGen; /**< random number generator */
        std::normal_distribution<realScalarType> mNormDist; /**< normal(0,1) distribution */

        realMatrixType mChol; /**< Cholesky decomposition of the covariance matrix */
    };

    /**
     * \fn split(const std::string &s, char delim, std::vector<realScalarType> & elems)
     * \brief A function for splitting delimiter separated values
     * \param s input string
     * \param delim delimiter
     * \param elements a vector values to be returned
     */
    template<typename realScalarType>
    void split(const std::string &s, char delim, std::vector<realScalarType> & elems)
    {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim))
        {
            elems.push_back(atof(item.c_str()));
        }
    }

    /**
     * \fn readData(std::string const& fileName,char delim,std::vector< std::vector<realScalarType> > & data)
     * \brief A function for reading ascii data from a text file
     * \param fileName file-name
     * \param delim delimiter
     * \param data data matrix to be returned
     */
    template<typename realScalarType>
    void readData(std::string const& fileName,char delim,
        std::vector< std::vector<realScalarType> > & data)
    {
        std::cout<<"# reading data from "<<fileName<<std::endl;
        std::ifstream inFile(fileName);
        std::string line;
        while( std::getline(inFile, line) )
        {
            std::vector<realScalarType> rowEntry;
            split(line,delim,rowEntry);
            data.push_back(rowEntry);
        }

        std::cout<<"# Data has "<<data.size()<<" rows and "
            <<data[0].size()<<" columns "<<std::endl;
    }


}//namespace utils

#endif //KDE_UTILS_HPP
