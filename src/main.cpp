#include <print>

#include <core/application.hpp>
#include <helpers/log.hpp>

int main(int argc, char const *argv[])
{
    Application app;
    if (!app.Run())
    {
        ERROR("Application stopped unexpectedly!");
    }
}
