#pragma once

// Base64, in the lowest layer that needs it.
//
// Binary parameter values (blobs) have to survive a round-trip through XML
// preset files, which are text. This lives in GMPI/Core rather than in a
// host or a plug-in library because both sides of that round-trip - the
// preset writer in Hosting/processor_holder.cpp and the reader in
// Hosting/controller_holder.h - are below any of them.
//
// SynthEditLib/Base64.h predates this and now forwards here, so there is one
// implementation rather than two.

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gmpi
{

inline constexpr std::string_view base64Alphabet =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline std::string base64Encode(std::span<const uint8_t> data)
{
	std::string out;
	out.reserve(((data.size() + 2) / 3) * 4);

	size_t i = 0;
	while (i + 2 < data.size())
	{
		const uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
		out += base64Alphabet[(n >> 18) & 0x3f];
		out += base64Alphabet[(n >> 12) & 0x3f];
		out += base64Alphabet[(n >> 6) & 0x3f];
		out += base64Alphabet[n & 0x3f];
		i += 3;
	}

	// Tail: one or two bytes, padded with '='. Encoding the remainder as if
	// the missing bytes were zero is what makes the padding count meaningful.
	if (i < data.size())
	{
		const bool haveTwo = (i + 1 < data.size());
		const uint32_t n = (data[i] << 16) | (haveTwo ? (data[i + 1] << 8) : 0);

		out += base64Alphabet[(n >> 18) & 0x3f];
		out += base64Alphabet[(n >> 12) & 0x3f];
		out += haveTwo ? base64Alphabet[(n >> 6) & 0x3f] : '=';
		out += '=';
	}

	return out;
}

inline std::vector<uint8_t> base64Decode(std::string_view text)
{
	// Reverse lookup built once. 0xff marks "not a base64 character", which
	// covers padding and any whitespace a file may have picked up.
	static const auto reverse = []
		{
			std::array<uint8_t, 256> t{};
			t.fill(0xff);
			for (uint8_t i = 0; i < static_cast<uint8_t>(base64Alphabet.size()); ++i)
				t[static_cast<uint8_t>(base64Alphabet[i])] = i;
			return t;
		}();

	std::vector<uint8_t> out;
	out.reserve((text.size() / 4) * 3);

	uint32_t accumulator = 0;
	int bitsHeld = 0;

	for (const char c : text)
	{
		const uint8_t v = reverse[static_cast<uint8_t>(c)];
		if (v == 0xff)
			continue; // '=' or stray whitespace

		accumulator = (accumulator << 6) | v;
		bitsHeld += 6;

		if (bitsHeld >= 8)
		{
			bitsHeld -= 8;
			out.push_back(static_cast<uint8_t>((accumulator >> bitsHeld) & 0xff));
		}
	}

	return out;
}

// Convenience for callers holding text rather than bytes.
inline std::string base64Encode(std::string_view text)
{
	return base64Encode(std::span<const uint8_t>(
		reinterpret_cast<const uint8_t*>(text.data()), text.size()));
}

} // namespace gmpi
