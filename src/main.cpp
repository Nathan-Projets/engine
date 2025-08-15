#include <print>

#include <core/application.hpp>

int main(int argc, char const *argv[])
{
    Application app;
    if (!app.Run())
    {
        std::println("Application stopped unexpectedly!");
    }
}
