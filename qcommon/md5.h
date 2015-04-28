#pragma once

namespace Crypto {

	// compute the MD5 for the given buffer
	void ChecksumMD5(
		const char *in,
		size_t inLen,
		char out[33]
	);

} // namespace Crypto
