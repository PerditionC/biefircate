#include <ctype.h>

int __cdecl isdigit(int ch)
{
	switch ((unsigned char)ch) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		return 1;
	    default:
		return 0;
	}
}

int __cdecl isprint(int ch)
{
	unsigned char c = (unsigned char)ch;
	return c >= 0x20 && c <= 0x7e;
}

int __cdecl isspace(int ch)
{
	switch ((unsigned char)ch) {
	    case '\f':
	    case '\n':
	    case '\r':
	    case '\t':
	    case '\v':
	    case ' ':
		return 1;
	    default:
		return 0;
	}
}

int __cdecl isxdigit(int ch)
{
	switch ((unsigned char)ch) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return 1;
	    default:
		return 0;
	}
}

int __cdecl tolower(int ch)
{
	unsigned char c = (unsigned char)ch;
	switch (c) {
	    case 'A' ... 'Z':
		return c - 'A' + 'a';
	    default:
		return c;
	}
}

int __cdecl toupper(int ch)
{
	unsigned char c = (unsigned char)ch;
	switch (c) {
	    case 'a' ... 'z':
		return c - 'a' + 'A';
	    default:
		return c;
	}
}
