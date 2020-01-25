#include <pybind11/pybind11.h>

#define FORCE_IMPORT_ARRAY
#include "xtensor-python/pyarray.hpp"
#include "xtensor-python/pytensor.hpp"
#include "xtensor-python/pyvectorize.hpp"

#include <iostream>

namespace py = pybind11;


namespace nifty{
namespace transformation{
    void exportAffineTransformation(py::module &);
}
}


PYBIND11_MODULE(_transformation, module) {

    xt::import_numpy();

    py::options options;
    options.disable_function_signatures();

    module.doc() = "transformation submodule of nifty";

    using namespace nifty::transformation;
    exportAffineTransformation(module);
}
