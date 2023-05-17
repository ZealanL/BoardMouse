#include "Transpos.h"

void TransposTable::Init(size_t newSize) {
	buckets.resize(newSize);
	Reset();
}

void TransposTable::Reset() {
	for (TransposBucket& bucket : buckets)
		bucket = TransposBucket();
}

void TransposTable::MarkOld() {
#ifdef _DEBUG
	LOG("Marking old transpos table entries...");

	size_t numDecreased = 0, numReset = 0;
#endif

	for (auto& bucket : buckets) {
		for (auto& entry : bucket.entries) {
			if (!entry.IsValid())
				continue;

			if (entry.depth > TRANSPOS_OLD_DEPTH_DECREASE) {
				entry.depth -= TRANSPOS_OLD_DEPTH_DECREASE;
#ifdef _DEBUG
				numDecreased++;
#endif
			} else {
				// Depth is <= decrease, just reset it
				entry.Reset();
#ifdef _DEBUG
				numReset++;
#endif
			}
		}
	}

#ifdef _DEBUG
	LOG(" > Decreased: " << numDecreased);
	LOG(" > Reset: " << numReset);
#endif
}

TransposTable
Transpos::main = {};