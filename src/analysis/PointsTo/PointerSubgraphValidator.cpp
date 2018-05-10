#include <string>
#include "PointerSubgraphValidator.h"

namespace dg {
namespace analysis {
namespace pta {
namespace debug {


bool PointerSubgraphValidator::reportInvalNumberOfOperands(const PSNode *nd, const std::string& user_err) {
    errors += "Invalid number of operands for " + std::string(PSNodeTypeToCString(nd->getType())) +
              " with ID " + std::to_string(nd->getID()) + "\n  - operands: [";
    for (unsigned i = 0, e =  nd->getOperandsNum(); i < e; ++i) {
        errors += std::to_string(nd->getID());
        if (i != e - 1)
            errors += " ";
    }
    errors += "]\n";

    if (!user_err.empty())
        errors += "(" + user_err + ")\n";

    return true;
}

bool PointerSubgraphValidator::reportInvalEdges(const PSNode *nd, const std::string& user_err) {
    errors += "Invalid number of edges for " + std::string(PSNodeTypeToCString(nd->getType())) +
              " with ID " + std::to_string(nd->getID()) + "\n";
    if (!user_err.empty())
        errors += "(" + user_err + ")\n";
    return true;
}

bool PointerSubgraphValidator::reportUnreachableNode(const PSNode *nd) {
    errors += "Unreachable " + std::string(PSNodeTypeToCString(nd->getType())) +
              " with ID " + std::to_string(nd->getID()) + "\n";
    return true;
}

static bool hasDuplicateOperand(const PSNode *nd)
{
    std::set<const PSNode *> ops;
    for (const PSNode *op : nd->getOperands()) {
        if (!ops.insert(op).second)
            return true;
    }

    return false;
}

bool PointerSubgraphValidator::checkOperands() {
    bool invalid = false;

    for (const PSNode *nd : PS->getNodes()) {
        // this is the first node
        // XXX: do we know this?
        if (!nd)
            continue;

        switch (nd->getType()) {
            case PSNodeType::PHI:
                if (nd->getOperandsNum() == 0) {
                    invalid |= reportInvalNumberOfOperands(nd);
                } else if (hasDuplicateOperand(nd)) {
                    invalid |= reportInvalNumberOfOperands(nd, "PHI Node contains duplicated operand");
                }
                break;
            case PSNodeType::NULL_ADDR:
            case PSNodeType::UNKNOWN_MEM:
            case PSNodeType::NOOP:
            case PSNodeType::FUNCTION:
            case PSNodeType::CONSTANT:
                if (nd->getOperandsNum() != 0) {
                    invalid |= reportInvalNumberOfOperands(nd);
                }
                break;
            case PSNodeType::GEP:
            case PSNodeType::LOAD:
            case PSNodeType::CAST:
            case PSNodeType::INVALIDATE_OBJECT:
            case PSNodeType::FREE:
                if (nd->getOperandsNum() != 1) {
                    invalid |= reportInvalNumberOfOperands(nd);
                }
                break;
            case PSNodeType::STORE:
            case PSNodeType::MEMCPY:
                if (nd->getOperandsNum() != 2) {
                    invalid |= reportInvalNumberOfOperands(nd);
                }
                break;
        }
    }

    return invalid;
}

static inline bool isInPredecessors(const PSNode *nd, const PSNode *of) {
    for (const PSNode *pred: of->getPredecessors()) {
        if (pred == nd)
            return true;
    }

    return false;
}

static inline bool canBeOutsideGraph(const PSNode *nd) {
    return (nd->getType() == PSNodeType::FUNCTION ||
            nd->getType() == PSNodeType::CONSTANT ||
            nd->getType() == PSNodeType::UNKNOWN_MEM ||
            nd->getType() == PSNodeType::NULL_ADDR);
}

std::set<const PSNode *> reachableNodes(const PSNode *nd) {
    std::set<const PSNode *> reachable;
    reachable.insert(nd);

    std::vector<const PSNode *> to_process;
    to_process.reserve(4);
    to_process.push_back(nd);

    while (!to_process.empty()) {
        std::vector<const PSNode *> new_to_process;
        new_to_process.reserve(to_process.size());

        for (const PSNode *cur : to_process) {
            for (const PSNode *succ : cur->getSuccessors()) {
                if (reachable.insert(succ).second)
                    new_to_process.push_back(succ);
            }
        }

        new_to_process.swap(to_process);
    }

    return reachable;
}

bool PointerSubgraphValidator::checkEdges() {
    bool invalid = false;

    // check incoming/outcoming edges of all nodes
    auto nodes = PS->getNodes();
    for (const PSNode *nd : nodes) {
        if (!nd)
            continue;

        if (nd->predecessorsNum() == 0 && nd != PS->getRoot() && !canBeOutsideGraph(nd)) {
            invalid |= reportInvalEdges(nd, "Non-root node has no predecessors");
        }

        for (const PSNode *succ : nd->getSuccessors()) {
            if (!isInPredecessors(nd, succ))
                invalid |= reportInvalEdges(nd, "Node not set as a predecessor of some of its successors");
        }
    }

    // check that the edges form valid CFG (all nodes are reachable)
    auto reachable = reachableNodes(PS->getRoot());
    for (const PSNode *nd : nodes) {
        if (!nd)
            continue;

        if (reachable.count(nd) < 1 && !canBeOutsideGraph(nd)) {
                invalid |= reportUnreachableNode(nd);
        }
    }

    return invalid;
}

bool PointerSubgraphValidator::validate() {
    bool invalid = false;

    invalid |= checkOperands();
    invalid |= checkEdges();

    return invalid;
}


} // namespace debug
} // namespace pta
} // namespace analysis
} // namespace dg

