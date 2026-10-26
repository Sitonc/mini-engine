#include "Window.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <utility>

Window::Window(const std::string& title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error(SDL_GetError());

    m_window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
    if (!m_window)
        throw std::runtime_error(SDL_GetError());

    LOG_INFO("Window created ({}x{})", width, height);
}

Window::~Window()
{
    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
}

Window::Window(Window&& other) noexcept
    : m_window(std::exchange(other.m_window, nullptr)) {
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        if (m_window) { SDL_DestroyWindow(m_window); SDL_Quit(); }
        m_window = std::exchange(other.m_window, nullptr);
    }
    return *this;
}

bool Window::PumpEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT) return false;
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) return false;
    }
    return true;
}