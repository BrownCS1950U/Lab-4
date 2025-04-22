#include "window.h"

int main(int argc, char *argv[])
{

    // Check command line arguments
    if (argc < 2)
    {
        std::cout << "Usage: viewer [filename.obj]" << std::endl;
        return 0;
    }

    gl::Window::initialize(argv[1]);

    while (gl::Window::isActive())
    {
        gl::Window::update();
    }

    glfwTerminate();
    return 0;
}