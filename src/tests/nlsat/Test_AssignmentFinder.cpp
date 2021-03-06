#include <boost/test/unit_test.hpp>

#include <smtrat-modules/SATModule/mcsat/BaseBackend.h>
#include <smtrat-common/smtrat-common.h>

using namespace smtrat;

BOOST_AUTO_TEST_SUITE(Test_AssignmentFinder);

/**
 * Tests Example 3 from the NLSAT-Paper
 */
BOOST_AUTO_TEST_CASE(Test_NLSATPaper_Ex3)
{
	
	carl::Variable x = carl::freshRealVariable("x");
	carl::Variable y = carl::freshRealVariable("y");
	
	// Original constraints
	FormulaT c1(Poly(x)+Rational(1), carl::Relation::LEQ);
	FormulaT c2(Poly(x)-Rational(1), carl::Relation::GEQ);
	FormulaT c3(Poly(x)+y, carl::Relation::GREATER);
	FormulaT c4(Poly(x)-y, carl::Relation::GREATER);
	// Learnt from first conflict
	FormulaT c5(Poly(x), carl::Relation::LESS);
	
	// Explanation for first conflict
	FormulaT ex1(carl::FormulaType::OR, { c3.negated(), c4.negated(), c5.negated() });
	// Explanation for second conflict
	FormulaT ex2(carl::FormulaType::OR, { c1.negated(), c5 });
	
	mcsat::MCSATBackend<mcsat::MCSATSettingsNL> nlsat;
	
	// Decide x+1<=0
	SMTRAT_LOG_INFO("smtrat.test.nlsat", "Decide " << c1);
	nlsat.pushConstraint(c1);
	
	// T-Decide
	SMTRAT_LOG_INFO("smtrat.test.nlsat", "T-Decide " << x << " = -2");
	{
		auto res = nlsat.findAssignment(x);
		BOOST_CHECK(carl::variant_is_type<mcsat::ModelValues>(res));
		auto value = boost::get<mcsat::ModelValues>(res);
		BOOST_CHECK(value.size() == 1);
		BOOST_CHECK(value[0].first == x);
		BOOST_CHECK(value[0].second == ModelValue(Rational(-1)));
	}
	FormulaT fx(Poly(x) + Rational(2), carl::Relation::EQ);
	nlsat.pushAssignment(x, Rational(-2), fx);
	
	// Propagate x+y>0
	SMTRAT_LOG_INFO("smtrat.test.nlsat", "Propagate " << c3);
	nlsat.pushConstraint(c3);
	
	// Check whether x-y>0 is feasible, propagate x-y<=0
	SMTRAT_LOG_INFO("smtrat.test.nlsat", "T-Propagate " << c4.negated());
	{
		auto res = nlsat.isInfeasible(y, c4);
		BOOST_CHECK(carl::variant_is_type<FormulasT>(res));
		auto r = boost::get<FormulasT>(res);
		auto explanation = nlsat.explain(y, r);
		BOOST_CHECK(boost::get<FormulaT>(&explanation) != nullptr);
		BOOST_CHECK(ex1 == boost::get<FormulaT>(explanation));
	}
	nlsat.pushConstraint(c4.negated());
	
	// Backtrack assignment
	SMTRAT_LOG_INFO("smtrat.test.nlsat", "Backtrack " << x);
	nlsat.popAssignment(x);
	
	// Check whether x>0 is feasible, propagate x<=0
	SMTRAT_LOG_INFO("smtrat.test.nlsat", "T-Propagate " << c5);
	{
		auto res = nlsat.isInfeasible(x, c5.negated());
		BOOST_CHECK(carl::variant_is_type<FormulasT>(res));
		auto r = boost::get<FormulasT>(res);
		auto explanation = nlsat.explain(x, r);
		BOOST_CHECK(boost::get<FormulaT>(&explanation) != nullptr);
		BOOST_CHECK(ex2 == boost::get<FormulaT>(explanation));
	}
}

BOOST_AUTO_TEST_CASE(CoverComputation)
{
	carl::Variable a = carl::freshRealVariable("a");
	carl::Variable b = carl::freshRealVariable("b");
	Poly p = Poly(20)*a*a*b - Poly(a)*a*a*a*b - Poly(120)*b;
	carl::UnivariatePolynomial<Rational> q(a, {
		Rational(900),
		Rational(0),
		Rational(-2550),
		Rational(0),
		Rational(1055)/2,
		Rational(0),
		Rational(-40),
		Rational(0),
		Rational(1)
	});
	carl::Interval<Rational> i(Rational(633)/1025, carl::BoundType::STRICT, Rational(2533)/4096, carl::BoundType::STRICT);
	FormulaT f(p, carl::Relation::LESS);
	
	std::cout << "Constructing RAN on " << q << " / " << i << std::endl;
	carl::RealAlgebraicNumber<Rational> ran(q, i);
	std::cout << "-> " << ran << std::endl;
	
	Model model;
	model.assign(a, ran);
	model.assign(b, Rational(-3));
	
	auto res = carl::model::evaluate(f, model);
	std::cout << f << " on " << model << " -> " << res << std::endl;
	
	model.assign(b, Rational(1));
	res = carl::model::evaluate(f, model);
	std::cout << f << " on " << model << " -> " << res << std::endl;
}

BOOST_AUTO_TEST_SUITE_END();
