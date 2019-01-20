/**
 * Class to create the formulas for axioms.
 * @author Aklima Zaman
 * @since 2018-09-24
 * @version 2018-09-24
 */

#pragma once


#include "../dto/VariableCapsule.h"
#include "lib/Common.h"
#include "../Util.h"
#include "../../../smtrat-common/model.h"


namespace smtrat {

    class AxiomFactory {

    public:
        enum AxiomType { ZERO, TANGENT_PLANE, MONOTONICITY, CONGRUENCE };
        static FormulasT createFormula(AxiomType axiomType, Model abstractModel, MonomialMap monomialMap);
    };

}