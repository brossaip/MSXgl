// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  Game module sample
//─────────────────────────────────────────────────────────────────────────────

//=============================================================================
// INCLUDES
//=============================================================================
#include "msxgl.h"
#include "game/state.h"
#include "game/pawn.h"
#include "math.h"
#include "debug.h"

//=============================================================================
// DEFINES
//=============================================================================

// Library's logo
#define MSX_GL "\x02\x03\x04\x05"

// Function prototypes
bool State_Initialize();
bool State_Game();
bool State_Pause();

//=============================================================================
// READ-ONLY DATA
//=============================================================================

// Font
#include "font/font_mgl_sample8.h"
// Sprites data by GrafxKid (https://opengameart.org/content/super-random-sprites)
#include "../../samples/content/data_sprt_layer.h"

// Sign-of-life character animation data
const c8 g_ChrAnim[] = { '|', '\\', '-', '/' };

// Pawn sprite layers
const Pawn_Sprite g_SpriteLayers[] =
{//   X  Y  Pattern Color            Option
	{ 0, 0, 0,      COLOR_BLACK,     PAWN_SPRITE_BLEND }, // Black sprite alternated each frame
	{ 0, 0, 4,      COLOR_WHITE,     0 },
	{ 0, 0, 8,      COLOR_LIGHT_RED, 0 },
};

// Idle animation frames
const Pawn_Frame g_FramesIdle[] =
{ //  Pattern Time Function
	{ 6*16,	  48,  NULL },
	{ 7*16,	  24,  NULL },
};

// Move animation frames
const Pawn_Frame g_FramesMove[] =
{
	{ 0*16,	4,	NULL },
	{ 1*16,	4,	NULL },
	{ 2*16,	4,	NULL },
	{ 3*16,	4,	NULL },
	{ 4*16,	4,	NULL },
	{ 5*16,	4,	NULL },
};

// Jump animation frames
const Pawn_Frame g_FramesJump[] =
{
	{ 3*16,	4,	NULL },
	{ 8*16,	4,	NULL },
};

// Fall animation frames
const Pawn_Frame g_FramesFall[] =
{
	{ 9*16,	4,	NULL },
};

// Actions id
enum ANIM_ACTION_ID
{
	ACTION_IDLE = 0,
	ACTION_MOVE,
	ACTION_JUMP,
	ACTION_FALL,
};

// List of all player actions
const Pawn_Action g_AnimActions[] =
{ //  Frames        Number                  Loop? Interrupt?
	{ g_FramesIdle, numberof(g_FramesIdle), TRUE, TRUE },
	{ g_FramesMove, numberof(g_FramesMove), TRUE, TRUE },
	{ g_FramesJump, numberof(g_FramesJump), TRUE, TRUE },
	{ g_FramesFall, numberof(g_FramesFall), TRUE, TRUE },
};

//=============================================================================
// MEMORY DATA
//=============================================================================

bool g_GameExit = FALSE;			// Game exit flag
Pawn g_PlayerPawn;					// Player's pawn data structure
bool g_bFlicker = TRUE;				// Activate sprite colorflickering 
bool g_bMoving = FALSE;				// Is player moving?
u8   g_PrevRow8 = 0xFF;				// Previous keyboard 8th row value
i8   g_DX = 0;						// Current X movement
i8   g_DY = 0;						// Current Y movement
bool g_bEnable = TRUE;				// 

//=============================================================================
// FUNCTIONS
//=============================================================================

//-----------------------------------------------------------------------------
// Wait for VBlank (simplified version)
void VDP_WaitVBlank()
{
	// Simple VBlank wait - halt until interrupt
	__asm
		halt
	__endasm;
}

//-----------------------------------------------------------------------------
// Update the current game state
void Game_UpdateState()
{
	// Call the current state function (simplified version)
	State_Game();
}

//-----------------------------------------------------------------------------
// Initialize the game
bool State_Initialize()
{
	// Initialize display
	VDP_EnableDisplay(FALSE);
	VDP_SetColor(COLOR_BLACK);
	
	// Initialize text font
	Print_SetTextFont(g_Font_MGL_Sample8, 32);
	Print_SetColor(0xF, 0x1);

	// Initialize sprite
	VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);
	VDP_LoadSpritePattern(g_DataSprtLayer, 0, 13*4*4);	
	VDP_DisableSpritesFrom(3);

	// Init player pawn
	Pawn_Initialize(&g_PlayerPawn, g_SpriteLayers, numberof(g_SpriteLayers), 0, g_AnimActions);
	Pawn_SetPosition(&g_PlayerPawn, 100, 60);
	Pawn_SetColorBlend(&g_PlayerPawn, g_bFlicker);

	// Initialize text
	Print_SetPosition(0, 0);
	Print_DrawText(MSX_GL " SPRITE ANIMATION DEMO - Use arrows to move, ESC to exit");

	VDP_EnableDisplay(TRUE);

	Game_SetState(State_Game);
	return FALSE; // Frame finished
}

//-----------------------------------------------------------------------------
// Update the gameplay
bool State_Game()
{
	// Update player animation
	u8 act = ACTION_IDLE;
	if (g_bMoving)
		act = ACTION_MOVE;
	
	Pawn_SetAction(&g_PlayerPawn, act);
	Pawn_SetMovement(&g_PlayerPawn, g_DX, g_DY);
	Pawn_Update(&g_PlayerPawn);
	Pawn_Draw(&g_PlayerPawn);

	// Character animation
	Print_SetPosition(31, 0);
	Print_DrawChar(g_ChrAnim[g_PlayerPawn.Counter & 0x03]);

	// Handle input
	g_DX = 0;
	g_DY = 0;
	u8 row8 = Keyboard_Read(8);
	if (IS_KEY_PRESSED(row8, KEY_RIGHT))
	{
		g_DX++;
		g_bMoving = TRUE;
	}
	else if (IS_KEY_PRESSED(row8, KEY_LEFT))
	{
		g_DX--;
		g_bMoving = TRUE;
	}
	else if (IS_KEY_PRESSED(row8, KEY_UP))
	{
		g_DY--;
		g_bMoving = TRUE;
	}
	else if (IS_KEY_PRESSED(row8, KEY_DOWN))
	{
		g_DY++;
		g_bMoving = TRUE;
	}
	else
		g_bMoving = FALSE;

	if (IS_KEY_PUSHED(row8, g_PrevRow8, KEY_HOME))
	{
		g_bFlicker = 1 - g_bFlicker;
		Pawn_SetColorBlend(&g_PlayerPawn, g_bFlicker);
	}
	
	if (IS_KEY_PUSHED(row8, g_PrevRow8, KEY_DEL))
	{
		g_bEnable = !g_bEnable;
		Pawn_SetEnable(&g_PlayerPawn, g_bEnable);
	}

	g_PrevRow8 = row8;

	if (Keyboard_IsKeyPressed(KEY_ESC))
		Game_Exit();

	return TRUE; // Frame finished
}

//-----------------------------------------------------------------------------
// Pause the game
bool State_Pause()
{
	return TRUE; // Frame finished
}

//=============================================================================
// MAIN LOOP
//=============================================================================

//-----------------------------------------------------------------------------
// Programme entry point
void main()
{
	Bios_SetKeyClick(FALSE);
	
	State_Initialize();
	
	// Main game loop
	while(!g_GameExit)
	{
		Game_UpdateState();
		
		// Wait for VBlank
		VDP_WaitVBlank();
	}
}
