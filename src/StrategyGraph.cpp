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
 * @file StrategyGraph.cpp
 *
 * @author  Henrik Schmitz
 * @since   2012-09-10
 * @version 2012-09-19
 */

#include "StrategyGraph.h"

using namespace std;

namespace smtrat{
    unsigned StrategyGraph::Edge::mIdAllocator = 0;
    
    StrategyGraph::Edge::Edge(){}
    
    StrategyGraph::Edge::Edge( unsigned _to, ConditionEvaluation _condition ):
        mId( mIdAllocator++ ),
        mSuccessor( _to ),
        mpConditionEvaluation( _condition )
    {}
    
    StrategyGraph::Edge::~Edge(){}


    StrategyGraph::Vertex::Vertex():
        mModuleType( MT_Module ),
        mpEdgeList( new vector<Edge>() )
    {}

    StrategyGraph::Vertex::Vertex( ModuleType _moduleType ):
        mModuleType( _moduleType ),
        mpEdgeList( new vector<Edge>() )
    {}
    
    StrategyGraph::Vertex::~Vertex(){}
    

    StrategyGraph::StrategyGraph():
        mStrategyGraph()
    {
        mStrategyGraph.push_back( Vertex() );
    }

    StrategyGraph::~StrategyGraph(){}

    /**
     * ...
     *
     * @param formula       The formula to be considered.
     *
     * @return void
     */
    bool StrategyGraph::Vertex::successorExists( unsigned _to ) const
    {
        for( auto edge = mpEdgeList->begin(); edge!=mpEdgeList->end(); ++edge )
        {
            if ( edge->successor()==_to )
                return true;
        }
        return false;
    }

    /**
     * ...
     *
     * @param formula       The formula to be considered.
     *
     * @return void
     */
    void StrategyGraph::Vertex::addSuccessor( unsigned _to, ConditionEvaluation _condition )
    {
        assert( !successorExists( _to ) );
        mpEdgeList->push_back( Edge( _to, _condition ) );
    }

    /**
     * ...
     *
     * @param formula       The formula to be considered.
     *
     * @return void
     */
    unsigned StrategyGraph::addModuleType( unsigned _at, ModuleType _moduleType, ConditionEvaluation _condition )
    {
        Vertex successor = Vertex( _moduleType );
        mStrategyGraph.push_back( successor );
        
        addCondition( _at, mStrategyGraph.size()-1, _condition );
        
        return mStrategyGraph.size()-1;
    }

    /**
     * ...
     *
     * @param formula       The formula to be considered.
     *
     * @return void
     */
    void StrategyGraph::addCondition( unsigned _from, unsigned _to, ConditionEvaluation _condition )
    {
        mStrategyGraph[_from].addSuccessor( _to, _condition );
    }

    /**
     * ...
     *
     * @param formula       The formula to be considered.
     *
     * @return void
     */
    vector< pair<unsigned, ModuleType> > StrategyGraph::nextModuleTypes( unsigned _from, Condition _condition )
    {
        vector< pair<unsigned, ModuleType> > result = vector< pair<unsigned, ModuleType> >();
        
        const vector<Edge>& edges = mStrategyGraph[_from].edgeList();
        for( auto edge = edges.begin(); edge!=edges.end(); ++edge )
        {
            if ( edge->conditionEvaluation()( _condition ) )
            {
                unsigned succ = edge->successor();
                result.push_back( pair<unsigned, ModuleType>( succ, mStrategyGraph[succ].moduleType() ) );
            }
        }
        return result;
    }

}    // namespace smtrat