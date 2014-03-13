/*
    This file is part of Spike Guard.

    Spike Guard is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spike Guard is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spike Guard.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "utils.h"

namespace utils {

std::string read_ascii_string(FILE* f)
{
	std::string s = std::string();
	char c;
	while (1 == fread(&c, 1, 1, f))
	{
		if (c == '\0') {
			break;
		}
		s += c;
	}
	return s;
}

std::string read_unicode_string(FILE* f)
{
	std::wstring s = std::wstring();
	wchar_t c = 0;
	boost::uint16_t size;
	if (2 !=fread(&size, 1, 2, f)) {
		return "";
	}

	// Microsoft's "unicode" strings are word aligned.
	for (unsigned int i = 0 ; i < size ; ++i)
	{
		if (2 != fread(&c, 1, 2, f)) {
			break;
		}
		s += c;
	}
	s += L'\0';

	// Convert the wstring into a string
	boost::shared_array<char> conv = boost::shared_array<char>(new char[s.size() + 1]);
	memset(conv.get(), 0, sizeof(char) * (s.size() + 1));
	wcstombs(conv.get(), s.c_str(), s.size());
	return std::string(conv.get());
}

bool read_string_at_offset(FILE* f, unsigned int offset, std::string& out, bool unicode)
{
	unsigned int saved_offset = ftell(f);
	if (saved_offset == -1 || fseek(f, offset, SEEK_SET))
	{
		std::cerr << "[!] Error: Could not reach offset 0x" << std::hex << offset << "." << std::endl;
		return false;
	}
	if (!unicode) {
		out = read_ascii_string(f);
	}
	else {
		out = read_unicode_string(f);
	}
	return !fseek(f, saved_offset, SEEK_SET) && out != "";
}

bool is_address_in_section(unsigned int rva, sg::pimage_section_header section, bool check_raw_size)
{
	if (!check_raw_size) {
		return section->VirtualAddress <= rva && rva < section->VirtualAddress + section->VirtualSize;
	}
	else {
		return section->VirtualAddress <= rva && rva < section->VirtualAddress + section->SizeOfRawData;
	}
}

sg::pimage_section_header find_section(unsigned int rva, const std::vector<sg::pimage_section_header>& section_list)
{
	sg::pimage_section_header res = sg::pimage_section_header();
	std::vector<sg::pimage_section_header>::const_iterator it;
	for (it = section_list.begin() ; it != section_list.end() ; ++it)
	{
		if (is_address_in_section(rva, *it)) 
		{
			res = *it;
			break;
		}
	}

	if (!res) // VirtualSize may be erroneous. Check with RawSizeofData.
	{
		for (it = section_list.begin() ; it != section_list.end() ; ++it)
		{
			if (is_address_in_section(rva, *it, true)) 
			{
				res = *it;
				break;
			}
		}
	}

	return res;
}

}