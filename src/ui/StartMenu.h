#pragma once

/**
 * @file StartMenu.h
 * @brief Pre-simulation modal: title + action buttons over the rotating skybox.
 */

/**
 * @brief Possible outcomes of a single StartMenu frame.
 */
enum class StartMenuAction
{
    None,     ///< User did not click any action button this frame.
    Start,    ///< Begin a new simulation from the default preset.
    Load,     ///< Open the load-save modal (deferred to Task 12).
    Settings, ///< Open the settings modal (deferred to Task 11).
    Quit      ///< Close the application.
};

/**
 * @brief Renders the start-screen modal. Stateless — purely a render helper.
 */
class StartMenu
{
public:
    /**
     * @brief Draw the modal and return the user's action for this frame.
     *
     * Must be invoked between `ImGui::NewFrame` and `ImGui::Render`.
     *
     * @param viewportWidth  Framebuffer width in pixels (used for centering).
     * @param viewportHeight Framebuffer height in pixels (used for centering).
     * @return               The action the user selected, or `None` if nothing.
     */
    StartMenuAction render(int viewportWidth, int viewportHeight);
};
