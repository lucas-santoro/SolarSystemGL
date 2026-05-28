#include "core/Application.h"

#include <cstdlib>
#include <exception>
#include <iostream>

/**
 * @file main.cpp
 * @brief Program entry point. Delegates to Application::run() with top-level
 *        exception handling.
 */
int main()
{
    try
    {
        Application app(800, 600, "SolarSystemGL");
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
