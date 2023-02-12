#include "doomgeneric.h"

uint32_t* DG_ScreenBuffer = 0;

/**
		Playdate display is 400x240, 30fps default framerate, 50 fps max
		Some contradictions here:
			20fps max: https://sdk.play.date/1.12.3/Inside%20Playdate%20with%20C.html#f-display.setRefreshRate
			50fps max: https://sdk.play.date/1.12.3/Inside%20Playdate.html#f-display.setRefreshRate
*/

/**
								PC								Playdate
Fire						CTRL							A
Move						UDLR							UDLR
Run							SHIFT + Move			? hold U? tilt Y?
Strafe					ALT + Move				? hold LR? tilt X?
Activate/Use		SPACE							B
Map							TAB								? Menu option
Menu						ESC								? Menu option
Select					ENTER							? A

PC: https://en.wikibooks.org/wiki/Doom/Controls
GB: https://www.gamespot.com/articles/hands-on-doom-gba/1100-2803649/
PD: https://sdk.play.date/1.12.3/Inside%20Playdate%20with%20C.html#_buttons
PD: https://sdk.play.date/1.12.3/Inside%20Playdate%20with%20C.html#_interacting_with_the_system_menu
				up to 3 menu items in System Menu and these can be dynamically updated.

**/
static unsigned char convertToDoomKey(unsigned int key){
  switch (key)
    {
    case kButtonA:
      key = KEY_ENTER;
      break;
    case PLACEBO_MENU: // Set when returning from system menu if they selected Menu
      key = KEY_ESCAPE;
      break;
    case kButtonLeft:
      key = KEY_LEFTARROW;
      break;
    case kButtonRight:
      key = KEY_RIGHTARROW;
      break;
    case kButtonUp:
      key = KEY_UPARROW;
      break;
    case kButtonDown:
      key = KEY_DOWNARROW;
      break;
    case kButtonA:
      key = KEY_FIRE;
      break;
    case kButtonB:
      key = KEY_USE;
      break;
    case PLACEBO_RUN:
      key = KEY_RSHIFT;
      break;
    default:
      key = tolower(key);
      break;
    }

  return key;
}

void dg_Create()
{
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	DG_Init();
}
