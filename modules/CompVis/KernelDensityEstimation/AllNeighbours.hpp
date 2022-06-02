#ifndef KDE_ALLNEIGHBOURS_HPP
#define KDE_ALLNEIGHBOURS_HPP

#include <KernelDensityEstimation/KernelDensityEstimation>

namespace kde
{
    /**
     * \class AllNeighbours
     *
     * \brief A class that returs all neighbours of a point
     *
     * \tparam _realScalarType real floating point type
     *
     * This class returns all points in the data for pdf estimation. All points
     * are considered to be neighbours.
     */
    template<typename _realScalarType>
    class AllNeighbours
    {
    public:
        typedef _realScalarType realScalarType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,1> realVectorType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,Eigen::Dynamic> realMatrixType;
        typedef typename realMatrixType::Index indexType;
        typedef std::vector<size_t> neighbourIndexVectorType;

        /**
         * \brief A constructor that sets up the class
         * \param data input data
         */
        explicit AllNeighbours(realMatrixType const & data)
        :mNIVect(data.rows()),mNumDims(data.cols())
        {
            assert(data.rows()>1);
            assert(data.cols()>0);

            for(size_t i=0;i<mNIVect.size();++i)
            {
                mNIVect[i] = i;
            }
        }

        /**
         * \brief A function that return the indices of (all) neighbours
         * \param x the point for which neighbours are sought
         * \return The indices of (all) neighbours
         */
        inline neighbourIndexVectorType indices(realVectorType const& x)
        {
            Q_UNUSED(x);
            assert(mNumDims == x.rows());
            return mNIVect;
        }

    private:
        neighbourIndexVectorType mNIVect; /**< neighbour indices */
        indexType mNumDims; /**< number of dimensions */

    };

}//namespace kde


#endif //KDE_ALLNEIGHBOURS_HPP
