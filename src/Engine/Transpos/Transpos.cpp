#include "Transpos.h"

void TransposTable::Init(size_t newSize) {
	buckets.resize(newSize);
	Reset();
}

void TransposTable::Reset() {
	for (TransposBucket& bucket : buckets)
		bucket = TransposBucket();
}

TransposTable
Transpos::main = {};