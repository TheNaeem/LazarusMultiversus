#pragma once

struct FAESKey
{
	static constexpr int KeySize = 32;
	static constexpr int AESBlockSize = 16;

	uint8_t Key[KeySize];

	FAESKey();
	FAESKey(std::string KeyString);

	bool operator==(const FAESKey& Other) const;
	bool IsValid() const;
	std::string ToString();
};