/**
 * @file main.cpp
 * @author Naikekongwo
 * @brief OpenWorlder 的程序入口
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenWorlder/OpenWorlder.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char **args)
{
    OpenWorlder worlder;

    try
    {
        if (!worlder.StartUp())
            return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
