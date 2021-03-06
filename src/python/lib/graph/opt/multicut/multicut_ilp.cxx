#include <pybind11/pybind11.h>

#include <boost/algorithm/string.hpp>


// concrete solvers for concrete factories
#include "nifty/graph/opt/multicut/multicut_ilp.hxx"

#ifdef WITH_GUROBI
#include "nifty/ilp_backend/gurobi.hxx"
#endif

#ifdef WITH_CPLEX
#include "nifty/ilp_backend/cplex.hxx"
#endif

#ifdef WITH_GLPK
#include "nifty/ilp_backend/glpk.hxx"
#endif

#include "nifty/python/graph/undirected_list_graph.hxx"
#include "nifty/python/graph/edge_contraction_graph.hxx"
#include "nifty/python/graph/opt/multicut/multicut_objective.hxx"
#include "nifty/python/converter.hxx"
#include "nifty/python/graph/opt/solver_docstring.hxx"
#include "nifty/python/graph/opt/multicut/export_multicut_solver.hxx"

namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

namespace nifty{
namespace graph{
namespace opt{
namespace multicut{
    
    template<class OBJECTIVE, class BACKEND>
    void exportMulticutIlpWithBackendT(py::module & multicutModule, const std::string & backendName){






        ///////////////////////////////////////////////////////////////
        // DOCSTRING HELPER
        ///////////////////////////////////////////////////////////////
        nifty::graph::opt::SolverDocstringHelper docHelper;
        docHelper.objectiveName = "multicut objective"; 
        docHelper.objectiveClsName = MulticutObjectiveName<OBJECTIVE>::name();
        docHelper.name = "multicut ilp " + backendName; 
        docHelper.mainText =  
            "Find a global optimal solution by a cutting plane ILP solver\n"
            "as described in :cite:`Kappes-2011` \n"
            "and :cite:`andres_2011_probabilistic` \n";
        docHelper.cites.emplace_back("Kappes-2011");
        docHelper.cites.emplace_back("andres_2011_probabilistic");
        docHelper.note = "This might take very long for large models.";
        docHelper.requirements.emplace_back(
            std::string("WITH_") + boost::to_upper_copy<std::string>(backendName)
        );


        
        typedef OBJECTIVE ObjectiveType;
        typedef BACKEND IlpSolver;
        typedef MulticutIlp<ObjectiveType, IlpSolver> Solver;
        
        typedef typename Solver::SettingsType SettingsType;        
        const auto solverName = std::string("MulticutIlp") + backendName;
        // todo exportMulticutSolver should be in the correct namespace

        nifty::graph::opt::multicut::exportMulticutSolver<Solver>(multicutModule, solverName.c_str(), docHelper)

            .def(py::init<>())
            .def_readwrite("numberOfIterations", &SettingsType::numberOfIterations)
            //.def_readwrite("verbose", &SettingsType::verbose)
            //.def_readwrite("verboseIlp", &SettingsType::verboseIlp)
            .def_readwrite("addThreeCyclesConstraints", &SettingsType::addThreeCyclesConstraints)
            .def_readwrite("addOnlyViolatedThreeCyclesConstraints", &SettingsType::addOnlyViolatedThreeCyclesConstraints)
            .def_readwrite("ilpSettings",&SettingsType::ilpSettings)
        ; 
    }

    template<class OBJECTIVE>
    void exportMulticutIlpT(py::module & multicutModule) {
        typedef OBJECTIVE ObjectiveType;
        #ifdef WITH_CPLEX
            exportMulticutIlpWithBackendT<ObjectiveType, ilp_backend::Cplex>(multicutModule, "Cplex");
        #endif
        #ifdef WITH_GUROBI
            exportMulticutIlpWithBackendT<ObjectiveType, ilp_backend::Gurobi>(multicutModule, "Gurobi");
        #endif
        #ifdef WITH_GLPK
            exportMulticutIlpWithBackendT<ObjectiveType, ilp_backend::Glpk>(multicutModule, "Glpk");
        #endif   
        
    }
    
    void exportMulticutIlp(py::module & multicutModule){
        
        py::options options;
        options.disable_function_signatures();
                
        py::class_<ilp_backend::IlpBackendSettings>(multicutModule, "IlpBackendSettings")
            .def(py::init<>())
            .def_readwrite("relativeGap", &ilp_backend::IlpBackendSettings::relativeGap)
            .def_readwrite("absoluteGap", &ilp_backend::IlpBackendSettings::absoluteGap)
            .def_readwrite("memLimit",  &ilp_backend::IlpBackendSettings::memLimit)
        ;

        {
            typedef PyUndirectedGraph GraphType;
            typedef MulticutObjective<GraphType, double> ObjectiveType;
            exportMulticutIlpT<ObjectiveType>(multicutModule);
        }
        {
            typedef PyContractionGraph<PyUndirectedGraph> GraphType;
            typedef MulticutObjective<GraphType, double> ObjectiveType;
            exportMulticutIlpT<ObjectiveType>(multicutModule);
        }     
    }
} // namespace nifty::graph::opt::multicut
} // namespace nifty::graph::opt
} // namespace nifty::graph
} // namespace nifty