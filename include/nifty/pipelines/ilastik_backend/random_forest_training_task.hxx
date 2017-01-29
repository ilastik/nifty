#pragma once

#include "nifty/pipelines/ilastik_backend/feature_computation.hxx"
#include "nifty/pipelines/ilastik_backend/random_forest_training.hxx"

#include <tbb/tbb.h>
#define TBB_PREVIEW_CONCURRENT_LRU_CACHE 1
#include <tbb/concurrent_lru_cache.h>

namespace nifty{
namespace pipelines{
namespace ilastik_backend{
            
    template<unsigned DIM>
    class random_forest_training_task : public tbb::task
    {
    private:
        using data_type = float;
        using label_data_type = uint32_t;
        using coordinate = nifty::array::StaticArray<int64_t, DIM>;
        using multichan_coordinate = nifty::array::StaticArray<int64_t, DIM+1>;
        
        using float_array = nifty::marray::Marray<data_type>;
        using float_array_view = nifty::marray::View<data_type>;
        using uint32_array = nifty::marray::Marray<label_data_type>;
        using uint32_array_view = nifty::marray::View<label_data_type>;
        
        using cache    = tbb::concurrent_lru_cache<size_t, float_array, std::function<float_array(size_t)>>;
        using random_forest = RandomForest2Type;

        using blocking = tools::Blocking<DIM>;
        using selection_type = selected_feature_type;
        
    public:

        // construct batch prediction for single input
        random_forest_training_task(
                const std::vector<std::string> & label_block_filenames,
                const std::string & label_block_key,
                const std::string & out_rf_file,
                const std::string & out_rf_key,
                const selection_type & selected_features,
                const std::shared_ptr<blocking> & blocking,
                const std::shared_ptr<cache>& feature_cache,
                const coordinate & roiBegin
        ) :
            blocking_(blocking),
            label_block_filenames_(label_block_filenames),
            label_block_key_(label_block_key),
            featureCache_(feature_cache),
            selectedFeatures_(selected_features),
            out_rf_file_(out_rf_file),
            out_rf_key_(out_rf_key),
            roiBegin_(roiBegin)
        {
            //init();
        }

        
        void init() {
            std::cout << "random_forest_training_task::init called" << std::endl;
            
            // TODO: read labelled blocks and their blockSlice attribute
            // No need to store those in a cache, as we always need ALL of them anyways.
        }

        std::vector<size_t> find_required_block_ids()
        {
            // TODO: given all the labelled blocks and their coordinates, 
            //       get all the blocks of our blocking of which we need to fetch the features
            // ATTENTION: we might need the same blockId from several labelled blocks!
        }

        size_t count_labelled_pixels()
        {
            // TODO: count the non-zero pixels in all labelled blocks
        }

        void save_random_forest()
        {
            // TODO: save rf_ to out_rf_file_ at out_rf_key
        }

        tbb::task* execute() {
            
            std::cout << "batch_prediction_task::execute called" << std::endl;
            init();

            std::vector<size_t> required_block_ids = find_required_block_ids();
            size_t num_labelled_pixels = count_labelled_pixels();
            size_t num_features = getNumberOfChannels<DIM>(this->selectedFeatures_);

            float_array feature_matrix({num_labelled_pixels, num_features});
            uint32_array label_vector({num_labelled_pixels});

            // read features of required blocks and that of the labelled pixels into the feature matrix
            tbb::parallel_for(tbb::blocked_range<size_t>(0, required_block_ids.size()), 
                              [this, &required_block_ids](const tbb::blocked_range<size_t> &range)
            {
                for( size_t idx=range.begin(); idx!=range.end(); ++idx ) 
                {
                    size_t blockId = required_block_ids[idx];
                    std::cout << "fetching block " << blockId << " / " << blocking_->numberOfBlocks() << std::endl;

                    auto handle = (*(this->featureCache_))[blockId];
                    auto featureView = handle.value();

                    auto block = blocking_->getBlock(blockId);
                    coordinate blockBegin = block.begin() - roiBegin_;

                    // TODO: extract the features for the labelled pixels and put them into the feature matrix
                    // TODO: store the corresponding labels into label_vector
                }       
            });

            random_forest2_training<DIM>(feature_matrix, label_vector, rf_);
            save_random_forest();

            return NULL;
        }


    private:
        static std::mutex s_mutex;
        // global blocking
        std::shared_ptr<blocking> blocking_;
        std::string in_key_;
        hid_t rawHandle_;
        hid_t outHandle_;
        std::string outKey_;
        std::shared_ptr<cache> featureCache_;
        coordinate blockShape_;
        size_t maxNumCacheEntries_;
        random_forest rf_;
        coordinate roiBegin_;
        coordinate roiEnd_;
        size_t nClasses_;
        std::vector<std::string> label_block_filenames_;
        std::string label_block_key_;
        std::string out_rf_file_;
        std::string out_rf_key_;
        selection_type selectedFeatures_;
    };
    
    template <unsigned DIM>
    std::mutex random_forest_training_task<DIM>::s_mutex;
    
} // namespace ilastik_backend
} // namepsace pipelines
} // namespace nifty
