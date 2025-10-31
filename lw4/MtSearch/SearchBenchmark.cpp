#include "MtSearch.h"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#define CATCH_CONFIG_ENABLE_BENCHMARKING

TEST_CASE("Search benchmark")
{
	std::stringstream input;
	std::stringstream output;

	const std::string dirUrl = "/home/dmitriy.rybakov/projects/crm/crm-app/lib"; // 39380

	std::vector<double> results;

	for (int i : { 16, 8, 4, 2, 1 })
	{
		MtSearch search(input, output, i);
		search.AddDirToIndex(dirUrl, true);

		BENCHMARK_ADVANCED("Search with " + std::to_string(i) + " threads")(Catch::Benchmark::Chronometer meter)
		{
			meter.measure([&] {
				search.FindMostRelevantDocIds({
					"deal",
					"lead",
					"qualify" });
			});
		};
	}
}
// TODO оценить степень параллелизма (закон Амдала)

TEST_CASE("Index benchmark")
{
	std::stringstream input;
	std::stringstream output;

	const std::string dirUrl = "/home/dmitriy.rybakov/projects/crm/crm-app/lib"; // 39380

	std::vector<double> results;
	for (int i : { 16, 8, 4, 2, 1 })
	{
		MtSearch search(input, output, i);

		BENCHMARK_ADVANCED("AddWordToIndex with " + std::to_string(i) + " threads")(Catch::Benchmark::Chronometer meter)
		{
			meter.measure([&] {
				search.AddDirToIndex(dirUrl, true);
			});
		};
	}
}
