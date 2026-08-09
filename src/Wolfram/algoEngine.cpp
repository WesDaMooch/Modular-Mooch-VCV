// algoEngine.cpp
// Part of the Modular Mooch Wolfram module (VCV Rack)
//
// GitHub: https://github.com/WesDaMooch/Modular-Mooch-VCV
// 
// Copyright (c) 2026 Wesley Lawrence Leggo-Morrell
// License: GPL-3.0-or-later

#include "algoEngine.hpp"


AlgoEngine::AlgoEngine() = default;
AlgoEngine::~AlgoEngine() = default;


void AlgoEngine::process(const EngineCoreParams& p, EngineOutput& output) {

	// Sequencer
	bool refreshDisplay = p.step;
	bool syncStep = p.sync && p.step;
	generate = rack::random::get<float>() < p.probability;

	if (!p.sync || (syncStep))
		setRuleCv(p.ruleCv);

	bool injectOccured = (p.inject != 0);
	if (injectOccured && p.sync)
		injectPending += p.inject;

	// Non-sync inject
	if (injectOccured && !p.sync) {
		inject(p.inject, p.sync);
		refreshDisplay = true;
	}

	// Reset
	bool seedReset = (p.miniMenuChanged && generate) && !p.sync;

	if (p.miniMenuChanged && p.sync)
		seedResetPending = true;

	if (p.reset && p.sync)
		resetPending = true;

	if (((p.reset || seedReset) && !p.sync) || ((resetPending || seedResetPending) && syncStep)) {
		if (generate) {
			resetToSeed(p.sync);
			generate = false;
		}
		else if (!seedResetPending) {
			if (p.sync) {
				writeHead = 0;
			}
			else {
				readHead = 0;
				writeHead = 1;
			}
		}
		resetPending = false;
		seedResetPending = false;
		refreshDisplay = true;
	}

	// Generate
	if (generate && p.step) {
		onGenerate();
		refreshDisplay = true;
	}

	// Sync inject
	if (injectPending && syncStep) {
		inject(injectPending, p.sync);
		injectPending = 0;
	}

	// Offset
	int newOffset = p.offset - 4;
	if ((!p.sync && (offset != newOffset)) || syncStep) {
		offset = newOffset;
		refreshDisplay = true;
	}

	// Update
	if (refreshDisplay)
		updateDisplay(p.step, p.length);

	// Render output
	renderOutput(output);
	displayMatrixUpdated = false;
}


// Setters
void AlgoEngine::setReadHead(size_t newReadHead) {
    readHead = rack::clamp(newReadHead, 0, MAX_SEQUENCE_LENGTH - 1);
}


void AlgoEngine::setWriteHead(size_t newWriteHead) {
    writeHead = rack::clamp(newWriteHead, 0, MAX_SEQUENCE_LENGTH - 1);
}


// Getters
int AlgoEngine::getReadHead() {
    return static_cast<int>(readHead);
}


int AlgoEngine::getWriteHead() {
    return static_cast<int>(writeHead);
}


void AlgoEngine::getEngineLabel(char out[5]) {
    memcpy(out, engineLabel, 5);
}


// Helpers
uint8_t AlgoEngine::applyOffset(uint8_t inputRow, int inputOffset) {
    int shift = inputOffset % 8;
    if (shift > 3)
        shift -= 8;

    if (shift < 0) {
        shift = -shift;
        inputRow = ((inputRow << shift) | (inputRow >> (8 - shift))) & 0xFF;
    }
    else if (shift > 0) {
        inputRow = ((inputRow >> shift) | (inputRow << (8 - shift))) & 0xFF;
    }
    return inputRow;
}

