/*
 * File:   utils.h
 * Author: stefan
 *
 * Created on June 19, 2013, 4:06 PM
 */

#pragma once

#include "../../Constraint.h"
#include "../../Formula.h"
#include "../LRAModule/LRAModule.h"
#include <carl/numbers/operations.h>

namespace smtrat
{
    namespace icp
    {
        /**
        * Determines, if the given expression is linear
        * @param _constr Needed to associate linearization with constraint
        * @param _expr Expression, which is checked
        * @return true, if expression is linear
        */
        bool isLinear( const Constraint* _constr, const Polynomial& _expr, FastMap<Polynomial, const Constraint*>& _tempMonomes );
        
        /**
        * Creates a new constraint from an existing interval
        * @param _interval
        * @return pair <lowerBoundConstraint*, upperBoundConstraint*>
        */
        std::pair<const Constraint*, const Constraint*> intervalToConstraint( const carl::Variable& _var, const smtrat::DoubleInterval _interval );
        
        /**
        * Checks mIntervals if it contains an empty interval.
        * @return 
        */
        bool intervalBoxContainsEmptyInterval(const EvalDoubleIntervalMap& _intervals);
        
        
        const LRAVariable* getOriginalLraVar ( const carl::Variable::Arg _var, LRAModule& _lra );
        
    }
}
