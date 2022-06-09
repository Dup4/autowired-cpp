#include <benchmark/benchmark.h>

#include "autowired-cpp/hello_world.h"

static void BenchmarkHelloWorld(benchmark::State& state) {
    for (auto _ : state) {
        HelloWorld();
    }
}

BENCHMARK(BenchmarkHelloWorld);
