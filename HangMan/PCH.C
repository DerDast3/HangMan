#include "pch.h"

int dummydummy(int a)
{
    static int b;
    b=a;
    return b;
}

