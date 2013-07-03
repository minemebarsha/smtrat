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


/*
 * File:   variable.h
 * Author: Stefan Schupp
 *
 * Created on January 15, 2013, 10:40 AM
 */

#ifndef VARIABLE_H
#define VARIABLE_H

#include "ContractionCandidate.h"
#include "../../Formula.h"
#include <assert.h>

namespace smtrat
{
namespace icp
{
    class IcpVariable
    {
        private:

            /*
             * Members
             */
            const GiNaC::symbol                              mVar;
            bool                                       mOriginal;
            std::vector<ContractionCandidate*> mCandidates;
            lra::Variable<lra::Numeric>*                             mLraVar;
            bool                                       mActive;
            bool                                       mLinear;
            
            // interval Bound generation
            std::pair<bool,bool>                       mBoundsSet; // internal, external
            std::pair<bool,bool>                       mUpdated; // internal, external
            smtrat::Formula*                           mInternalLeftBound;
            smtrat::Formula*                           mInternalRightBound;
            smtrat::Formula::iterator                  mExternalLeftBound;
            smtrat::Formula::iterator                  mExternalRightBound;

        public:

            /*
             * Constructors
             */
            
            IcpVariable()
            {
                assert(false);
            }

            IcpVariable( symbol _var, bool _original, lra::Variable<lra::Numeric>* _lraVar = NULL ):
                mVar( _var ),
                mOriginal( _original ),
                mCandidates(),
                mLraVar( _lraVar ),
                mActive( false ),
                mLinear( true ),
                mBoundsSet( std::make_pair(false,false) ),
                mUpdated( std::make_pair(false,false) ),
                mInternalLeftBound ( ),
                mInternalRightBound ( ),
                mExternalLeftBound ( ),
                mExternalRightBound ( )
            {
            }

            IcpVariable( symbol _var,
                         bool _original,
                         ContractionCandidate* _candidate,
                         lra::Variable<lra::Numeric>* _lraVar = NULL ):
                mVar( _var ),
                mOriginal ( _original ),
                mCandidates(),
                mLraVar( _lraVar ),
                mActive( _candidate->isActive() ),
                mLinear( _candidate->isLinear() ),
                mBoundsSet (std::make_pair(false,false)),
                mUpdated( std::make_pair(false,false) ),
                mInternalLeftBound ( ),
                mInternalRightBound ( ),
                mExternalLeftBound ( ),
                mExternalRightBound ( )
            {
                mCandidates.insert( mCandidates.end(), _candidate );
            }
            
            ~IcpVariable()
            {
                if( mBoundsSet.first )
                {
                    delete mInternalLeftBound;
                    delete mInternalRightBound;
                }
            }

            /*
             * Getter/Setter
             */

            const GiNaC::symbol var() const
            {
                return mVar;
            }

            std::vector<ContractionCandidate*>& candidates()
            {
                return mCandidates;
            }

            const lra::Variable<lra::Numeric>* lraVar() const
            {
                return mLraVar;
            }

            void addCandidate( ContractionCandidate* _candidate )
            {
                mCandidates.insert( mCandidates.end(), _candidate );
                if( _candidate->isActive() )
                {
                    mActive = true;
                }
            }

            void setLraVar( lra::Variable<lra::Numeric>* _lraVar )
            {
                mLraVar = _lraVar;
                mUpdated = std::make_pair(true,true);
            }

            void deleteCandidate( ContractionCandidate* _candidate )
            {
                std::vector<ContractionCandidate*>::iterator candidateIt;
                for( candidateIt = mCandidates.begin(); candidateIt != mCandidates.end(); )
                {
                    if( *candidateIt == _candidate )
                    {
                        candidateIt = mCandidates.erase( candidateIt );
                    }
                    else
                    {
                        ++candidateIt;
                    }
                }
            }

            void print( ostream& _out = std::cout ) const
            {
                _out << "Original: " << mOriginal << ", " << mVar << ", ";
                if( mLinear && (mLraVar != NULL) )
                {
                    mLraVar->print();
                }
                _out << endl;
            }

            bool isActive() const
            {
                return mActive;
            }

            void activate()
            {
                mActive = true;
            }

            void deactivate()
            {
                mActive = false;
            }

            bool checkLinear()
            {
                std::vector<ContractionCandidate*>::iterator candidateIt = mCandidates.begin();
                for ( ; candidateIt != mCandidates.end(); ++candidateIt )
                {
                    if ( (*candidateIt)->isLinear() == false )
                    {
                        mLinear = false;
                        return false;
                    }
                }
                mLinear = true;
                return true;
            }
            
            bool isLinear()
            {
                return mLinear;
            }
            
            void setUpdated(bool _internal=true, bool _external=true)
            {
                mUpdated = std::make_pair(_internal,_external);
            }
            
            bool isInternalUpdated() const
            {
                return mUpdated.first;
            }
            
            bool isExternalUpdated() const
            {
                return mUpdated.second;
            }
            
            smtrat::Formula* internalLeftBound() const
            {
                assert(mBoundsSet.first);
                return mInternalLeftBound;
            }
            
            smtrat::Formula* internalRightBound() const
            {
                assert(mBoundsSet.first);
                return mInternalRightBound;
            }
            
            smtrat::Formula::iterator externalLeftBound() const
            {
                assert(mBoundsSet.second);
                return mExternalLeftBound;
            }
            
            smtrat::Formula::iterator externalRightBound() const
            {
                assert(mBoundsSet.second);
                return mExternalRightBound;
            }
            
            void setInternalLeftBound( smtrat::Formula* _left )
            {
                smtrat::Formula* toDelete = mInternalLeftBound;
                mInternalLeftBound = _left;
                delete toDelete;
            }
            
            void setInternalRightBound( smtrat::Formula* _right )
            {
                smtrat::Formula* toDelete = mInternalRightBound;
                mInternalRightBound = _right;
                delete toDelete;
            }
            
            void setExternalLeftBound( smtrat::Formula::iterator _left )
            {
                mExternalLeftBound = _left;
            }
            
            void setExternalRightBound( smtrat::Formula::iterator _right )
            {
                mExternalRightBound = _right;
            }
            
            void boundsSet(bool _internal=true, bool _external=true)
            {
                mBoundsSet = std::make_pair(_internal,_external);
            }
            
            void internalBoundsSet(bool _internal=true)
            {
                mBoundsSet = std::make_pair(_internal,mBoundsSet.second);
                mUpdated = std::pair<bool,bool>(false, mUpdated.second);
            }
            
            void externalBoundsSet(bool _external=true)
            {
                mBoundsSet = std::make_pair(mBoundsSet.first,_external);
                mUpdated = std::pair<bool,bool>(mUpdated.first, false);
            }
            
            bool isInternalBoundsSet() const
            {
                return mBoundsSet.first;
            }
            
            bool isExternalBoundsSet() const
            {
                return mBoundsSet.second;
            }
            
            const bool isOriginal() const
            {
                return mOriginal;
            }
            
            /**
             * Checks all candidates if at least one is active - if so, set variable as active.
             * @return true if variable is active
             */
            bool autoActivate()
            {
                std::vector<ContractionCandidate*>::iterator candidateIt;
                for( candidateIt = mCandidates.begin(); candidateIt != mCandidates.end(); ++candidateIt )
                {
                    if( (*candidateIt)->isActive() )
                    {
                        mActive = true;
                        return mActive;
                    }
                }
                return false;
            }

        private:

            /*
             * Auxiliary functions
             */
    };
}    // namespace icp
} // namespace smtrat
#endif   /* VARIABLE_H */
