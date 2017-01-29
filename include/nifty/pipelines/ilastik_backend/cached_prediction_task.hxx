#pragma once

#include "nifty/pipelines/ilastik_backend/feature_computation.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_prediction.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_loader.hxx"

#include <tbb/tbb.h>
#define TBB_PREVIEW_CONCURRENT_LRU_CACHE 1
#include <tbb/concurrent_lru_cache.h>

namespace nifty{
namespace pipelines{
namespace ilastik_backend{
            
    template<unsigned DIM>
    class cached_prediction_task : public tbb::task
    {
    private:
        using data_type = float;
        using in_data_type = uint8_t;
        using coordinate = nifty::array::StaticArray<int64_t, DIM>;
        using multichan_coordinate = nifty::array::StaticArray<int64_t, DIM+1>;
        
        using float_array = nifty::marray::Marray<data_type>;
        using float_array_view = nifty::marray::View<data_type>;
        using uint_array = nifty::marray::Marray<in_data_type>;
        using uint_array_view = nifty::marray::View<in_data_type>;
        
        using raw_cache = hdf5::Hdf5Array<in_data_type>;
        using cache    = tbb::concurrent_lru_cache<size_t, float_array, std::function<float_array(size_t)>>;
        using random_forest = RandomForest2Type;

        using blocking = tools::Blocking<DIM>;
        using selection_type = selected_feature_type;
        

    public:

        // construct prediction for single input
        cached_prediction_task(
                const std::string & rf_file,
                const std::string & rf_key,
                const std::string & out_file,
                const std::string & out_key,
                const coordinate & block_shape,
                const size_t max_num_cache_entries,
                const coordinate & roiBegin,
                const coordinate & roiEnd,
                const std::shared_ptr<blocking> & blocking,
                const std::shared_ptr<cache>& feature_cache
        ) :
            blocking_(blocking),
            rfFile_(rf_file),
            rfKey_(rf_key),
            outHandle_(hdf5::createFile(out_file)),
            outKey_(out_key),
            featureCache_(feature_cache),
            predictionCache_(),
            blockShape_(block_shape),
            maxNumCacheEntries_(max_num_cache_entries),
            rf_(), // TODO we don't need this once we learn the rf
            roiBegin_(roiBegin),
            roiEnd_(roiEnd)
        {
            //init();
        }

        
        void init() {

            std::cout << "cached_prediction_task::init called" << std::endl;
            // TODO handle the roi shapes in python
            // init the blocking
            coordinate roiShape = roiEnd_ - roiBegin_;
            
            // TODO make rf single threaded
            rf_ = get_rf2_from_file(rfFile_, rfKey_);
            
            nClasses_ = rf_.class_count();
            
            // init the prediction cache
            std::function<float_array(size_t)> retrieve_prediction_for_caching = [this](size_t blockId) -> float_array {
                
                // predict the random forest
                const auto & block = this->blocking_->getBlock(blockId);
                const auto & blockShape = block.shape();
                
                multichan_coordinate predictionShape;
                predictionShape[DIM] = nClasses_;
                for(int d = 0; d < DIM; ++d)
                    predictionShape[d] = blockShape[d];

                // get the features from the feature cache
                auto feature_handle = (*(this->featureCache_))[blockId];
                auto feature_array = feature_handle.value();

                float_array prediction_array(predictionShape.begin(), predictionShape.end());
                random_forest2_prediction<DIM>(blockId, feature_array, prediction_array, this->rf_, 1);

                return prediction_array;
            };

            predictionCache_ = std::make_unique<cache>(retrieve_prediction_for_caching, maxNumCacheEntries_);

            multichan_coordinate outShape, chunkShape; 
            outShape[DIM] = this->nClasses_;
            chunkShape[DIM] = 1;
            for(int d = 0; d < DIM; ++d) {
                outShape[d] = roiShape[d];
                chunkShape[d] = blockShape_[d];
            }
            out_ = std::make_unique<hdf5::Hdf5Array<data_type>>( outHandle_, outKey_, outShape.begin(), outShape.end(), chunkShape.begin() );
        }

        //TODO request and block subtasks


        tbb::task* execute() {
            
            std::cout << "batch_prediction_task::execute called" << std::endl;
            
            init();
            
            // why two loops ?!
            // -> TBB just slices the whole range up into separate tasks, each of which
            // executes the inner loop and works on its own subrange of the blockIds.
            tbb::parallel_for(tbb::blocked_range<size_t>(0,blocking_->numberOfBlocks()), [this](const tbb::blocked_range<size_t> &range) {
                for( size_t blockId=range.begin(); blockId!=range.end(); ++blockId ) {

                    std::cout << "Processing block " << blockId << " / " << blocking_->numberOfBlocks() << std::endl;

                    auto handle = (*(this->predictionCache_))[blockId];
                    auto outView = handle.value();

                    auto block = blocking_->getBlock(blockId);
                    coordinate blockBegin = block.begin() - roiBegin_;

                    // need to attach the channel coordinate
                    multichan_coordinate outBegin;
                    for(int d = 0; d < DIM; ++d)
                        outBegin[d] = blockBegin[d];
                    outBegin[DIM] = 0;

                    {
                    std::lock_guard<std::mutex> lock(this->s_mutex);
                    out_->writeSubarray(outBegin.begin(), outView);
                    }
                }       
            });

            // close the rawFile and outFile
            hdf5::closeFile(outHandle_);

            return NULL;
        }


    private:
        static std::mutex s_mutex;
        // global blocking
        std::shared_ptr<blocking> blocking_;
        std::string rfFile_;
        std::string rfKey_;
        hid_t outHandle_;
        std::string outKey_;
        std::shared_ptr<cache> featureCache_;
        std::unique_ptr<cache> predictionCache_;
        coordinate blockShape_;
        size_t maxNumCacheEntries_;
        random_forest rf_;
        std::unique_ptr<hdf5::Hdf5Array<data_type>> out_;
        coordinate roiBegin_;
        coordinate roiEnd_;
        size_t nClasses_;
    };
    
    template <unsigned DIM>
    std::mutex cached_prediction_task<DIM>::s_mutex;
    
} // namespace ilastik_backend
} // namepsace pipelines
} // namespace nifty
