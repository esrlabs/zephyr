// Copyright 2025 Accenture.

#include "util/string/ConstString.h"
#include <iostream>

using namespace ::util::string;

int main(void)
{
    ConstString hw = ConstString("Hello World!");
    std::cout << hw.data() << " from " << CONFIG_BOARD_TARGET << std::endl;

    return 0;
}
