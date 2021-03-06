#pragma once

#include <lib/datastructures/mcsat/fm/Explanation.h>

#include <smtrat-mcsat/assignments/arithmetic/AssignmentFinder.h>
#include <smtrat-mcsat/assignments/smtaf/AssignmentFinder.h>
#include <smtrat-mcsat/assignments/SequentialAssignment.h>
#include <smtrat-mcsat/explanations/ParallelExplanation.h>
#include <smtrat-mcsat/explanations/SequentialExplanation.h>
#include <smtrat-mcsat/explanations/icp/Explanation.h>
#include <smtrat-mcsat/explanations/nlsat/Explanation.h>
#include <smtrat-mcsat/explanations/onecellcad/Explanation.h>
#include <smtrat-mcsat/explanations/vs/Explanation.h>
#include <smtrat-mcsat/variableordering/VariableOrdering.h>

namespace smtrat {
namespace mcsat {

struct MCSATSettingsNL {
	static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
	using AssignmentFinderBackend = arithmetic::AssignmentFinder;
//	using AssignmentFinderBackend = SequentialAssignment<smtaf::AssignmentFinder<smtaf::DefaultSettings>,arithmetic::AssignmentFinder>;
	using ExplanationBackend = SequentialExplanation<nlsat::Explanation>;
};

//OneCell only
struct MCSATSettingsOC {
  static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
	using AssignmentFinderBackend = arithmetic::AssignmentFinder;
  using ExplanationBackend = SequentialExplanation<onecellcad::Explanation, nlsat::Explanation>;
};
struct MCSATSettingsFMVSOC {
  static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
  using AssignmentFinderBackend = arithmetic::AssignmentFinder;
	using ExplanationBackend = SequentialExplanation<fm::Explanation<fm::DefaultSettings>,vs::Explanation,onecellcad::Explanation, nlsat::Explanation>;
};

struct MCSATSettingsFMNL {
	static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
	using AssignmentFinderBackend = arithmetic::AssignmentFinder;
	// using AssignmentFinderBackend = SequentialAssignment<smtaf::AssignmentFinder<smtaf::DefaultSettings>,arithmetic::AssignmentFinder>;
	using ExplanationBackend = SequentialExplanation<fm::Explanation<fm::DefaultSettings>,nlsat::Explanation>;
};

struct MCSATSettingsVSNL {
	static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
	using AssignmentFinderBackend = arithmetic::AssignmentFinder;
	using ExplanationBackend = SequentialExplanation<vs::Explanation,nlsat::Explanation>;
};

struct MCSATSettingsFMVSNL {
	static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
	using AssignmentFinderBackend = arithmetic::AssignmentFinder;
	using ExplanationBackend = SequentialExplanation<fm::Explanation<fm::DefaultSettings>,vs::Explanation,nlsat::Explanation>;
};

struct MCSATSettingsICPNL {
	static constexpr VariableOrdering variable_ordering = VariableOrdering::FeatureBased;
	using AssignmentFinderBackend = arithmetic::AssignmentFinder;
	using ExplanationBackend = SequentialExplanation<icp::Explanation,nlsat::Explanation>;
};

}
}
