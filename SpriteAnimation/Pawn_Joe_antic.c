// Partiré del s_game i el simplificaré

// ____________________________
// ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄
// ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
// █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
//  Pawn Joe animation module
//─────────────────────────────────────────────────────────────────────────────

//=============================================================================
// INCLUDES
//=============================================================================
#include "Bricks_sprites_msxgl.h"
#include "game/pawn.h"
#include "sprite_fx.h"
#include "msxgl.h"
#include "input.h"
#include "print.h"
#include "vdp.h"

//=============================================================================
// DEFINES
//=============================================================================

// Actions id for Joe
enum JOE_ACTION_ID
{
	JOE_ACTION_IDLE = 0,
	JOE_ACTION_WALK,
	JOE_ACTION_JUMP,
	JOE_ACTION_FALL,
};

//=============================================================================
// READ-ONLY DATA
//=============================================================================

// Joe sprite layers - using patterns from Bricks_sprites.h
const Pawn_Sprite g_JoeSpriteLayers[] =
{//   OffsetX OffsetY DataOffset Color            Flag
	{ 0,      0,      0,         COLOR_LIGHT_YELLOW, 0 }, // Main body
	{ 0,      0,      4,         COLOR_LIGHT_RED,    0 }, // Details layer 1
	{ 0,      0,      8,         COLOR_LIGHT_BLUE,   0 }, // Details layer 2
};

// Idle animation frames - Joe standing still with subtle movement
const Pawn_Frame g_JoeFramesIdle[] =
{ //  Id     Duration Function
	{ 0,	  60,     NULL }, // Standing pose 1
	{ 1,	  60,     NULL }, // Standing pose 2 (slight variation)
	{ 0,	  60,     NULL }, // Back to pose 1
	{ 2,	  30,     NULL }, // Blink or small movement
};

// Walk animation frames - Joe walking cycle
const Pawn_Frame g_JoeFramesWalk[] =
{
	{ 3,	8,	NULL }, // Walk frame 1
	{ 4,	8,	NULL }, // Walk frame 2
	{ 5,	8,	NULL }, // Walk frame 3
	{ 6,	8,	NULL }, // Walk frame 4
	{ 7,	8,	NULL }, // Walk frame 5
	{ 8,	8,	NULL }, // Walk frame 6
};

// Jump animation frames - Joe jumping up
const Pawn_Frame g_JoeFramesJump[] =
{
	{ 9,	 12,	NULL }, // Jump preparation
	{ 10,    8,	    NULL }, // Jump peak
	{ 11,    8,	    NULL }, // Jump extension
};

// Fall animation frames - Joe falling down
const Pawn_Frame g_JoeFramesFall[] =
{
	{ 12, 8,	NULL }, // Fall pose 1
	{ 13, 8,	NULL }, // Fall pose 2
};

// List of all Joe actions
const Pawn_Action g_JoeAnimActions[] =
{ //  Frames           Number                     Loop? Interrupt?
	{ g_JoeFramesIdle, numberof(g_JoeFramesIdle), TRUE, TRUE },
	{ g_JoeFramesWalk, numberof(g_JoeFramesWalk), TRUE, TRUE },
	{ g_JoeFramesJump, numberof(g_JoeFramesJump), TRUE, TRUE },
	{ g_JoeFramesFall, numberof(g_JoeFramesFall), TRUE, TRUE },
};

//=============================================================================
// MEMORY DATA
//=============================================================================

Pawn g_JoePawn;					// Joe's pawn data structure
u8   g_PrevRow8 = 0xFF;			// Previous keyboard 8th row value
bool g_bMoving = FALSE;			// Is Joe moving?
i8   g_DX = 0;					// Current X movement

// Font
#include "font/font_mgl_sample8.h"

//=============================================================================
// FUNCTIONS
//=============================================================================

//-----------------------------------------------------------------------------
// Initialize Joe pawn
void Joe_Initialize(u8 x, u8 y)
{
}



//-----------------------------------------------------------------------------
// Initialize the demo
void Initialize()
{
	// Initialize display
	VDP_EnableDisplay(FALSE);
	VDP_SetColor(COLOR_BLACK);
	
	// Initialize text font
	Print_SetTextFont(g_Font_MGL_Sample8, 32);
	Print_SetColor(0xF, 0x1);
	
	// Initialize sprite
	VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16 | VDP_SPRITE_SCALE_1);
	VDP_LoadSpritePattern(Sprites, 0, 18*32);	// Load all 18 sprite patterns, 32 bytes each
	VDP_DisableSpritesFrom(3);

        // Initialize Joe
        // Initialize Joe pawn with sprite layers and animations
  Pawn_Initialize(&g_JoePawn, g_JoeSpriteLayers,
                  numberof(g_JoeSpriteLayers), 0, g_JoeAnimActions);

  // Set initial position
  Pawn_SetPosition(&g_JoePawn, 100, 60);

  // Set initial action to idle
  Pawn_SetAction(&g_JoePawn, JOE_ACTION_IDLE);

  // Update the pawn animation
  Pawn_Update(&g_JoePawn);
  // Draw Joe
  Pawn_Draw(&g_JoePawn);
  
  // Initialize text display
	Print_SetPosition(0, 0);
  Print_DrawText("JOE DEMO - Use arrows to move, ESC to exit");

  VDP_SetMode(VDP_MODE_GRAPHIC4);

	VDP_EnableDisplay(TRUE);
}

//-----------------------------------------------------------------------------
// Update the demo
void Update()
{
	// Update Joe's animation based on movement
	u8 act = JOE_ACTION_IDLE;
	if (g_bMoving)
		act = JOE_ACTION_WALK;
	
	Pawn_SetAction(&g_JoePawn, act);
	Pawn_SetMovement(&g_JoePawn, g_DX, 0);
	
	Joe_Update();
	Joe_Draw();

	// Handle input
	g_DX = 0;
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
	else
		g_bMoving = FALSE;

	g_PrevRow8 = row8;
}

//=============================================================================
// MAIN LOOP
//=============================================================================

//-----------------------------------------------------------------------------
// Programme entry point
void main()
{
	Bios_SetKeyClick(FALSE);
	
	Initialize();
	
	// Main animation loop
	while(!Keyboard_IsKeyPressed(KEY_ESC))
	{
		Update();
		
		// Wait for VBlank
		VDP_WaitVBlank();
	}
}

