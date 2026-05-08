/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp"
#include <ixion/version.hpp>

#include <format>

void test_version()
{
    IXION_TEST_FUNC_SCOPE;

    {
        std::string ver = IXION_VERSION; // it's a string literal
        std::cout << "version: " << ver << std::endl;

        assert(ver == std::format("{}.{}.{}", IXION_VERSION_MAJOR, IXION_VERSION_MINOR, IXION_VERSION_MICRO));
    }

    {
        // make sure these are integer literals
        int ver = IXION_VERSION_MAJOR;
        std::cout << "major: " << ver << std::endl;
        ver = IXION_VERSION_MINOR;
        std::cout << "minor: " << ver << std::endl;
        ver = IXION_VERSION_MICRO;
        std::cout << "micro: " << ver << std::endl;
    }

    {
        std::string ver = IXION_API_VERSION; // it's a string literal
        std::cout << "API version: " << ver << std::endl;

        assert(ver == std::format("{}.{}", IXION_API_VERSION_MAJOR, IXION_API_VERSION_MINOR));
    }

    {
        // make sure these are integer literals
        int ver = IXION_API_VERSION_MAJOR;
        std::cout << "API major: " << ver << std::endl;
        ver = IXION_API_VERSION_MINOR;
        std::cout << "API minor: " << ver << std::endl;
    }
}

int main()
{
    test_version();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
