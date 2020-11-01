/**
 * @file top_floorplan.h
 * @brief The floorplan for top-level system integration
 * @author Keren Zhu
 * @date 10/05/2020
 */

#pragma once

#include "db/DesignDB.h"
#include "util/linear_programming.h"
#include "util/Box.h"
#include "constraint_graph.hpp"

PROJECT_NAMESPACE_BEGIN

/// @brief the top-level problem
class TopFloorplanProblem
{
    friend class IlpTopFloorplanProblem;
    public:
        explicit TopFloorplanProblem() = default;
        /// @brief intialize a problem from CktGraph
        void initProblem(const DesignDB& dDb, const CktGraph &ckt, const std::string& symnetFile);
        /// @brief set resource per length
        void setPinResourceLen(IntType pinResourceLen) { _resourcePerLen = pinResourceLen; }
    private:
        enum class FpPinType
        {
            ASYM = 0,
            SYM_PRI = 1, // The primary one in the sym pair
            SYM_SCE = 2, // The secondary one in the sym pair
            OTHER // Don't need to care
        };
        enum class FpPinAssignType
        {
            LEFT = 0,
            RIGHT = 1,
            UNDEFINED = 2
        };
        struct PinIdx
        {
            FpPinType pinType; 
            IndexType idx;
            IntType cellIdx;
            std::string name = "";
            FpPinAssignType assign = FpPinAssignType::UNDEFINED;
        };
        struct FpNet
        {
            std::vector<IndexType> pins;
        };
        /* Problem   */
        std::vector<PinIdx> _pinIdx; ///< To relate the pin in CktGraph to the variables
        std::vector<Box<LocType>> _cellBBox; ///< The original cell layout bounding boxes
        std::vector<std::string> _cellNames; ///< The name of the cells
        IntType _resourcePerLen = 10000; ///< To quanitfy the pin resource per length
        std::vector<FpNet> _nets; ///< The nets
        IndexType _numAsymPins = 0; ///< The number of asym pins need to assign
        IndexType _numSymPriPins = 0; ///< The number of primary sym pins need to assign
        IndexType _numSymSecPins = 0; ///< The number of primary sym pins need to assign
};

class TopFloorplanProblemResult
{
    friend class IlpTopFloorplanProblem;
    public:
        explicit TopFloorplanProblemResult() = default;
        /// @brief get whether assign pin to left or right
        /// @param first: the cell name
        /// @param second: the pin name (internal net name)
        /// @return 0: left 1: right. -1: not defined
        IntType pinAssignSol(const std::string &cellName, const std::string &pinName)
        {
            if (_pinAssignMap.find(cellName) == _pinAssignMap.end())
            {
                return -1;
            }
            if (_pinAssignMap.at(cellName).find(pinName) == _pinAssignMap.at(cellName).end())
            {
                return -1;
            }
            return _pinAssignMap.at(cellName).at(pinName);
        }
        /// @brief get the planned cell height 
        /// @param the module name
        /// @return the target height. -1 if not found
        IntType targetModuleHeight(const std::string &cellName)
        {
            if (_cellYLenMap.find(cellName) == _cellYLenMap.end())
            {
                return -1;
            }
            return _cellYLenMap.at(cellName);
        }
        const std::map<std::string, std::map<std::string, IntType>> & pinAssignMap() const { return _pinAssignMap; }
        const std::map<std::string, IntType> & cellYLenMap() const { return _cellYLenMap; }
        const std::map<std::string, Box<LocType>> & netExternalBBoxMap() const { return _netExternalBBoxMap; }
        /// @brief add one external net bounding box
        /// @param  the pin name
        /// @return if valid the bounding box of the net exclude the pin itself. If not valid, no such pin info
        Box<LocType> externalNetBBox(const std::string &pinName)
        {
            if (_netExternalBBoxMap.find(pinName) == _netExternalBBoxMap.end())
            {
                Box<LocType> box;
                Assert(not box.valid());
                return box;
            }
            return _netExternalBBoxMap[pinName];
        }
    
    private:
        std::map<std::string, std::map<std::string, IntType>> _pinAssignMap; ///< map[cell name][pin name] = status
        std::map<std::string, IntType> _cellYLenMap; ///< map[cell name] = cell height
        std::map<std::string, Box<LocType>> _netExternalBBoxMap; ///< map[pin name] = the net bbox excluding the pin
};

/// @brief the ilp for solving the TopFloorplanProblem
class IlpTopFloorplanProblem
{
    typedef ::klib::lp::LpModel lp_solver_type;
    typedef ::klib::lp::LpTrait lp_trait;
    typedef lp_trait::variable_type lp_variable_type;
    typedef lp_trait::expr_type lp_expr_type;
    public:
        explicit IlpTopFloorplanProblem(const TopFloorplanProblem &problem)
            : _problem(problem) {}
        /// @brief solve the problem
        /// @return true: successful.
        bool solve();
        /* Write out the result */
        void writeOut(TopFloorplanProblemResult &result);
    private:
        /// @brief sweep line to generate the vertical constraint graph
        void verticalSweepLine();
        /* ILP Variable */
        void addVariables();
        /// @brief get the crossing variable
        /// @param first: index of the net
        /// @param second: index of the first pin in _problem.nets.pins
        /// @param third: index of the second pin in _problem.nets.pins
        const lp_variable_type & crossVariable(IndexType netIdx, IndexType firstPinIdx, IndexType secondPinIdx) const
        {
            // Calculate the index of the pair
            // Assume the order is (0,1) (0,2) (1,2)
            if (firstPinIdx > secondPinIdx)
            {
                std::swap(firstPinIdx, secondPinIdx);
            }
            auto numPins = _problem._nets.at(netIdx).pins.size();
            IndexType offset = 0;
            for (IndexType i = 0; i < firstPinIdx; ++i)
            {
                offset += (numPins - i - 1);
            }
            IndexType idx = offset + (secondPinIdx - firstPinIdx - 1);
            return _crossVars.at(netIdx).at(idx);
        }
        /* ILP Constraints */
        void addConstr();
        void addYLoConstr();
        void addPinResrouceConstr();
        void addCrossConstr();
        void addBoundaryConstr();
        /* ILP Objective Function */
        void configObjFunc();
        /* ILP solve */
        bool solveIlp();
        /* For debugging */
        void printVariableValue();
        
    private:
        std::vector<constraint_graph::ConstraintEdge> _verConstrGraph; ///< The vertical constraint graph
        const TopFloorplanProblem &_problem;
        lp_solver_type _solver; ///< ILP model
        lp_expr_type _obj;  ///< ILP objective function
        std::vector<lp_variable_type> _symPinAssignVars; ///< The binary variables to represent how to assign the sym pins
        std::vector<lp_variable_type> _aSymAssignVars; ///< The binary variables to represent how to assign the asym pins
        std::vector<lp_variable_type> _extraResourcesVars; ///< The variables to represent the extra resources of each module
        std::vector<lp_variable_type> _yLoVars; ///< The lower y variables for each module
        std::vector<std::vector<lp_variable_type>> _crossVars; ///< The variables to represent whether there is a crossing between pair of pins
        lp_variable_type _yHiVar; ///< The variable representing the upper boundary of the floorplan
};

class DesignDB;

namespace FP {
    /// @brief apply floorplan solution to design db
    /// @param first a floorplan solution
    /// @param the design DB
    /// @param the index of circuit that floorplan solution be in
    void applyFpSolution(const TopFloorplanProblemResult &sol, DesignDB &db, IndexType cktIdx);
} // namespace FP

PROJECT_NAMESPACE_END