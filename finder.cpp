#include "pch.h"
#include "finder.h"

namespace ASM
{
	constexpr DWORD JZ = 0x74;
	constexpr DWORD JMP_REL8 = 0xEB;
	constexpr DWORD CALL = 0xE8;
	constexpr DWORD LEA = 0x8D;
	constexpr DWORD CDQ = 0x99;
	constexpr DWORD CMOVL = 0x4C;
	constexpr DWORD CMOVS = 0x48;
	constexpr DWORD INT3 = 0xCC;
	constexpr DWORD RETN = 0xC3;
	constexpr DWORD SKIP = 0xC4;
	constexpr DWORD ADD = 0x86;
	constexpr DWORD XOR_32 = 0x45;
	constexpr DWORD CALL_QWORD = 0xFF;
}

static constexpr bool isAscii[0x100] = { false, false, false, false, false, false, false, false, false, true, true, false, false, true, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
										 true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
										 true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
										 true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
										 true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
										 true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
										 false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

void* Finder::FindPattern(const char* signature, bool bRelative /*= false*/, uint32_t offset /*= 0*/)
{
	uintptr_t base_address = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
	static auto patternToByte = [](const char* pattern)
	{
		auto bytes = std::vector<int>{};
		const auto start = const_cast<char*>(pattern);
		const auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current)
		{
			if (*current == '?')
			{
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			}
			else { bytes.push_back(strtoul(current, &current, 16)); }
		}
		return bytes;
	};

	const auto dosHeader = (PIMAGE_DOS_HEADER)base_address;
	const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)base_address + dosHeader->e_lfanew);

	const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = patternToByte(signature);
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(base_address);

	const auto s = patternBytes.size();
	const auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i)
	{
		bool found = true;
		for (auto j = 0ul; j < s; ++j)
		{
			if (scanBytes[i + j] != d[j] && d[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			uintptr_t address = reinterpret_cast<uintptr_t>(&scanBytes[i]);
			if (bRelative)
			{
				address = ((address + offset + 4) + *(int32_t*)(address + offset));
				return (void*)address;
			}
			return (void*)address;
		}
	}

	return NULL;
}

template <typename T = std::wstring>
static void* FindStringRef(T& string)
{
	auto modBase = (intptr_t)GetModuleHandle(nullptr);

	bool bIsWide = std::is_same<T, std::wstring>::value;

	IMAGE_SECTION_HEADER* textSectionPtr = nullptr;
	IMAGE_SECTION_HEADER* rdataSectionPtr = nullptr;

	const auto dosHeader = (PIMAGE_DOS_HEADER)modBase;
	const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)modBase + dosHeader->e_lfanew);
	auto sectionsSize = ntHeaders->FileHeader.NumberOfSections;
	auto section = IMAGE_FIRST_SECTION(ntHeaders);

	for (WORD i = 0; i < sectionsSize; section++)
	{
		auto secName = std::string((char*)section->Name);

		if (secName == ".text")
		{
			textSectionPtr = section;
		}
		else if (secName == ".rdata")
		{
			rdataSectionPtr = section;
		}

		if (textSectionPtr && rdataSectionPtr)
			break;
	}

	auto textSection = *textSectionPtr;
	auto rdataSection = *rdataSectionPtr;

	if (textSection.Misc.VirtualSize < 0 && rdataSection.Misc.VirtualSize < 0)
	{
		printf("Could not find text or rdata section\n");
		return nullptr;
	}

	auto textStart = modBase + textSection.VirtualAddress;

	auto rdataStart = modBase + rdataSection.VirtualAddress;
	auto rdataEnd = rdataStart + rdataSection.Misc.VirtualSize;

	const auto scanBytes = reinterpret_cast<std::uint8_t*>(textStart);

	for (DWORD i = 0x0; i < textSection.Misc.VirtualSize; i++)
	{
		if ((scanBytes[i] == ASM::CMOVL || scanBytes[i] == ASM::CMOVS) && scanBytes[i + 1] == ASM::LEA)
		{
			auto stringAdd = reinterpret_cast<uintptr_t>(&scanBytes[i]);
			stringAdd = stringAdd + 7 + *reinterpret_cast<int32_t*>(stringAdd + 3);

			// Check if the string is in the .rdata section
			if (stringAdd && stringAdd > rdataStart && stringAdd < rdataEnd)
			{
				auto strBytes = reinterpret_cast<uint8_t*>(stringAdd);

				// Check if the first char is printable
				if (isAscii[strBytes[0]])
				{
					typedef T::value_type char_type;

					T lea((char_type*)stringAdd);

					/*
					if (bIsWide && strBytes[1] != 0)
						break;
					*/

					// bIsWide ? printf("%ls\n", lea.c_str()) : printf("%s\n", lea.c_str());

					if (lea == string)
					{
						return &scanBytes[i];
					}
				}
			}
		}
	}

	return nullptr;
}

template <typename T>
void* Finder::FindByString(T& string, std::vector<int> opcodesToFind, bool bRelative, uint32_t offset, bool forward, int skip, DWORD delimiter)
{
	auto ref = FindStringRef(string);

	if (ref)
	{

		int skipped = 0;
		const auto scanBytes = static_cast<std::uint8_t*>(ref);

		for (auto i = 0; forward ? (i < 2048) : (i > -2048); forward ? i++ : i--)
		{
			if (opcodesToFind.size() == 0)
			{
				if (scanBytes[i] == ASM::INT3 ||
					scanBytes[i] == ASM::RETN ||
					scanBytes[i] == delimiter)
				{
					if (scanBytes[i - 1] != ASM::ADD)
						return &scanBytes[i + 1];
				}
			}
			else
			{
				for (uint8_t byte : opcodesToFind)
				{
					if (scanBytes[i] == byte && byte != ASM::SKIP)
					{
						if (skipped != skip)
						{
							skipped++;
							continue;
						}

						if (bRelative)
						{
							uintptr_t address = reinterpret_cast<uintptr_t>(&scanBytes[i]);
							address = ((address + offset + 4) + *(int32_t*)(address + offset));
							return (void*)address;
						}
						return &scanBytes[i];
					}
				}
			}
		}
	}

	return nullptr;
}

template void* Finder::FindByString(const std::string& string, std::vector<int> opcodesToFind, bool bRelative, uint32_t offset, bool forward, int skip, DWORD delimiter);
template void* Finder::FindByString(const std::wstring& string, std::vector<int> opcodesToFind, bool bRelative, uint32_t offset, bool forward, int skip, DWORD delimiter);