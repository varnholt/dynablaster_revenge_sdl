#pragma once

#include <string>

//! renders one effect in a fixed, deterministic scene and writes PNG captures - see
//! tools/effect-lab/SPEC.md for the scenario shared with the old client
int runEffectLab(const std::string& effect, const std::string& out_dir);
