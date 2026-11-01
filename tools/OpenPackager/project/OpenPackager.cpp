#include "OpenPackager/OpenPackager.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char **args)
{
    OpenPackager scripter;

    try
    {
        scripter.StartUp();
    }
    catch (std::exception e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}