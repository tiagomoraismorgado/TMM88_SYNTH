#include <benchmark/benchmark.h>
#include "../include/tmm88/dsp.h"
#include <vector>

static void BM_Oscillator_AVX2(benchmark::State& state) {
    Oscillator osc(WaveformType::Wavetable);
    osc.setSampleRate(44100.0);
    osc.setFrequency(440.0);
    osc.setUnison(8, 0.1); // Max voices for stress test
    osc.setShape(0.5);
    osc.init();

    for (auto _ : state) {
        double out = osc.process(0.0);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Oscillator_AVX2);

static void BM_Oscillator_Scalar(benchmark::State& state) {
    Oscillator osc(WaveformType::Wavetable);
    osc.setSampleRate(44100.0);
    osc.setFrequency(440.0);
    osc.setUnison(8, 0.1);
    osc.setShape(0.5);
    osc.init();

    for (auto _ : state) {
        // Assuming processScalar is added to the Oscillator class
        double out = osc.processScalar(0.0);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Oscillator_Scalar);

static void BM_FullBuffer_AVX2(benchmark::State& state) {
    Oscillator osc(WaveformType::Wavetable);
    osc.init();
    std::vector<double> output(1024);

    for (auto _ : state) {
        osc.processBuffer(nullptr, output.data(), output.size());
        benchmark::DoNotOptimize(output);
    }
    state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_FullBuffer_AVX2);

static void BM_Filter_SoftSaturation_Tanh(benchmark::State& state) {
    ResonantLPF filter;
    filter.setSampleRate(44100.0);
    filter.setCutoff(1000.0);
    filter.setResonance(0.707);
    filter.setSaturationModel(FilterSaturationModel::Soft); // Uses std::tanh
    filter.init();

    // Use a small buffer of noise to avoid branch prediction "cheating"
    std::vector<double> noise(1024);
    for(auto& n : noise) n = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
    size_t i = 0;

    for (auto _ : state) {
        double out = filter.process(noise[i++ % 1024], 0.0);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Filter_SoftSaturation_Tanh);

static void BM_Filter_NoSaturation_Linear(benchmark::State& state) {
    ResonantLPF filter;
    filter.setSampleRate(44100.0);
    filter.setCutoff(1000.0);
    filter.setResonance(0.707);
    // Trigger the default case in setSaturationModel which is linear (x * d)
    filter.setSaturationModel(static_cast<FilterSaturationModel>(999)); 
    filter.init();

    std::vector<double> noise(1024);
    for(auto& n : noise) n = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
    size_t i = 0;

    for (auto _ : state) {
        double out = filter.process(noise[i++ % 1024], 0.0);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Filter_NoSaturation_Linear);

BENCHMARK_MAIN();