#pragma once

namespace Finder
{
	void* FindPattern(const char*, bool = false, uint32_t = 0);

	template <typename T>
	void* FindByString(T& string, std::vector<int> opcodesToFind = {}, bool bRelative = false, uint32_t offset = 0, bool forward = false, int skip = 0, DWORD delimiter = 0xCC);
};