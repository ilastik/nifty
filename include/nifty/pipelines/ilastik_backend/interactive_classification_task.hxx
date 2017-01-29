#pragma once

#include "nifty/pipelines/ilastik_backend/feature_computation.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_prediction.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_training.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_loader.hxx"
#include "nifty/pipelines/ilastik_backend/cached_prediction_task.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_training_task.hxx"

#include <tbb/tbb.h>
#define TBB_PREVIEW_CONCURRENT_LRU_CACHE 1
#include <tbb/concurrent_lru_cache.h>

namespace nifty{
namespace pipelines{
namespace ilastik_backend{
            
    template<unsigned DIM>
    class interactive_classification_task : public tbb::task
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

        // construct batch prediction for single input
        interactive_classification_task(const std::string & in_file,
                const std::string & in_key,
                const std::vector<std::string> & label_block_filenames,
                const std::string & label_block_key,
                const std::string & rf_file,
                const std::string & rf_key,
                const std::string & out_file,
                const std::string & out_key,
                const selection_type & selected_features,
                const coordinate & block_shape,
                const size_t max_num_cache_entries,
                const coordinate & roiBegin,
                const coordinate & roiEnd) :
            blocking_(),
            in_key_(in_key),
            label_block_filenames_(label_block_filenames),
            label_block_key_(label_block_key),
            rfFile_(rf_file),
            rfKey_(rf_key),
            rawHandle_(hdf5::openFile(in_file)),
            outFile_(out_file),
            outKey_(out_key),
            featureCache_(),
            selectedFeatures_(selected_features),
            blockShape_(block_shape),
            maxNumCacheEntries_(max_num_cache_entries),
            roiBegin_(roiBegin),
            roiEnd_(roiEnd)
        {
            //init();
        }

        
        void init() {

            std::cout << "interactive_classification_task::init called" << std::endl;
            rawCache_ = std::make_unique<raw_cache>( rawHandle_, in_key_ );

            // TODO handle the roi shapes in python
            // init the blocking
            coordinate roiShape = roiEnd_ - roiBegin_;
            blocking_ = std::make_shared<blocking>(roiBegin_, roiEnd_, blockShape_);

            // init the feature cache
            std::function<float_array(size_t)> retrieve_features_for_caching = [this](size_t blockId) -> float_array {
                
                // compute features    
                const auto halo = getHaloShape<DIM>(this->selectedFeatures_);

                // compute coordinates from blockId
                const auto & blockWithHalo = this->blocking_->getBlockWithHalo(blockId, halo);
                const auto & outerBlock = blockWithHalo.outerBlock();
                const auto & outerBlockShape = outerBlock.shape();

                // load the raw data
                uint_array raw(outerBlockShape.begin(), outerBlockShape.end());
                {
		        std::lock_guard<std::mutex> lock(this->s_mutex);
                this->rawCache_->readSubarray(outerBlock.begin().begin(), raw);
                }

                // allocate the feature array
                size_t nChannels = getNumberOfChannels<DIM>(this->selectedFeatures_);
                multichan_coordinate filterShape;
                filterShape[0] = nChannels;
                for(int d = 0; d < DIM; ++d)
                    filterShape[d+1] = outerBlockShape[d];
                float_array feature_array(filterShape.begin(), filterShape.end());

		        feature_computation<DIM>(
                        blockId,
                        raw,
                        feature_array,
                        this->selectedFeatures_);

                // resize the out array to cut the halo
                const auto & localCore  = blockWithHalo.innerBlockLocal();
                const auto & localBegin = localCore.begin();
                const auto & localShape = localCore.shape();

                multichan_coordinate coreBegin;
                multichan_coordinate coreShape;
                for(int d = 1; d < DIM+1; d++){
                    coreBegin[d] = localBegin[d-1];
                    coreShape[d]  = localShape[d-1];
                }
                coreBegin[0] = 0;
                coreShape[0] = feature_array.shape(0);

                feature_array = feature_array.view(coreBegin.begin(), coreShape.begin());
                return feature_array;
            
            };
            
            featureCache_ = std::make_shared<cache>(retrieve_features_for_caching, maxNumCacheEntries_);
        }

        tbb::task* execute() {
            
            std::cout << "interactive_classification_task::execute called" << std::endl;
            
            init();
            
            // FROM FUTURE IMPORT TODO: wait for incoming label blocks and start processing only once these are there

            // start training task
            random_forest_training_task<DIM>& random_forest_training = *new(tbb::task::allocate_child()) random_forest_training_task<DIM>(
                label_block_filenames_, label_block_key_,
                rfFile_, rfKey_,
                selectedFeatures_,
                blocking_,
                featureCache_,
                roiBegin_);

            this->spawn_and_wait_for_all(random_forest_training);
            
            // once that is done, we can predict
            cached_prediction_task<DIM>& cached_prediction = *new(tbb::task::allocate_child()) cached_prediction_task<DIM>(
                rfFile_, rfKey_,
                outFile_, outKey_,
                blockShape_, maxNumCacheEntries_,
                roiBegin_, roiEnd_,
                blocking_,
                featureCache_);

            this->spawn_and_wait_for_all(cached_prediction);

            // close the rawFile and outFile
            hdf5::closeFile(rawHandle_);

            return NULL;
        }


    private:
	    static std::mutex s_mutex;
        // global blocking
        std::shared_ptr<blocking> blocking_;
        std::unique_ptr<raw_cache> rawCache_;
        std::string in_key_;
        std::string rfFile_;
        std::string rfKey_;
        hid_t rawHandle_;
        std::string outKey_;
        std::string outFile_;
        std::shared_ptr<cache> featureCache_;
        selection_type selectedFeatures_;
        coordinate blockShape_;
        size_t maxNumCacheEntries_;
        coordinate roiBegin_;
        coordinate roiEnd_;
        std::vector<std::string> label_block_filenames_;
        std::string label_block_key_;
    };
    
    template <unsigned DIM>
    std::mutex interactive_classification_task<DIM>::s_mutex;
    
} // namespace ilastik_backend
} // namepsace pipelines
} // namespace nifty
