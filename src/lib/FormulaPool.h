/*
 *  SMT-RAT - Satisfiability-Modulo-Theories Real Algebra Toolbox
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
 * @file FormulaPool.h
 *
 * @author Florian Corzilius
 * @version 2014-05-08
 */
#include "Formula.h"
#include "ConstraintPool.h"
#include <mutex>

#ifndef FORMULAPOOL_H
#define FORMULAPOOL_H

#define SIMPLIFY_FORMULAS

namespace smtrat
{
    class FormulaPool : public carl::Singleton<FormulaPool>
    {
        friend carl::Singleton<FormulaPool>;
        private:
            // Members:
            /// id allocator
            unsigned mIdAllocator;
            /// The unique formula representing true.
            const Formula* mpTrue;
            /// The unique formula representing false.
            const Formula* mpFalse;
            /// The formula pool.
            FastPointerSet<Formula> mFormulas;
            /// Mutex to avoid multiple access to the pool
            mutable std::mutex mMutexPool;
            
            #ifdef SMTRAT_STRAT_PARALLEL_MODE
            #define FORMULA_POOL_LOCK_GUARD std::lock_guard<std::recursive_mutex> lock( mMutexPool );
            #define FORMULA_POOL_LOCK mMutexPool.lock();
            #define FORMULA_POOL_UNLOCK mMutexPool.unlock();
            #else
            #define FORMULA_POOL_LOCK_GUARD
            #define FORMULA_POOL_LOCK
            #define FORMULA_POOL_UNLOCK
            #endif
            
        protected:
            
            /**
             * Constructor of the formula pool.
             * @param _capacity Expected necessary capacity of the pool.
             */
            FormulaPool( unsigned _capacity = 10000 );
            
        public:

            /**
             * Destructor.
             */
            ~FormulaPool();
    
            const Formula* trueFormula()
            {
                return mpTrue;
            }

            const Formula* falseFormula()
            {
                return mpFalse;
            }

            /**
             * @param _booleanVar The Boolean variable wrapped by this formula.
             * @return A formula with wrapping the given Boolean variable.
             */
            const Formula* newFormula( carl::Variable::Arg _booleanVar )
            {
                return addFormulaToPool( new Formula( _booleanVar ) );
            }
            
            /**
             * @param _constraint The constraint wrapped by this formula.
             * @return A formula with wrapping the given constraint.
             */
            const Formula* newFormula( const Constraint* _constraint )
            {
                #ifdef SIMPLIFY_FORMULAS
                if( _constraint == constraintPool().consistentConstraint() )
                    return mpTrue;
                if( _constraint == constraintPool().inconsistentConstraint() )
                    return mpFalse;
                #endif
                return addFormulaToPool( new Formula( _constraint ) );
            }
            
            /**
             * 
             * @param _subformula
             * @return 
             */
            const Formula* newNegation( const Formula* _subformula )
            {
                #ifdef SIMPLIFY_FORMULAS
                if( _subformula == mpTrue )
                    return mpFalse;
                if( _subformula == mpFalse )
                    return mpTrue;
                #endif
                return addFormulaToPool( new Formula( _subformula ) );
            }
    
            /**
             * 
             * @param _subformulaA
             * @param _subformulaB
             * @return 
             */
            const Formula* newImplication( const Formula* _subformulaA, const Formula* _subformulaB )
            {
                #ifdef SIMPLIFY_FORMULAS
                if( _subformulaA == mpFalse )
                    return mpTrue;
                if( _subformulaB == mpTrue )
                    return mpTrue;
                #endif
                return addFormulaToPool( new Formula( _subformulaA, _subformulaB ) );
            }
            
            /**
             * @param _type The type of the n-ary operator (n>1) of the formula to create.
             * @param _subformulaA The first sub-formula of the formula to create.
             * @param _subformulaB The second sub-formula of the formula to create.
             * @return A formula with the given operator and sub-formulas.
             */
            const Formula* newFormula( Type _type, const Formula* _subformulaA, const Formula* _subformulaB )
            {
                PointerSet<Formula> subformulas;
                subformulas.insert( _subformulaA );
                subformulas.insert( _subformulaB );
                return newFormula( _type, std::move( subformulas ) );
            }
            
            /**
             * @param _type The type of the n-ary operator (n>1) of the formula to create.
             * @param _subformulas The sub-formulas of the formula to create.
             * @return A formula with the given operator and sub-formulas.
             */
            const Formula* newFormula( Type _type, const PointerSet<Formula>& _subformulas )
            {
                return newFormula( _type, std::move( PointerSet<Formula>( _subformulas ) ) );
            }
            
            /**
             * @param _type The type of the n-ary operator (n>1) of the formula to create.
             * @param _subformulas The sub-formulas of the formula to create.
             * @return A formula with the given operator and sub-formulas.
             */
            const Formula* newFormula( Type _type, PointerSet<Formula>&& _subformulas );
            
    private:
        
            /**
             * Creates a formula of the given type but with only one sub-formula.
             * @param _type
             * @param _subformula
             * @return True, if the given type is IFF;
             *         False, if the given type is XOR;
             *         The given sub-formula if the type is AND or OR.
             */
            const Formula* newFormulaWithOneSubformula( Type _type, const Formula* _subformula )
            {
                assert( OR || AND || XOR || IFF );
                // We expect that this only happens, if the intended sub-formulas are all the same.
                switch( _type )
                {
                    case XOR: // f xor f is false
                        return mpFalse;
                    case IFF: // f iff f is true
                        return mpTrue;
                    default: // f or f = f; f and f = f
                        return _subformula;
                }
            }
            
            /**
             * Adds the given formula to the pool, if it does not yet occur in there.
             * Note, that this method uses the allocator which is locked before calling.
             * @sideeffect The given formula will be deleted, if it already occurs in the pool.
             * @param _formula The formula to add to the pool.
             * @return The given formula, if it did not yet occur in the pool;
             *         The equivalent formula already occurring in the pool, otherwise.
             */
            const Formula* addFormulaToPool( Formula* _formula );
    };
    
    const Formula* trueFormula();
    
    const Formula* falseFormula();
    
    const Formula* newFormula( carl::Variable::Arg _booleanVar );
    
    const Formula* newFormula( const Constraint* _constraint );
    
    const Formula* newNegation( const Formula* _subformula );
    
    const Formula* newImplication( const Formula* _subformulaA, const Formula* _subformulaB );
    
    const Formula* newFormula( Type _type, const Formula* _subformulaA, const Formula* _subformulaB );
    
    const Formula* newFormula( Type _type, const PointerSet<Formula>& _subformulas );
    
    const Formula* newFormula( Type _type, PointerSet<Formula>&& _subformulas );
}    // namespace smtrat

#endif   /* FORMULAPOOL_H */