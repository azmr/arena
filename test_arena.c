#define Assert(x) Test(x)
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <sweet/sweet.h>
typedef size_t memory_index;
#include "arena.h"

int main()
{
	Test(0);
	return PrintTestResults();
}

SWEET_END_TESTS;
