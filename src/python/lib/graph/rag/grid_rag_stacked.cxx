#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "nifty/graph/rag/grid_rag_stacked_2d.hxx"

// still need this for python bindings of nifty::ArrayExtender
#include "nifty/python/converter.hxx"

#ifdef WITH_HDF5
#include "nifty/hdf5/hdf5_array.hxx"
#endif

#ifdef WITH_Z5
#include "nifty/z5/z5.hxx"
#endif

#include "xtensor-python/pytensor.hpp"

namespace py = pybind11;


namespace nifty{
namespace graph{



    using namespace py;

    // FIXME switch to xtensor
    template<class CLS, class BASE>
    void removeFunctions(py::class_<CLS, BASE > & clsT){
        clsT
            .def("insertEdge", [](CLS * self,const uint64_t u,const uint64_t ){
                throw std::runtime_error("cannot insert edges into 'GridRag'");
            })
            .def("insertEdges",[](CLS * self, py::array_t<uint64_t> pyArray) {
                throw std::runtime_error("cannot insert edges into 'GridRag'");
            })
        ;
    }

    template<class LABELS_PROXY>
    void exportGridRagStackedT(py::module & ragModule,
                               const std::string & clsName,
                               const std::string & facName){

        typedef LABELS_PROXY LabelsProxyType;
        typedef typename LabelsProxyType::LabelArrayType LabelArrayType;
        typedef GridRag<3, LabelsProxyType> BaseGraph;
        typedef GridRagStacked2D<LabelsProxyType> GridRagType;

        auto clsT = py::class_<GridRagType, BaseGraph>(ragModule, clsName.c_str());
        clsT
            // export shape and acces to the labels proxy object
            //.def_property_readonly("shape", [](const GridRagType & self){return self.shape();})

            //
            // export the per slice properties
            //
            .def("minMaxLabelPerSlice",[](const GridRagType & self){
                const auto & shape = self.shape();
                xt::pytensor<uint64_t, 2> out({int64_t(shape[0]), int64_t(2)});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    auto mima = self.minMaxNode(sliceIndex);
                    out(sliceIndex, 0) = mima.first;
                    out(sliceIndex, 1) = mima.second;
                }
                return out;
            })

            .def("numberOfNodesPerSlice",[](const GridRagType & self){
                const auto & shape = self.shape();
                xt::pytensor<uint64_t, 1> out = xt::zeros<uint64_t>({(int64_t) shape[0]});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfNodes(sliceIndex);
                }
                return out;
            })

            .def("numberOfInSliceEdges",[](const GridRagType & self){
                const auto & shape = self.shape();
                xt::pytensor<uint64_t, 1> out = xt::zeros<uint64_t>({(int64_t) shape[0]});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfInSliceEdges(sliceIndex);
                }
                return out;
            })

            .def("numberOfInBetweenSliceEdges",[](const GridRagType & self){
                const auto & shape = self.shape();
                xt::pytensor<uint64_t, 1> out = xt::zeros<uint64_t>({(int64_t) shape[0]});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfInBetweenSliceEdges(sliceIndex);
                }
                return out;
            })

            .def("inSliceEdgeOffset",[](const GridRagType & self){
                const auto & shape = self.shape();
                xt::pytensor<uint64_t, 1> out = xt::zeros<uint64_t>({(int64_t) shape[0]});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.inSliceEdgeOffset(sliceIndex);
                }
                return out;
            })

            .def("betweenSliceEdgeOffset",[](const GridRagType & self){
                const auto & shape = self.shape();
                xt::pytensor<uint64_t, 1> out = xt::zeros<uint64_t>({(int64_t) shape[0]});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.betweenSliceEdgeOffset(sliceIndex);
                }
                return out;
            })

            .def_property_readonly("totalNumberOfInSliceEdges",[](const GridRagType & self){
                return self.numberOfInSliceEdges();
            })
            .def_property_readonly("totalNumberOfInBetweenSliceEdges",[](const GridRagType & self){
                return self.numberOfInBetweenSliceEdges();
            })
            // export serialization and deserialization
            .def("serialize",[](const GridRagType & self){
                xt::pytensor<uint64_t, 1> out = xt::zeros<uint64_t>({self.serializationSize()});
                auto ptr = &out(0);
                self.serialize(ptr);
                return out;
            })

            .def("deserialize",[](GridRagType & self, xt::pytensor<uint64_t, 1> serialization) {
                    auto startPtr = &serialization(0);
                    auto lastElement = &serialization(serialization.size()-1);
                    auto d = lastElement - startPtr + 1;
                    NIFTY_CHECK_OP(d,==,serialization.size(), "serialization must be contiguous");
                    self.deserialize(startPtr);
            })

            // export edgeLengths TODO remove this once / if we have removed this functionality
            .def("edgeLengths",[](GridRagType & self) {
                xt::pytensor<uint64_t,1> out = xt::zeros<uint64_t>({self.numberOfEdges()});
                const auto & edgeLens = self.edgeLengths();
                for(int edge = 0; edge < self.numberOfEdges(); ++edge)
                    out(edge) = edgeLens[edge];
                return out;
            })
        ;

        removeFunctions<GridRagType, BaseGraph>(clsT);

        // from labels
        ragModule.def(facName.c_str(),
            [](const LabelArrayType & labels,
               const int64_t numberOfLabels,
               const int numberOfThreads){

                LabelsProxyType labelsProxy(labels, numberOfLabels);

                auto s = typename  GridRagType::SettingsType();
                s.numberOfThreads = numberOfThreads;
                return new GridRagType(labelsProxy, s);
            },
            py::return_value_policy::take_ownership,
            py::keep_alive<0, 1>(),
            py::arg("labels").noconvert(),
            py::arg("numberOfLabels"),
            py::arg_t<int>("numberOfThreads", -1)
        );

        // from labels + serialization
        ragModule.def(facName.c_str(),
            [](const LabelArrayType & labels,
               const int64_t numberOfLabels,
               xt::pytensor<uint64_t, 1> serialization
            ){
                LabelsProxyType labelsProxy(labels, numberOfLabels);

                auto startPtr = &serialization(0);
                auto lastElement = &serialization(serialization.size()-1);
                auto d = lastElement - startPtr + 1;
                NIFTY_CHECK_OP(d,==,serialization.size(), "serialization must be contiguous");

                auto s = typename GridRagType::SettingsType();
                s.numberOfThreads = -1;
                return new GridRagType(labelsProxy, startPtr, s);
            },
            py::return_value_policy::take_ownership,
            py::keep_alive<0, 1>(),
            py::arg("labels").noconvert(),
            py::arg("numberOfLabels"),
            py::arg("serialization")
        );

    }

    void exportGridRagStacked(py::module & ragModule) {
        // export in-memory labels
        typedef LabelsProxy<3, xt::pytensor<uint32_t, 3>> ExplicitPyLabels3D32;
        exportGridRagStackedT<ExplicitPyLabels3D32>(ragModule,
                                                  "GridRagStacked2D32",
                                                  "gridRagStacked2D32");
        typedef LabelsProxy<3, xt::pytensor<uint64_t, 3>> ExplicitPyLabels3D64;
        exportGridRagStackedT<ExplicitPyLabels3D64>(ragModule,
                                                  "GridRagStacked2D64",
                                                  "gridRagStacked2D64");

        // export hdf5 labels
        #ifdef WITH_HDF5
        typedef LabelsProxy<3, nifty::hdf5::Hdf5Array<uint32_t>> Hdf5Labels32;
        exportGridRagStackedT<Hdf5Labels32>(ragModule,
                                            "GridRagStacked2DHdf532",
                                            "gridRagStacked2DHdf532");

        typedef LabelsProxy<3, nifty::hdf5::Hdf5Array<uint64_t>> Hdf5Labels64;
        exportGridRagStackedT<Hdf5Labels64>(ragModule,
                                            "GridRagStacked2DHdf564",
                                            "gridRagStacked2DHdf564");
        #endif

        // export z5 labels
        #ifdef WITH_Z5
        //typedef LabelsProxy<3, z5::DatasetTyped<uint32_t>> Z5Labels;
        typedef LabelsProxy<3, nifty::nz5::DatasetWrapper<uint32_t>> Z5Labels32;
        exportGridRagStackedT<Z5Labels32>(ragModule,
                                          "GridRagStacked2DZ532",
                                          "gridRagStacked2DZ532");

        typedef LabelsProxy<3, nifty::nz5::DatasetWrapper<uint64_t>> Z5Labels64;
        exportGridRagStackedT<Z5Labels64>(ragModule,
                                          "GridRagStacked2DZ564",
                                          "gridRagStacked2DZ564");
        #endif
    }

} // end namespace graph
} // end namespace nifty
