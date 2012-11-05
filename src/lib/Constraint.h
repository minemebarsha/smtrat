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
 * Constraint.h
 * @author Florian Corzilius
 * @author Sebastian Junges
 * @since 2010-04-26
 * @version 2012-10-12
 */


#ifndef SMTRAT_TS_CONSTRAINT_H
#define SMTRAT_TS_CONSTRAINT_H

//#define SMTRAT_TS_CONSTRAINT_SIMPLIFIER
//#define NDEBUG
#define VS_USE_GINAC_EXPAND
//#define VS_USE_GINAC_NORMAL

#include <ginac/ginac.h>
#include <ginac/flags.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <string.h>
#include <sstream>
#include <assert.h>

namespace smtrat
{
    //
    // Type and object definitions
    //

    /*
     * The expected number of variables occurring in the constraint.
     */
    const unsigned VS_EXPECTED_NUMBER_OF_VARIABLES = 10;

    enum Constraint_Relation
    {
        CR_EQ = 0, CR_NEQ = 1, CR_LESS = 2, CR_GREATER = 3, CR_LEQ = 4, CR_GEQ = 5
    };

    bool constraintRelationIsStrict( Constraint_Relation rel );
    std::string relationToString( const Constraint_Relation rel );

    struct strCmp
    {
        bool operator ()( const std::string& _stringA, const std::string& _stringB ) const
        {
            return _stringA.compare( _stringB ) < 0;
        }
    };

    struct VarInfo
    {
        unsigned maxDegree;
        unsigned minDegree;
        unsigned occurences;
    };

    typedef std::map< std::string, VarInfo, strCmp > VarInfoMap;

    typedef std::pair< std::string, signed > VarDegree;

    struct varDegreeCmp
    {
        bool operator ()( const VarDegree& varDegreeA, const VarDegree& varDegreeB ) const
        {
            signed result = varDegreeA.first.compare( varDegreeB.first );
            if( result < 0 ) return true;
            else if ( result == 0 ) return varDegreeA.second < varDegreeB.second;
        }
    };

    typedef std::map< VarDegree, GiNaC::ex*, varDegreeCmp > Coefficients;

    /**
     * Class to create a constraint object.
     * @author Florian Corzilius
     * @since 2010-04-26
     * @version 2011-12-05
     */
    class Constraint
    {
        private:
            /*
             * Attributes:
             */
            bool                 mSumOfSquares;
            unsigned             mID;
            unsigned             mNumMonomials;
            Constraint_Relation  mRelation;
            GiNaC::ex*           pLhs;
            GiNaC::ex*           mpMultiRootLessLhs;
            GiNaC::symtab        mVariables;
            VarInfoMap           mVarInfoMap;
            Coefficients         mCoefficients;

        public:

            /*
             * Constructors:
             */
            Constraint();
            Constraint( const GiNaC::ex&, const Constraint_Relation, const GiNaC::symtab&, unsigned = 0 );
            Constraint( const GiNaC::ex&, const GiNaC::ex&, const Constraint_Relation&, const GiNaC::symtab&, unsigned = 0 );
            Constraint( const Constraint& );

            /*
             * Destructor:
             */
            ~Constraint();

            /*
             * Methods:
             */
            GiNaC::ex& rLhs()
            {
                return *pLhs;
            }

            const GiNaC::ex& lhs() const
            {
                return *pLhs;
            }

            GiNaC::symtab& rVariables()
            {
                return mVariables;
            }

            const GiNaC::symtab& variables() const
            {
                return mVariables;
            }

            Constraint_Relation& rRelation()
            {
                return mRelation;
            }

            Constraint_Relation relation() const
            {
                return mRelation;
            }

            unsigned id() const
            {
                return mID;
            }

            const GiNaC::ex* pMultiRootLessLhs() const
            {
                return mpMultiRootLessLhs;
            }

            const GiNaC::ex& multiRootLessLhs() const
            {
                return *mpMultiRootLessLhs;
            }

            static void normalize( GiNaC::ex& _exp )
            {
                #ifdef VS_USE_GINAC_NORMAL
                #ifdef VS_USE_GINAC_EXPAND
                _exp = _exp.expand().normal();
                #else
                _exp = _exp.normal();
                #endif
                #else
                #ifdef VS_USE_GINAC_EXPAND
                _exp = _exp.expand();
                #endif
                #endif
            }

            // Data access methods (read only).
            bool variable( const std::string&, GiNaC::symbol& ) const;
            bool hasVariable( const std::string& ) const;
            unsigned isConsistent() const;
            unsigned satisfiedBy( GiNaC::exmap& ) const;
            bool hasFinitelyManySolutionsIn( const std::string& ) const;
            void getCoefficients( const GiNaC::symbol&, std::vector<GiNaC::ex>& ) const;
            signed degree( const std::string& ) const;
            signed highestDegree() const;
            GiNaC::numeric constantPart() const;
            bool isLinear() const;
            unsigned maxDegree() const;
            std::map<const std::string, GiNaC::numeric, strCmp> linearAndConstantCoefficients() const;
            void init();
            static int exCompare( const GiNaC::ex&, const GiNaC::symtab&, const GiNaC::ex&, const GiNaC::symtab& );

            // Data access methods (read and write).
            const GiNaC::ex& multiRootLessLhs( const std::string& ) const;

            // Operators.
            bool operator <( const Constraint& ) const;
            bool operator ==( const Constraint& ) const;
            friend std::ostream& operator <<( std::ostream&, const Constraint& );

            // Manipulating methods.
            void simplify();

            // Printing methods.
            std::string toString() const;
            void print( std::ostream& _out = std::cout ) const;
            void print2( std::ostream& _out = std::cout ) const;
            void printInPrefix( std::ostream& _out = std::cout ) const;
            const std::string prefixStringOf( const GiNaC::ex& ) const;
            std::string smtlibString() const;

            //
            static signed compare( const Constraint&, const Constraint& );
            static const Constraint* mergeConstraints( const Constraint*, const Constraint* );
            static bool combineConstraints( const Constraint&, const Constraint&, const Constraint& );
        private:
            void getVariables( const GiNaC::ex&, GiNaC::symtab& );
    };

    typedef std::vector<const Constraint*>                                vec_const_pConstraint;
    typedef std::vector<std::set<const Constraint*> >                     vec_set_const_pConstraint;
    typedef std::map<const Constraint* const , vec_set_const_pConstraint> constraintOriginsMap;
    struct constraintPointerComp
    {
        bool operator ()( const Constraint* const pConstraintA, const Constraint* const pConstraintB ) const
        {
            return (*pConstraintA) < (*pConstraintB);
        }
    };
    typedef std::set< const Constraint*, constraintPointerComp > ConstraintSet;
}    // namespace smtrat

#endif
