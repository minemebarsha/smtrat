#pragma once

#include "BenchmarkSet.h"

#include <benchmax/settings/Settings.h>

#include <vector>

namespace benchmax {

/// Loads benchmark files from directory specified in settings.
std::vector<BenchmarkSet> loadBenchmarks();

namespace settings {

/// Settings for benchmarks.
struct BenchmarkSettings {
	/// Memory limit in megabytes.
	std::size_t limit_memory;
	/// Time limit in seconds.
	std::chrono::seconds limit_time;
	/// Lift of input directories.
	std::vector<std::string> input_directories;
	/// Common prefix of input directories (to shorten filenames in output).
	std::string input_directories_common_prefix;
	/// Output directory.
	std::string output_dir;
	/// Filename of xml file.
	std::string output_file_xml;
};
/// Postprocess benchmark settings.
template<typename V>
inline void finalize_settings(BenchmarkSettings& s, const V& values) {
	s.limit_time = std::chrono::seconds(values["timeout"].template as<std::size_t>());
	s.input_directories_common_prefix = common_prefix(s.input_directories);
}

/// Registers benchmark settings with the settings parser.
void registerBenchmarkSettings(SettingsParser* parser);
} // namespace settings

/// Return the benchmark settings.
inline const auto& settings_benchmarks() {
	return settings_get<settings::BenchmarkSettings>("benchmarks");
}

} // namespace benchmax