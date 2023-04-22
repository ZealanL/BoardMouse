#include "Transpos.h"

TransposBucket* Transpos::buckets = NULL;
size_t Transpos::bucketCount;

void Transpos::Init(size_t bucketCount) {
	Transpos::bucketCount = bucketCount;

	size_t memoryMBs = (size_t)ceil(bucketCount / (float)TRANSPOS_BUCKET_COUNT_MB);
	buckets = new TransposBucket[bucketCount];

	if (!buckets)
		ERR_CLOSE("Failed to allocate memory for transposition table (need " << memoryMBs << "MB)");
}

void Transpos::Resize(size_t bucketCount) {
	delete[] buckets;
	Init(bucketCount);
}

void Transpos::Reset() {
	Resize(bucketCount);
}