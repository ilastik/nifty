#pragma once
#ifndef NIFTY_GRAPH_GALA_CONTRACTION_ORDER_HXX
#define NIFTY_GRAPH_GALA_CONTRACTION_ORDER_HXX

#include <iostream>
#include <set>
#include <tuple> 

#include "vigra/multi_array.hxx"
#include "vigra/random_forest.hxx"
#include "vigra/priority_queue.hxx"
#include "vigra/algorithm.hxx"

#include "nifty/tools/runtime_check.hxx"
#include "nifty/graph/simple_graph.hxx"
#include "nifty/graph/gala/gala_feature_base.hxx"
#include "nifty/graph/gala/gala_instance.hxx"



#include "nifty/graph/multicut/multicut_base.hxx"
#include "nifty/graph/multicut/multicut_visitor_base.hxx"
#include "nifty/graph/multicut/multicut_factory.hxx"
#include "nifty/graph/multicut/fusion_move_based.hxx"
#include "nifty/graph/multicut/multicut_greedy_additive.hxx"
#include "nifty/graph/multicut/proposal_generators/watershed_proposals.hxx"
#include "nifty/graph/multicut/multicut_objective.hxx"
#include "nifty/graph/multicut/perturb_and_map.hxx"




namespace nifty{
namespace graph{





struct McGreedyHybridSetttings{
    
    std::array<double, 4> weights{{1.0,1.0,1.0,0.001}};
    double stopWeight{0.5};
    double localRfDamping{0.001};
};



template< class CALLBACK>
class McGreedyHybridBase{
private:
    typedef CALLBACK CallbackType;
    typedef typename CallbackType::ValueType                          ValueType;
    typedef typename CallbackType::GraphType                          GraphType;
    typedef typename CallbackType::EdgeContractionGraphType           CGraphType;
    typedef typename GraphType:: template EdgeMap<ValueType>          EdgeMapDouble;
    typedef vigra::ChangeablePriorityQueue< ValueType ,std::less<ValueType> > QueueType;

public: 
    typedef McGreedyHybridSetttings Setttings;

    McGreedyHybridBase(CALLBACK & callback,
        const bool training,
        const Setttings & settings = Setttings()
    )
    :   callback_(callback),
        graph_(callback.graph()),
        cgraph_(callback.cgraph()),
        training_(training),
        settings_(settings),
        pq_(callback.graph().maxEdgeId()+1),
        localRfProbs_(callback.graph()),
        mcPerturbAndMapProbs_(callback.graph()),
        mcMapProbs_(callback.graph()),
        wardProbs_(callback.graph()),
        constraints_(callback.graph(),0),
        hasMcPerturbAndMapProbs_(false),
        hasMcMapProbs_(false)
    {

    }

    uint64_t edgeToContractNext(){
        return pq_.top();
    }

    bool stopContraction(){
        if(!training_){
            return pq_.topPriority() >= settings_.stopWeight;
        }
        else{
            return pq_.topPriority() >= 1.5;
        }
    }

    void reset(){
        while(!pq_.empty()){
            pq_.pop();
        }
        hasMcPerturbAndMapProbs_ = false;
        hasMcMapProbs_ = false;
    }

    void setInitalLocalRfProb(const uint64_t edge, const ValueType prob){

        // initialization
        localRfProbs_[edge] = prob;

        this->updateWardProbs(edge,false);
        this->updatePriority(edge);
    }

    void updateLocalRfProb(const uint64_t edge, const ValueType newProb){
        const auto oldProb = localRfProbs_[edge];
        const auto d = settings_.localRfDamping;
        localRfProbs_[edge] = (1.0 - d)*newProb + d*oldProb;
        this->updatePriority(edge);
    }

    void updateWardProbs(const uint64_t edge, const bool update=true){
        const auto uv = cgraph_.uv(edge);
        const auto su = callback_.currentNodeSizes()[uv.first];
        const auto sv = callback_.currentNodeSizes()[uv.second];

        const auto ssu = 1.0 - std::exp(-4.0*su);
        const auto ssv = 1.0 - std::exp(-4.0*sv);

        const auto ward = 2.0 /( (1.0/ssu) + (1.0/ssv));
        wardProbs_[edge] = ward;
        if(update){
            this->updatePriority(edge);
        }
    }
    
    void constraintsEdge(const uint64_t edge){
        constraints_[edge] = std::numeric_limits<ValueType>::infinity();
        this->updatePriority(edge);
    }


    // call from external
    void contractEdge(const uint64_t edgeToContract){
        NIFTY_TEST(pq_.contains(edgeToContract));
        pq_.deleteItem(edgeToContract);
    }

    void mergeNodes(const uint64_t aliveNode, const uint64_t deadNode){

        const auto sa = callback_.currentNodeSizes()[aliveNode];
        const auto sd = callback_.currentNodeSizes()[deadNode];
        const auto s = sa + sd;


    }

    void mergeEdges(const uint64_t aliveEdge, const uint64_t deadEdge){
        // TODO merge the maps!
        pq_.deleteItem(deadEdge);

        // sizes are NOT YET merged, therefore it is save to do this
        const auto sa = callback_.currentEdgeSizes()[aliveEdge];
        const auto sd = callback_.currentEdgeSizes()[deadEdge];
        const auto s = sa + sd;

        localRfProbs_[sa]         = (sa*localRfProbs_[aliveEdge]         + sd*localRfProbs_[deadEdge])/s;
        mcPerturbAndMapProbs_[sa] = (sa*mcPerturbAndMapProbs_[aliveEdge] + sd*mcPerturbAndMapProbs_[deadEdge])/s;
        mcMapProbs_[sa]           = (sa*mcMapProbs_[aliveEdge]           + sd*mcMapProbs_[deadEdge])/s;
        wardProbs_[sa]            = (sa*wardProbs_[aliveEdge]            + sd*wardProbs_[deadEdge])/s;
        constraints_[sa]          = (sa*constraints_[aliveEdge]          + sd*constraints_[deadEdge])/s;

    }

    void contractEdgeDone(const uint64_t edgeToContract){
        // update wardness

        const auto u = cgraph_.nodeOfDeadEdge(edgeToContract);
        for(auto adj : cgraph_.adjacency(u)){
            const auto edge = adj.edge();
            this->updateWardProbs(edge);
        }

    }

private:

    void updatePriority(const uint64_t edge){
        const auto newTotalP = makeTotalProb(edge);
        pq_.push(edge, newTotalP);
    }

    ValueType makeTotalProb(const uint64_t edge)const{

        if(constraints_[edge] > 0.000001){
            return std::numeric_limits<ValueType>::infinity();
        }
        else{
            ValueType wSum = 0.0;
            ValueType pAcc = 0.0;
            const auto & w = settings_.weights;

            wSum += w[0];
            pAcc += w[0]*localRfProbs_[edge];
            
            if(hasMcPerturbAndMapProbs_){
                wSum += w[1];
                pAcc += w[1]*mcPerturbAndMapProbs_[edge];
            }

            if(hasMcMapProbs_){
                wSum += w[2];
                pAcc += w[2]*mcMapProbs_[edge];
            }

            
            wSum += w[3];
            pAcc += w[3]*wardProbs_[edge];
            

            return pAcc/wSum;
        }
    }

    CALLBACK & callback_;
    const GraphType & graph_;
    const CGraphType & cgraph_;

    bool training_;
    Setttings settings_;
    QueueType pq_;

    EdgeMapDouble localRfProbs_;
    EdgeMapDouble mcPerturbAndMapProbs_;
    EdgeMapDouble mcMapProbs_;
    EdgeMapDouble wardProbs_;
    EdgeMapDouble constraints_;

    bool hasMcPerturbAndMapProbs_;
    bool hasMcMapProbs_;
    bool hasWardProbs_;
};




struct McGreedy{
    template<class CB>
    struct Training{
        typedef McGreedyHybridBase<CB> ResultType;
    };

    template<class CB>
    struct Test{
        typedef McGreedyHybridBase<CB> ResultType;
    };
};



} // namespace nifty::graph
} // namespace nifty

#endif  // NIFTY_GRAPH_GALA_CONTRACTION_ORDER_HXX
