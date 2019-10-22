#ifndef DG_DATA_DEPENDENCE_ANALYSIS_H_
#define DG_DATA_DEPENDENCE_ANALYSIS_H_

#include <cassert>
#include <memory>

#include "dg/analysis/Offset.h"
#include "dg/analysis/DataDependence/DataDependenceAnalysisImpl.h"
#include "dg/analysis/DataDependence/DataDependenceAnalysisOptions.h"
#include "dg/analysis/ReadWriteGraph/ReadWriteGraph.h"
#include "dg/analysis/MemorySSA/MemorySSA.h"
#include "dg/analysis/ReachingDefinitions/ReachingDefinitions.h"

#include "dg/util/debug.h"

namespace dg {
namespace analysis {

// here the types are for type-checking (optional - user can do it
// when building the graph) and for later optimizations

class DataDependenceAnalysis {

    // the implementation either uses reaching definitions
    // or transformation to SSA
    std::unique_ptr<DataDependenceAnalysisImpl> _impl;

    DataDependenceAnalysisImpl *
    createAnalysis(ReadWriteGraph&& graph,
                   const DataDependenceAnalysisOptions& opts)
    {
        if (opts.isSSA()) {
            return new MemorySSATransformation(std::move(graph), opts);
        } else {
            return new ReachingDefinitionsAnalysis(std::move(graph), opts);
        }
    }

    const DataDependenceAnalysisOptions& _options;

public:
    DataDependenceAnalysis(ReadWriteGraph&& graph,
                           const DataDependenceAnalysisOptions& opts)
    : _impl(createAnalysis(std::move(graph), opts)), _options(opts) {}

    DataDependenceAnalysis(ReadWriteGraph&& graph)
    : DataDependenceAnalysis(std::move(graph), {}) {}

    ReadWriteGraph *getGraph() { return _impl->getGraph(); }
    const ReadWriteGraph *getGraph() const { return _impl->getGraph(); }
    RWNode *getRoot() const { return _impl->getRoot(); }

    // run the analysis
    void run() { _impl->run(); }

    // return the reaching definitions of ('mem', 'off', 'len')
    // at the location 'where'
    std::vector<RWNode *> getDefinitions(RWNode *where, RWNode *mem,
                                         const Offset& off, const Offset& len) {
        return _impl->getDefinitions(where, mem, off, len);
    }

    // return reaching definitions of a node that represents
    // the given use
    std::vector<RWNode *> getDefinitions(RWNode *use) {
        return _impl->getDefinitions(use);
    }

    const DataDependenceAnalysisOptions& getOptions() const { return _options; }

    DataDependenceAnalysisImpl *getImpl() { return _impl.get(); }
    const DataDependenceAnalysisImpl *getImpl() const { return _impl.get(); }
};



} // namespace analysis
} // namespace dg

#endif // DG_DATA_DEPENDENCE_ANALYSIS_H_
