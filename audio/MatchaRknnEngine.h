#pragma once

#include <atomic>
#include <string>
#include <vector>

// Initializes and runs the validated Matcha Encoder-768 + Estimator buckets
// + Vocos-600 pipeline. Models remain resident for the lifetime of the
// process, so consecutive LLM speech chunks do not reload RKNN contexts.
bool matchaRknnInitialize(
    const std::string &modelDir,
    std::string *error
);

bool matchaRknnSynthesize(
    const std::string &modelDir,
    const std::string &text,
    std::vector<float> *audio16k,
    const std::atomic<bool> *cancelRequested,
    std::string *error
);
