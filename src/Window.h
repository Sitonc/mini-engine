#pragma once
#include <string>

struct SDL_Window;

// RAII wrapper: owns the SDL window, move-only.
class Window
{
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    // Returns false when the user asked to quit.
    bool PumpEvents();

private:
    SDL_Window* m_window = nullptr;
};