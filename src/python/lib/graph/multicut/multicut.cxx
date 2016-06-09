#include <pybind11/pybind11.h>
#include <iostream>

namespace py = pybind11;

namespace nifty{
namespace graph{


    void exportMulticutObjective(py::module &);
    void exportMulticutFactory(py::module &);

    void initSubmoduleMulticut(py::module &graphModule) {
        auto multicutModule = graphModule.def_submodule("multicut","multicut submodule");

        exportMulticutObjective(multicutModule);
        exportMulticutFactory(multicutModule);
    }

}
}