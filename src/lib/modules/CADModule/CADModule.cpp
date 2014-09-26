/*
 * SMT-RAT - Satisfiability-Modulo-Theories Real Algebra Toolbox
 * Copyright (C) 2012 Florian Corzilius, Ulrich Loup, Erika Abraham, Sebastian Junges
 *
 * This file is part of SMT-RAT.
 *
 * SMT-RAT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SMT-RAT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SMT-RAT.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/**
 * @file CADModule.cpp
 *
 * @author Ulrich Loup
 * @since 2012-01-19
 * @version 2013-07-10
 */

#include "../../solver/Manager.h"
#include "CADModule.h"

#include <memory>
#include <iostream>

#include "carl/core/logging.h"

using carl::UnivariatePolynomial;
using carl::cad::EliminationSet;
using carl::cad::Constraint;
using carl::Polynomial;
using carl::CAD;
using carl::RealAlgebraicPoint;
using carl::cad::ConflictGraph;

using namespace std;

// CAD settings
//#define SMTRAT_CAD_GENERIC_SETTING
#define SMTRAT_CAD_DISABLE_PROJECTIONORDEROPTIMIZATION
//#define SMTRAT_CAD_DISABLE_SMT
//#define SMTRAT_CAD_DISABLE_MIS
//#define CHECK_SMALLER_MUSES
//#define SMTRAT_CAD_ONEMOSTDEGREEVERTEX_MISHEURISTIC
//#define SMTRAT_CAD_TWOMOSTDEGREEVERTICES_MISHEURISTIC
#ifdef SMTRAT_CAD_DISABLE_SMT
	#define SMTRAT_CAD_DISABLE_MIS
#endif

namespace smtrat
{

	CADModule::CADModule(ModuleType _type, const ModuleInput* _formula, RuntimeSettings*, Conditionals& _conditionals, Manager* const _manager):
		Module(_type, _formula, _conditionals, _manager),
		mCAD(_conditionals),
		mConstraints(),
		hasFalse(false),
		subformulaQueue(),
		mConstraintsMap(),
		mRealAlgebraicSolution(),
		mConflictGraph(),
		mVariableBounds()
#ifdef SMTRAT_DEVOPTION_Statistics
		,mStats(CADStatistics::getInstance(0))
#endif
	{
		mInfeasibleSubsets.clear();	// initially everything is satisfied
		// CAD setting
		carl::cad::CADSettings setting = mCAD.getSetting();
		// general setting set
		setting = carl::cad::CADSettings::getSettings(carl::cad::CADSettingsType::BOUNDED); // standard
		setting.simplifyByFactorization = true;
		setting.simplifyByRootcounting  = true;

		#ifdef SMTRAT_CAD_DISABLE_MIS
			setting.computeConflictGraph = false;
		#else
			setting.computeConflictGraph = true;
		#endif

		setting.trimVariables = false; // maintains the dimension important for the constraint checking
//		setting.autoSeparateEquations = false; // <- @TODO: find a correct implementation of the MIS for the only-strict or only-equations optimizations

		#ifndef SMTRAT_CAD_DISABLE_PROJECTIONORDEROPTIMIZATION
		// variable order optimization
		std::forward_list<symbol> variables = std::forward_list<symbol>( );
		GiNaC::symtab allVariables = mpReceivedFormula->constraintPool().realVariables();
		for( GiNaC::symtab::const_iterator i = allVariables.begin(); i != allVariables.end(); ++i )
			variables.push_front( GiNaC::ex_to<symbol>( i->second ) );
		std::forward_list<Polynomial> polynomials = std::forward_list<Polynomial>( );
		for( fcs_const_iterator i = mpReceivedFormula->constraintPool().begin(); i != mpReceivedFormula->constraintPool().end(); ++i )
			polynomials.push_front( (*i)->lhs() );
		mCAD = CAD( {}, CAD::orderVariablesGreeedily( variables.begin(), variables.end(), polynomials.begin(), polynomials.end() ), _conditionals, setting );
		#ifdef MODULE_VERBOSE
		cout << "Optimizing CAD variable order from ";
		for( forward_list<GiNaC::symbol>::const_iterator k = variables.begin(); k != variables.end(); ++k )
			cout << *k << " ";
		cout << "  to   ";
		for( vector<GiNaC::symbol>::const_iterator k = mCAD.variablesScheduled().begin(); k != mCAD.variablesScheduled().end(); ++k )
			cout << *k << " ";
		cout << endl;;
		#endif
		#else
		mCAD.alterSetting(setting);
		#endif

		LOGMSG_TRACE("smtrat.cad", "Initial CAD setting:" << std::endl << setting);
		#ifdef SMTRAT_CAD_GENERIC_SETTING
		LOGMSG_TRACE("smtrat.cad", "SMTRAT_CAD_GENERIC_SETTING set");
		#endif
		#ifdef SMTRAT_CAD_DISABLE_PROJECTIONORDEROPTIMIZATION
		LOGMSG_TRACE("smtrat.cad", "SMTRAT_CAD_DISABLE_PROJECTIONORDEROPTIMIZATION set");
		#endif
		#ifdef SMTRAT_CAD_DISABLE_SMT
		LOGMSG_TRACE("smtrat.cad", "SMTRAT_CAD_DISABLE_SMT set");
		#endif
		#ifdef SMTRAT_CAD_DISABLE_MIS
		LOGMSG_TRACE("smtrat.cad", "SMTRAT_CAD_DISABLE_MIS set");
		#endif
	}

	CADModule::~CADModule(){}

	/**
	 * This method just adds the respective constraint of the subformula, which ought to be one real constraint,
	 * to the local list of constraints. Moreover, the list of all variables is updated accordingly.
	 *
	 * Note that the CAD object is not touched here, the respective calls to CAD::addPolynomial and CAD::check happen in isConsistent.
	 * @param _subformula
	 * @return returns false if the current list of constraints was already found to be unsatisfiable (in this case, nothing is done), returns true previous result if the constraint was already checked for consistency before, otherwise true
	 */
	bool CADModule::assertSubformula(ModuleInput::const_iterator _subformula)
	{
		LOG_FUNC("smtrat.cad", **_subformula);
		Module::assertSubformula(_subformula);
		switch ((*_subformula)->getType()) {
		case TTRUE: 
			return true;
		case FFALSE: {
			this->hasFalse = true;
			PointerSet<Formula> infSubSet;
			infSubSet.insert(*_subformula);
			mInfeasibleSubsets.push_back(infSubSet);
			foundAnswer(False);
			return false;
		}
		case CONSTRAINT: {
			if (this->hasFalse) {
				this->subformulaQueue.insert(*_subformula);
				return false;
			} else {
				return this->addConstraintFormula(*_subformula);
			}
		}
		default:
			LOGMSG_ERROR("smtrat.cad", "Asserted " << **_subformula);
			assert(false);
			return true;
		}
	}

	/**
	 * All constraints asserted (and not removed)  so far are now added to the CAD object and checked for consistency.
	 * If the result is false, a minimal infeasible subset of the original constraint set is computed.
	 * Otherwise a sample value is available.
	 * @return True if consistent, False otherwise
	 */
	Answer CADModule::isConsistent()
	{
		if (this->hasFalse) return foundAnswer(False);
		else {
			for (auto f: this->subformulaQueue) {
				this->addConstraintFormula(f);
			}
			this->subformulaQueue.clear();
		}
		//std::cout << "CAD has:" << std::endl;
		//for (auto c: this->mConstraints) std::cout << "\t\t" << c << std::endl;
		//this->printReceivedFormula();
		if (!mpReceivedFormula->isRealConstraintConjunction() && !mpReceivedFormula->isIntegerConstraintConjunction()) {
			return foundAnswer(Unknown);
		}
		if (!mInfeasibleSubsets.empty())
			return foundAnswer(False); // there was no constraint removed which was in a previously generated infeasible subset
		// perform the scheduled elimination and see if there were new variables added
		if (mCAD.prepareElimination())
			mConflictGraph.clearSampleVertices(); // all sample vertices are now invalid, thus remove them
		// check the extended constraints for satisfiability

		if (variableBounds().isConflicting()) {
			mInfeasibleSubsets.push_back(variableBounds().getConflict());
			mRealAlgebraicSolution = carl::RealAlgebraicPoint<smtrat::Rational>();
			return foundAnswer(False);
		}
		carl::CAD<smtrat::Rational>::BoundMap boundMap;
		std::map<carl::Variable, carl::Interval<smtrat::Rational>> eiMap = mVariableBounds.getEvalIntervalMap();
		std::vector<carl::Variable> variables = mCAD.getVariables();
		for (unsigned v = 0; v < variables.size(); ++v)
		{
			auto vPos = eiMap.find(variables[v]);
			if (vPos != eiMap.end())
				boundMap[v] = vPos->second;
		}
		if (!mCAD.check(mConstraints, mRealAlgebraicSolution, mConflictGraph, boundMap, false, false))
		{
			#ifdef SMTRAT_CAD_DISABLE_SMT
			// simulate non-incrementality by constructing a trivial infeasible subset and clearing all data in the CAD
			#define SMTRAT_CAD_DISABLE_MIS // this constructs a trivial infeasible subset below
			mCAD.clear();
			// replay adding the polynomials as scheduled polynomials
			for( vector<carl::cad::Constraint<smtrat::Rational>>::const_iterator constraint = mConstraints.begin(); constraint != mConstraints.end(); ++constraint )
				mCAD.addPolynomial( constraint->getPolynomial(), constraint->getVariables() );
			#endif
			#ifdef SMTRAT_CAD_DISABLE_MIS
			// construct a trivial infeasible subset
			PointerSet<Formula> boundConstraints = mVariableBounds.getOriginsOfBounds();
			mInfeasibleSubsets.push_back( PointerSet<Formula>() );
			for (auto i:mConstraintsMap)
			{
				mInfeasibleSubsets.back().insert( *i.first );
			}
			mInfeasibleSubsets.back().insert( boundConstraints.begin(), boundConstraints.end() );
			#else
			// construct an infeasible subset
			assert(mCAD.getSetting().computeConflictGraph);
			// copy conflict graph for destructive heuristics and invert it
			ConflictGraph g(mConflictGraph);
			g.invert();
			#if defined SMTRAT_CAD_ONEMOSTDEGREEVERTEX_MISHEURISTIC
				// remove the lowest-degree vertex (highest degree in inverted graph)
				g.removeConstraintVertex(g.maxDegreeVertex());
			#elif defined SMTRAT_CAD_TWOMOSTDEGREEVERTICES_MISHEURISTIC
				// remove the two lowest-degree vertices (highest degree in inverted graph)
				g.removeConstraintVertex(g.maxDegreeVertex());
				g.removeConstraintVertex(g.maxDegreeVertex());
			#else
				// remove last vertex, assuming it is part of the MIS
				assert(mConstraints.size() > 0);
				g.removeConstraintVertex(mConstraints.size() - 1);
			#endif
			
			LOGMSG_DEBUG("smtrat.cad", "Input: " << mConstraints);
			for (auto j: mConstraints) LOGMSG_DEBUG("smtrat.cad", "\t" << j);
			LOGMSG_DEBUG("smtrat.cad", "Bounds: " << mVariableBounds);
				
			vec_set_const_pFormula infeasibleSubsets = extractMinimalInfeasibleSubsets_GreedyHeuristics(g);

			PointerSet<Formula> boundConstraints = mVariableBounds.getOriginsOfBounds();
			for (auto i: infeasibleSubsets) {
				LOGMSG_DEBUG("smtrat.cad", "Infeasible:");
				for (auto j: i) LOGMSG_DEBUG("smtrat.cad", "\t" << *j);
				mInfeasibleSubsets.push_back(i);
				mInfeasibleSubsets.back().insert(boundConstraints.begin(), boundConstraints.end());
			}

			#ifdef CHECK_SMALLER_MUSES
			Module::checkInfSubsetForMinimality(mInfeasibleSubsets->begin());
			#endif
			#endif
			mRealAlgebraicSolution = carl::RealAlgebraicPoint<smtrat::Rational>();
			return foundAnswer(False);
		}
		LOGMSG_TRACE("smtrat.cad", "#Samples: " << mCAD.samples().size());
		LOGMSG_TRACE("smtrat.cad", "Elimination sets:");
		for (unsigned i = 0; i != mCAD.getEliminationSets().size(); ++i) {
			LOGMSG_TRACE("smtrat.cad", "\tLevel " << i << " (" << mCAD.getEliminationSet(i).size() << "): " << mCAD.getEliminationSet(i));
		}
		LOGMSG_TRACE("smtrat.cad", "Result: true");
		LOGMSG_TRACE("smtrat.cad", "CAD complete: " << mCAD.isComplete());
		LOGMSG_TRACE("smtrat.cad", "Solution point: " << mRealAlgebraicSolution);
		mInfeasibleSubsets.clear();
		if (mpReceivedFormula->isIntegerConstraintConjunction()) {
			// Check whether the found assignment is integer.
			std::vector<carl::Variable> vars(mCAD.getVariables());
			for (unsigned d = 0; d < this->mRealAlgebraicSolution.dim(); d++) {
				auto r = this->mRealAlgebraicSolution[d]->branchingPoint();
				if (!carl::isInteger(r)) {
					branchAt(Polynomial(vars[d]), r);
					return foundAnswer(Unknown);
				}
			}
		}
		return foundAnswer(True);
	}

	void CADModule::removeSubformula(ModuleInput::const_iterator _subformula)
	{
		switch ((*_subformula)->getType()) {
		case TTRUE:
			Module::removeSubformula(_subformula);
			return;
		case FFALSE:
			this->hasFalse = false;
			Module::removeSubformula(_subformula);
			return;
		case CONSTRAINT: {
			auto it = this->subformulaQueue.find(*_subformula);
			if (it != this->subformulaQueue.end()) {
				this->subformulaQueue.erase(it);
				return;
			}

			mVariableBounds.removeBound((*_subformula)->pConstraint(), *_subformula);

			ConstraintIndexMap::iterator constraintIt = mConstraintsMap.find(*_subformula);
			if (constraintIt == mConstraintsMap.end())
				return; // there is nothing to remove
			carl::cad::Constraint<smtrat::Rational> constraint = mConstraints[constraintIt->second];

			LOGMSG_TRACE("smtrat.cad", "---- Constraint removal (before) ----");
			LOGMSG_TRACE("smtrat.cad", "Elimination sets:");
			for (unsigned i = 0; i != mCAD.getEliminationSets().size(); ++i) {
				LOGMSG_TRACE("smtrat.cad", "\tLevel " << i << " (" << mCAD.getEliminationSet(i).size() << "): " << mCAD.getEliminationSet(i));
			}
			LOGMSG_TRACE("smtrat.cad", "#Samples: " << mCAD.samples().size());
			LOGMSG_TRACE("smtrat.cad", "-----------------------------------------");
			LOGMSG_TRACE("smtrat.cad", "Removing " << constraint << "...");

			unsigned constraintIndex = constraintIt->second;
			// remove the constraint in mConstraintsMap
			mConstraintsMap.erase(constraintIt);
			// remove the constraint from the list of constraints
			assert(mConstraints.size() > constraintIndex); // the constraint to be removed should be stored in the local constraint list
			mConstraints.erase(mConstraints.begin() + constraintIndex);	// erase the (constraintIt->second)-th element
			// update the constraint / index map, i.e., decrement all indices above the removed one
			updateConstraintMap(constraintIndex, true);
			// remove the corresponding constraint node with index constraintIndex
			mConflictGraph.removeConstraintVertex(constraintIndex);
			// remove the corresponding polynomial from the CAD if it is not occurring in another constraint
			bool doDelete = true;

			///@todo Why was this iteration reversed?
			for (auto c: mConstraints) {
				if (constraint.getPolynomial() == c.getPolynomial()) {
					doDelete = false;
					break;
				}
			}
			if (doDelete) // no other constraint claims the polynomial, hence remove it from the list and the cad
				mCAD.removePolynomial(constraint.getPolynomial());

			LOGMSG_TRACE("smtrat.cad", "---- Constraint removal (afterwards) ----");
			LOGMSG_TRACE("smtrat.cad", "New constraint set: " << mConstraints);
			LOGMSG_TRACE("smtrat.cad", "Elimination sets:");
			for (unsigned i = 0; i != mCAD.getEliminationSets().size(); ++i) {
				LOGMSG_TRACE("smtrat.cad", "\tLevel " << i << " (" << mCAD.getEliminationSet(i).size() << "): " << mCAD.getEliminationSet(i));
			}
			LOGMSG_TRACE("smtrat.cad", "#Samples: " << mCAD.samples().size());
			LOGMSG_TRACE("smtrat.cad", "-----------------------------------------");

			Module::removeSubformula(_subformula);
			return;
		}
		default:
			return;
		}
	}

	/**
	 * Updates the model.
	 */
	void CADModule::updateModel() const
	{
		clearModel();
		if (this->solverState() == True) {
			// bound-independent part of the model
			std::vector<carl::Variable> vars(mCAD.getVariables());
			for (unsigned varID = 0; varID < vars.size(); ++varID) {
				Assignment ass = mRealAlgebraicSolution[varID];
				mModel.insert(std::make_pair(vars[varID], ass));
			}
			// bounds for variables which were not handled in the solution point
			for (auto b: mVariableBounds.getIntervalMap()) {
				// add an assignment for every bound of a variable not in vars (Caution! Destroys vars!)
				std::vector<carl::Variable>::iterator v = std::find(vars.begin(), vars.end(), b.first);
				if (v != vars.end()) {
					vars.erase(v); // shall never be found again
				} else {
					// variable not handled by CAD, use the midpoint of the bounding interval for the assignment
					Assignment ass = b.second.center();
					mModel.insert(std::make_pair(b.first, ass));
				}
			}
		}
	}

	///////////////////////
	// Auxiliary methods //
	///////////////////////
	
	bool CADModule::addConstraintFormula(const Formula* f) {
		assert(f->getType() == CONSTRAINT);
		mVariableBounds.addBound(f->pConstraint(), f);
		// add the constraint to the local list of constraints and memorize the index/constraint assignment if the constraint is not present already
		if (mConstraintsMap.find(f) != mConstraintsMap.end())
			return true;	// the exact constraint was already considered
		carl::cad::Constraint<smtrat::Rational> constraint = convertConstraint(f->constraint());
		mConstraints.push_back(constraint);
		mConstraintsMap[f] = (unsigned)(mConstraints.size() - 1);
		mCAD.addPolynomial(Polynomial(constraint.getPolynomial()), constraint.getVariables());
		mConflictGraph.addConstraintVertex(); // increases constraint index internally what corresponds to adding a new constraint node with index mConstraints.size()-1

		return solverState() != False;
	}

	/**
	 * Converts the constraint types.
	 * @param c constraint of the SMT-RAT
	 * @return constraint of GiNaCRA
	 */
	inline const carl::cad::Constraint<smtrat::Rational> CADModule::convertConstraint( const smtrat::Constraint& c )
	{
		// convert the constraints variable
		std::vector<carl::Variable> variables;
		for (auto i: c.variables()) {
			variables.push_back(i);
		}
		carl::Sign signForConstraint = carl::Sign::ZERO;
		bool cadConstraintNegated = false;
		switch (c.relation()) {
			case Relation::EQ: 
				break;
			case Relation::LEQ:
				signForConstraint	= carl::Sign::POSITIVE;
				cadConstraintNegated = true;
				break;
			case Relation::GEQ:
				signForConstraint	= carl::Sign::NEGATIVE;
				cadConstraintNegated = true;
				break;
			case Relation::LESS:
				signForConstraint = carl::Sign::NEGATIVE;
				break;
			case Relation::GREATER:
				signForConstraint = carl::Sign::POSITIVE;
				break;
			case Relation::NEQ:
				cadConstraintNegated = true;
				break;
			default: assert(false);
		}
		return carl::cad::Constraint<smtrat::Rational>(c.lhs(), signForConstraint, variables, cadConstraintNegated);
	}

	/**
	 * Converts the constraint types.
	 * @param c constraint of the GiNaCRA
	 * @return constraint of SMT-RAT
	 */
	inline const Constraint* CADModule::convertConstraint( const carl::cad::Constraint<smtrat::Rational>& c )
	{
		Relation relation = Relation::EQ;
		switch (c.getSign()) {
			case carl::Sign::POSITIVE:
				if (c.isNegated()) relation = Relation::LEQ;
				else relation = Relation::GREATER;
				break;
			case carl::Sign::ZERO:
				if (c.isNegated()) relation = Relation::NEQ;
				else relation = Relation::EQ;
				break;
			case carl::Sign::NEGATIVE:
				if (c.isNegated()) relation = Relation::GEQ;
				else relation = Relation::LESS;
				break;
			default: assert(false);
		}
		return newConstraint(Polynomial(c.getPolynomial()), relation);
	}

	/**
	 * Computes an infeasible subset of the current set of constraints by approximating a vertex cover of the given conflict graph.
	 * 
	 * Caution! The method is destructive with regard to the conflict graph.
	 *
	 * Heuristics:
	 * Select the highest-degree vertex for the vertex cover and remove it as long as we have edges in the graph.
	 *
	 * @param conflictGraph the conflict graph is destroyed during the computation
	 * @return an infeasible subset of the current set of constraints
	 */
	inline vec_set_const_pFormula CADModule::extractMinimalInfeasibleSubsets_GreedyHeuristics( ConflictGraph& conflictGraph )
	{
		// initialize MIS with the last constraint
		vec_set_const_pFormula mis = vec_set_const_pFormula(1, PointerSet<Formula>());
		mis.front().insert(getConstraintAt((unsigned)(mConstraints.size() - 1)));	// the last constraint is assumed to be always in the MIS
		if (mConstraints.size() > 1) {
			// construct set cover by greedy heuristic
			std::list<ConflictGraph::Vertex> setCover;
			long unsigned vertex = conflictGraph.maxDegreeVertex();
			while (conflictGraph.degree(vertex) > 0) {
				// add v to the setCover
				setCover.push_back(vertex);
				// remove coverage information of v from conflictGraph
				conflictGraph.invertConflictingVertices(vertex);
				LOGMSG_TRACE("smtrat.cad", "Conflict graph after removal of " << vertex << ": " << endl << conflictGraph);
				// get the new vertex with the biggest number of adjacent solution point vertices
				vertex = conflictGraph.maxDegreeVertex();
			}
			// collect constraints according to the vertex cover
			for (auto v: setCover)
				mis.front().insert(getConstraintAt((unsigned)(v)));
		}
		return mis;
	}

	/**
	 *
	 * @param index
	 * @return
	 */
	inline const Formula* CADModule::getConstraintAt(unsigned index) {
		LOGMSG_TRACE("smtrat.cad", "get " << index << " from " << mConstraintsMap);
		for (auto i: mConstraintsMap) {
			if (i.second == index) // found the entry in the constraint map
				return i.first;
		}
		assert(false);	// The given index should match an input constraint!
		return nullptr;
	}

	/**
	 * Increment all indices stored in the constraint map being greater than the given index; decrement if decrement is true.
	 * @param index
	 * @param decrement
	 */
	inline void CADModule::updateConstraintMap(unsigned index, bool decrement) {
		LOGMSG_TRACE("smtrat.cad", "updating " << index << " from " << mConstraintsMap);
		if (decrement) {
			for (auto& i: mConstraintsMap) {
				if (i.second > index) i.second--;
			}
		} else {
			for (auto& i: mConstraintsMap) {
				if (i.second > index) i.second++;
			}
		}
		LOGMSG_TRACE("smtrat.cad", "now: " << mConstraintsMap);
	}

}	// namespace smtrat

