#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "nifty/graph/simple_graph.hxx"
#include "nifty/graph/gala/gala_feature_base.hxx"

#include "../../converter.hxx"
#include "py_gala_feature_base.hxx"




namespace py = pybind11;

//PYBIND11_DECLARE_HOLDER_TYPE(__T, std::shared_ptr<__T>);

namespace nifty{
namespace graph{



    using namespace py;
    //PYBIND11_DECLARE_HOLDER_TYPE(BASE_CLASS_, std::shared_ptr<BASE_CLASS_>);

    void exportGalaFeatureBase(py::module & galaModule) {

        typedef UndirectedGraph<> GraphType;
        typedef double FeatureValueType;
        typedef PyGalaFeatureBase<GraphType, FeatureValueType> PyGalaFeatureBaseType;
        typedef GalaFeatureBase<GraphType, FeatureValueType> GalaFeatureBaseType;

        //PYBIND11_DECLARE_HOLDER_TYPE(McBase, std::shared_ptr<McBase>);

        // base factory
        py::class_<
            GalaFeatureBaseType, 
            std::unique_ptr<GalaFeatureBaseType>, 
            PyGalaFeatureBaseType 
        > galaFeatureBase(galaModule, "GalaFeatureBaseTypeUndirectedGraph");
        
        galaFeatureBase
        ;


        typedef DefaultAccEdgeMap<GraphType, FeatureValueType> EdgeMapType;
        typedef DefaultAccNodeMap<GraphType, FeatureValueType> NodeMapType;


        // concrete visitors
        typedef GalaDefaultAccFeature<GraphType, FeatureValueType> GalaDefaultAccFeatureType; 

        py::class_<GalaDefaultAccFeatureType >(galaModule, "GalaDefaultAccFeatureUndirectedGraph",  galaFeatureBase)
        .def(py::init<const GraphType &, const EdgeMapType & , const NodeMapType &>(),
                py::arg("graph"), 
                py::arg("edgeFeatures"), 
                py::arg("nodeFeatures"),
                py::keep_alive<1,2>(),
                py::keep_alive<1,3>(),
                py::keep_alive<1,4>()
            )
        ;
        
        galaModule.def("galaDefaultAccFeature",
            [](const GraphType & g, const EdgeMapType & e, const NodeMapType & n){
                auto ptr = new GalaDefaultAccFeatureType(g,e,n);
                return ptr;
            },
            py::return_value_policy::take_ownership,
            py::arg("graph"), 
            py::arg("edgeFeatures"), 
            py::arg("nodeFeatures"),
            py::keep_alive<0,1>(),
            py::keep_alive<0,2>(),
            py::keep_alive<0,3>()
        );

    }

}
}
    