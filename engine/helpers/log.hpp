#pragma once

#include <iostream>
#include <string>

// ANSI color codes
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

// logs function to display to the console
#define INFO(STR) \
    std::cout << GREEN << "[INFO] " << STR << RESET << std::endl;

#define ERROR(STR) \
    std::cout << RED << "[ERROR] " << STR << RESET << std::endl;

#define WARNING(STR) \
    std::cout << YELLOW << "[WARNING] " << STR << RESET << std::endl;

// uncomment the following line to allow debug display
#define IS_DEBUG

#ifdef IS_DEBUG
#define DEBUG(STR) \
    std::cout << BLUE << "[DEBUG] " << STR << RESET << std::endl
#else
#define DEBUG(STR)
#endif

// formatting helper to use within a buffer, (it doesn't print on its own)
#define BOLD(STR) \
    "\033[1m" << STR << "\033[22m"