#pragma once

#include <algorithm>

// includes for vigra::rf3
#include <vigra/random_forest_3.hxx>
#include <vigra/random_forest_3_hdf5_impex.hxx>

// includes for vigra::rf2
#include <vigra/random_forest_hdf5_impex.hxx>
#include <hdf5_hl.h>

#include <nifty/marray/marray.hxx>

namespace nifty
{
namespace pipelines
{
namespace ilastik_backend
{

using random_forest2 = nifty::pipelines::ilastik_backend::RandomForest2Type;
using random_forest3 = nifty::pipelines::ilastik_backend::RandomForest3Type;

/**
 * Train a random forest given the features and labels.
 * The features should be given as a 2D matrix of dimension samples x features,
 * and the labels as 1D vector with the class per sample.
 */
template<unsigned DIM, typename DATA_TYPE, typename LABEL_TYPE>
void random_forest2_training(
        const nifty::marray::View<DATA_TYPE> & features,
        const nifty::marray::View<LABEL_TYPE> & labels,
        random_forest2 & rf
        )
{
    assert(features.shape(0) == labels.shape(0));
    std::cout << "random_forest2_training from " << features.shape(0) 
              << " samples and " << features.shape(1) << " features" << std::endl;

    //vigra::MultiArrayView<2, DATA_TYPE> in_flatten( vigra::Shape2(pixel_count, num_features), &features(0) );
    
    // copy the feature matrix
    vigra::MultiArray<2, DATA_TYPE> v_feature_matrix( vigra::Shape2(features.shape(0), features.shape(1)) );
    for(size_t p = 0; p < features.shape(0); p++)
    {
        for(size_t f = 0; f < features.shape(1); f++)
        {
            v_feature_matrix(p, f) = features(p, f);
        }
    }

    // copy the labels
    vigra::MultiArray<2, LABEL_TYPE> v_labels( vigra::Shape2(features.shape(0), features.shape(1)) );
    for(size_t p = 0; p < features.shape(1); p++)
    {
        v_labels(p) = labels(p);
    }
    
    // train
    rf.learn(v_feature_matrix, v_labels);
}
    
} // namespace ilastik_backend
} // namespace pipelines
} // namespace nifty
