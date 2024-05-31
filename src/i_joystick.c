#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "doomtype.h"
#include "d_event.h"
#include "i_joystick.h"
#include "i_system.h"

#include "m_config.h"
#include "m_misc.h"

// When an axis is within the dead zone, it is set to zero.
// This is 5% of the full range:
#define DEAD_ZONE (32768 / 20)

static SDL_GameController *controller = NULL;

// Configuration variables:
static int usejoystick = 0;

// Virtual to physical button joystick button mapping. By default this
// is a straight mapping.
static int joystick_physical_buttons[NUM_VIRTUAL_BUTTONS] = {
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_GUIDE, SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK, SDL_CONTROLLER_BUTTON_LEFTSHOULDER
};

void I_ShutdownJoystick(void)
{
    if (controller != NULL)
    {
        SDL_GameControllerClose(controller);
        controller = NULL;
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

void I_InitJoystick(void)
{
    if (!usejoystick)
    {
        return;
    }

    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
    {
        printf("I_InitJoystick: SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    if (SDL_NumJoysticks() < 1)
    {
        printf("I_InitJoystick: No joysticks found!\n");
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        return;
    }

    // Open the first available game controller
    controller = SDL_GameControllerOpen(0);

    if (controller == NULL)
    {
        printf("I_InitJoystick: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        return;
    }

    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
    printf("I_InitJoystick: Found joystick: %s\n", SDL_JoystickName(joystick));

    SDL_GameControllerEventState(SDL_ENABLE);

    // Initialized okay!
    printf("I_InitJoystick: %s\n", SDL_JoystickName(joystick));

    I_AtExit(I_ShutdownJoystick, true);
}

// Get the state of the given virtual button.
static int ReadButtonState(int vbutton)
{
    // Map from virtual button to physical (SDL) button.
    if (vbutton < NUM_VIRTUAL_BUTTONS)
    {
        return SDL_GameControllerGetButton(controller, joystick_physical_buttons[vbutton]);
    }
    else
    {
        return 0;
    }
}

// Get a bitmask of all currently-pressed buttons
static int GetButtonsState(void)
{
    int i;
    int result = 0;

    for (i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
        if (ReadButtonState(i))
        {
            result |= 1 << i;
        }
    }

    return result;
}

// Read the state of an axis, inverting if necessary.
static int GetAxisState(SDL_GameControllerAxis axis, int invert)
{
    int result;

    // Axis -1 means disabled.
    if (axis < 0)
    {
        return 0;
    }

    result = SDL_GameControllerGetAxis(controller, axis);

    if (result < DEAD_ZONE && result > -DEAD_ZONE)
    {
        result = 0;
    }

    if (invert)
    {
        result = -result;
    }

    return result;
}

void I_UpdateJoystick(void)
{
    if (controller != NULL)
    {
        event_t ev;

        ev.type = ev_joystick;
        ev.data1 = GetButtonsState();
        ev.data2 = GetAxisState(SDL_CONTROLLER_AXIS_LEFTX, 0); // Horizontal movement
        ev.data3 = GetAxisState(SDL_CONTROLLER_AXIS_LEFTY, 0); // Vertical movement
        ev.data4 = GetAxisState(SDL_CONTROLLER_AXIS_RIGHTX, 0); // Strafing

        D_PostEvent(&ev);
    }
}

void I_BindJoystickVariables(void)
{
    int i;

    M_BindVariable("use_joystick", &usejoystick);

    for (i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
        char name[32];
        M_snprintf(name, sizeof(name), "joystick_physical_button%i", i);
        M_BindVariable(name, &joystick_physical_buttons[i]);
    }
}
