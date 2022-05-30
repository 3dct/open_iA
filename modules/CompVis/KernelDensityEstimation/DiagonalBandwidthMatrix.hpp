#ifndef KDE_DIAGONAL_BANDWIDTH_MATRIX_HPP
#define KDE_DIAGONAL_BANDWIDTH_MATRIX_HPP

#include <KernelDensityEstimation/KernelDensityEstimation>

namespace kde
{
    /**
     * \class DiagonalBandwidthMatrix
     *
     * \brief A class for computing Bandwidth Matrix using Silverman 1986
     *
     * \tparam _realScalarType real floating point type
     *
     * This class computes the diagonal bandwidth matrix. The values of the
     * diagonal elements are given by Silverman 1986. These can be written as
     * \f$ \sqrt{\mathbf{H}_{ii}} = \left( \frac{4}{d+2} \right)^{\frac{1}{d+4}}
     * n^{-\frac{1}{d+4}} \sigma_i \f$.
     *
     */
    template<typename _realScalarType>
    class DiagonalBandwidthMatrix
    {
    public:
        typedef _realScalarType realScalarType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,1> realVectorType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,1> bandwidhtMatrixType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,Eigen::Dynamic> realMatrixType;
        typedef typename realMatrixType::Index indexType;

        /**
         * \brief A constructor that sets up the diagonal bandwidths
         * \param data input data
         */
        explicit DiagonalBandwidthMatrix(realMatrixType const& data)
        :mHInvSqrt(data.cols())
        {
            computeBWSilvermanRule(data,mHInvSqrt);
        }

        /**
         * \brief Scale the input vector with bandwith matrix
         * \param x input vector
         *
         * This function performs |H|^{-1/2}x
         */
        void scale(realVectorType & x) const
        {
            assert(x.rows() == mHInvSqrt.rows());

            x = x.cwiseProduct(mHInvSqrt);
        }

        /**
         * \brief Return the bandwidth matrix
         * \return the bandwidth matrix
         *
         * This function returns the diagonal |H|^{-1/2} matrix
         */
        bandwidhtMatrixType const & bandwidth() const
        {
            return mHInvSqrt;
        }

    private:

        /**
         * A function for computing diagonal badnwidth matrix using
         * Silverman's rule. Hii = (4/(d+2))^(1/(d+4)) * n^(-1/(d+4)) * sigma
         * http://en.wikipedia.org/wiki/Multivariate_kernel_density_estimation#Rule_of_thumb
         *
         * \param data input data matrix
         * \param bandwidthMatrix output diagonal bandwidth matrix
         */
        void computeBWSilvermanRule(realMatrixType const& data,
            bandwidhtMatrixType & bandwidthMatrix)
        {
            for(indexType i=0;i<data.cols();++i)
            {
                //find std-dvn of each dimension
                realScalarType numDims = (realScalarType) data.cols();
                realScalarType count = (realScalarType) data.rows();
                realScalarType mean = data.col(i).mean();
                realScalarType sqMean = data.col(i).squaredNorm()/count;
                realScalarType sigma = std::sqrt(sqMean - mean*mean);
                //compute the bandwidth for each dimension using Silverman 1986
                bandwidthMatrix(i) = std::pow( realScalarType(4.)/realScalarType(numDims+2.),
                realScalarType(-1.)/realScalarType(numDims+4.) )
                    * std::pow(count,realScalarType(1.)/realScalarType(numDims+4.) )
                    / sigma ;
            }
        }

        bandwidhtMatrixType mHInvSqrt; /**< H^{-1/2} */
    };

}//namespace kde

#endif //KDE_DIAGONAL_BANDWIDTH_MATRIX_HPP
