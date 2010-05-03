/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file menu.h
 *
 * @brief Handles the important game menus.
 */


#include "menu.h"

#include "naev.h"

#include <string.h>

#include "SDL.h"

#include "toolkit.h"
#include "dialogue.h"
#include "log.h"
#include "pilot.h"
#include "space.h"
#include "player.h"
#include "mission.h"
#include "ntime.h"
#include "save.h"
#include "land.h"
#include "rng.h"
#include "nebula.h"
#include "pause.h"
#include "options.h"
#include "intro.h"
#include "music.h"
#include "map.h"
#include "nfile.h"
#include "info.h"
#include "comm.h"
#include "conf.h"
#include "dev_uniedit.h"


#define MAIN_WIDTH      130 /**< Main menu width. */
#define MAIN_HEIGHT     300 /**< Main menu height. */

#define MENU_WIDTH      130 /**< Escape menu width. */
#define MENU_HEIGHT     200 /**< Escape menu height. */


#define DEATH_WIDTH     130 /**< Death menu width. */
#define DEATH_HEIGHT    200 /**< Death menu height. */

#define OPTIONS_WIDTH   360 /**< Options menu width. */
#define OPTIONS_HEIGHT  90  /**< Options menu height. */

#define BUTTON_WIDTH    90 /**< Button width, standard across menus. */
#define BUTTON_HEIGHT   30 /**< Button height, standard across menus. */

#define menu_Open(f)    (menu_open |= (f)) /**< Marks a menu as opened. */
#define menu_Close(f)   (menu_open &= ~(f)) /**< Marks a menu as closed. */
int menu_open = 0; /**< Stores the opened/closed menus. */


static glTexture *main_naevLogo = NULL; /**< NAEV Logo texture. */
static unsigned int menu_main_lasttick = 0;


/*
 * prototypes
 */
/* Generic. */
static void menu_exit( unsigned int wid, char* str );
/* main menu */
static void main_menu_promptClose( unsigned int wid, char *unused );
void menu_main_close (void); /**< Externed in save.c */
static void menu_main_nebu( double x, double y, double w, double h, void *data );
static void menu_main_load( unsigned int wid, char* str );
static void menu_main_new( unsigned int wid, char* str );
static void menu_main_credits( unsigned int wid, char* str );
static void menu_main_cleanBG( unsigned int wid, char* str );
/* small menu */
static void menu_small_close( unsigned int wid, char* str );
static void menu_small_exit( unsigned int wid, char* str );
static void exit_game (void);
/* death menu */
static void menu_death_continue( unsigned int wid, char* str );
static void menu_death_restart( unsigned int wid, char* str );
static void menu_death_main( unsigned int wid, char* str );
/* options button. */
static void menu_options_button( unsigned int wid, char *str );


/**
 * @brief Opens the main menu (titlescreen).
 */
void menu_main (void)
{
   int offset_logo, offset_wdw, freespace;
   unsigned int bwid, wid;
   glTexture *tex;
   int h, y;

   /* Play load music. */
   music_choose("load");

   /* Load background and friends. */
   tex = gl_newImage( "gfx/NAEV.png", 0 );
   main_naevLogo = tex;
   nebu_prep( 300., 0. ); /* Needed for nebula to not spaz out */

   /* Calculate Logo and window offset. */
   freespace = SCREEN_H - tex->sh - MAIN_HEIGHT;
   if (freespace < 0) { /* Not enough freespace, this can get ugly. */
      offset_logo = SCREEN_W - tex->sh;
      offset_wdw  = 0;
   }
   else {
      /* We'll want a maximum seperation of 30 between logo and text. */
      if (freespace/3 > 25) {
         freespace -= 25;
         offset_logo = -25;
         /*offset_wdw  = -25 - tex->sh - 25;*/
         offset_wdw  = -1;
      }
      /* Otherwise space evenly. */
      else {
         offset_logo = -freespace/3;
         offset_wdw  = freespace/3;
      }
   }

   /* create background image window */
   bwid = window_create( "BG", -1, -1, SCREEN_W, SCREEN_H );
   window_onClose( bwid, menu_main_cleanBG );
   window_addRect( bwid, 0, 0, SCREEN_W, SCREEN_H, "rctBG", &cBlack, 0 );
   window_addCust( bwid, 0, 0, SCREEN_W, SCREEN_H, "cstBG", 0,
         menu_main_nebu, NULL, &menu_main_lasttick );
   window_addImage( bwid, (SCREEN_W-tex->sw)/2., offset_logo, 0, 0, "imgLogo", tex, 0 );
   window_addText( bwid, 0., 10, SCREEN_W, 30., 1, "txtBG", NULL,
         &cWhite, naev_version(1) );

   /* Set dimensions */
   h  = MAIN_HEIGHT;
   y  = 20 + (BUTTON_HEIGHT+20)*4;
   if (conf.devmode) {
      h += BUTTON_HEIGHT + 20;
      y += BUTTON_HEIGHT+20;
   }

   /* create menu window */
   wid = window_create( "Main Menu", -1, offset_wdw, MAIN_WIDTH, h );
   window_setCancel( wid, main_menu_promptClose );

   /* Buttons. */
   window_addButton( wid, 20, y, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnLoad", "Load Game", menu_main_load );
   y -= BUTTON_HEIGHT+20;
   window_addButton( wid, 20, y, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnNew", "New Game", menu_main_new );
   y -= BUTTON_HEIGHT+20;
   if (conf.devmode) {
      window_addButton( wid, 20, y, BUTTON_WIDTH, BUTTON_HEIGHT,
            "btnEditor", "Editor", uniedit_open );
      y -= BUTTON_HEIGHT+20;
   }
   window_addButton( wid, 20, y, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnOptions", "Options", menu_options_button );
   y -= BUTTON_HEIGHT+20;
   window_addButton( wid, 20, y, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnCredits", "Credits", menu_main_credits );
   y -= BUTTON_HEIGHT+20;
   window_addButton( wid, 20, y, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnExit", "Exit", menu_exit );

   /* Disable load button if there are no saves. */
   if (!save_hasSave())
      window_disableButton( wid, "btnLoad" );

   /* Make the background window a parent of the menu. */
   window_setParent( bwid, wid );

   /* Reset timer. */
   menu_main_lasttick = SDL_GetTicks();

   menu_Open(MENU_MAIN);
}


/**
 * @brief Main menu closing prompt.
 */
static void main_menu_promptClose( unsigned int wid, char *unused )
{
   if (dialogue_YesNo( "Quit", "Are you sure you want to quit?" ))
      menu_exit( wid, unused );
}


/**
 * @brief Renders the nebula.
 */
static void menu_main_nebu( double x, double y, double w, double h, void *data )
{
   (void) x;
   (void) y;
   (void) w;
   (void) h;
   unsigned int *t, tick;
   double dt;

   /* Get time dt. */
   t     = (unsigned int *)data;
   tick  = SDL_GetTicks();
   dt    = (double)(tick - *t) / 1000.;
   *t    = tick;

   /* Render. */
   nebu_render( dt );
}
/**
 * @brief Closes the main menu.
 */
void menu_main_close (void)
{
   if (window_exists("Main Menu"))
      window_destroy( window_get("Main Menu") );

   menu_Close(MENU_MAIN);
}
/**
 * @brief Function to active the load game menu.
 *    @param str Unused.
 */
static void menu_main_load( unsigned int wid, char* str )
{
   (void) str;
   (void) wid;
   save_loadGameMenu();
}
/**
 * @brief Function to active the new game menu.
 *    @param str Unused.
 */
static void menu_main_new( unsigned int wid, char* str )
{
   (void) str;
   (void) wid;
   menu_main_close();
   player_new();
}
/**
 * @brief Function to exit the main menu and game.
 *    @param str Unused.
 */
static void menu_main_credits( unsigned int wid, char* str )
{
   (void) str;
   (void) wid;
   intro_display( "AUTHORS", "credits" );
   /* We'll need to start music again. */
   music_choose("load");
}
/**
 * @brief Function to exit the main menu and game.
 *    @param str Unused.
 */
static void menu_exit( unsigned int wid, char* str )
{
   (void) str;
   (void) wid;

   exit_game();
}
/**
 * @brief Function to clean up the background window.
 *    @param wid Window to clean.
 *    @param str Unused.
 */
static void menu_main_cleanBG( unsigned int wid, char* str )
{
   (void) str;

   /* 
    * Ugly hack to prevent player.c from segfaulting due to the fact
    * that game will attempt to render while waiting for the quit event
    * pushed by exit_game() to be handled without actually having a player
    * nor anything of the likes (nor toolkit to stop rendering) while
    * not leaking any texture.
    */
   if (main_naevLogo != NULL)
      gl_freeTexture(main_naevLogo);
   main_naevLogo = NULL;
   window_modifyImage( wid, "imgLogo", NULL, 0, 0 );
}


/*
 *
 * ingame menu
 *
 */
/**
 * @brief Opens the small ingame menu.
 */
void menu_small (void)
{
   unsigned int wid;

   /* Check if menu should be openable. */
   if ((player.p == NULL) || player_isFlag(PLAYER_DESTROYED) ||
         pilot_isFlag(player.p,PILOT_DEAD) ||
         comm_isOpen() ||
         dialogue_isOpen() || /* Shouldn't open over dialogues. */
         (menu_isOpen(MENU_MAIN) ||
            menu_isOpen(MENU_SMALL) ||
            menu_isOpen(MENU_DEATH) ))
      return;

   wid = window_create( "Menu", -1, -1, MENU_WIDTH, MENU_HEIGHT );

   window_setCancel( wid, menu_small_close );

   window_addButton( wid, 20, 20 + BUTTON_HEIGHT*2 + 20*2,
         BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnResume", "Resume", menu_small_close );
   window_addButton( wid, 20, 20 + BUTTON_HEIGHT + 20,
         BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnOptions", "Options", menu_options_button );
   window_addButton( wid, 20, 20, BUTTON_WIDTH, BUTTON_HEIGHT, 
         "btnExit", "Exit", menu_small_exit );

   menu_Open(MENU_SMALL);
}
/**
 * @brief Closes the small ingame menu.
 *    @param str Unused.
 */
static void menu_small_close( unsigned int wid, char* str )
{
   (void)str;
   window_destroy( wid );
   menu_Close(MENU_SMALL);
}
/**
 * @brief Closes the small ingame menu and goes back to the main menu.
 *    @param str Unused.
 */
static void menu_small_exit( unsigned int wid, char* str )
{
   (void) str;
   unsigned int info_wid;
   
   /* if landed we must save anyways */
   if (landed) {
      save_all();
      land_cleanup();
   }

   /* Close info menu if open. */
   if (menu_isOpen(MENU_INFO)) {
      info_wid = window_get("Info");
      window_destroy( info_wid );
      menu_Close(MENU_INFO);
   }

   /* Stop player sounds because sometimes they hang. */
   player_abortAutonav( "Exited game." );
   player_stopSound();

   /* Clean up. */
   window_destroy( wid );
   menu_Close(MENU_SMALL);
   menu_main();
}


/**
 * @brief Exits the game.
 */
static void exit_game (void)
{
   /* if landed we must save anyways */
   if (landed) {
      save_all();
      land_cleanup();
   }
   SDL_Event quit;
   quit.type = SDL_QUIT;
   SDL_PushEvent(&quit);
}


/**
 * @brief Reload the current savegame, when player want to continue after death
 */
static void menu_death_continue( unsigned int wid, char* str )
{
   (void) str;

   window_destroy( wid );
   menu_Close(MENU_DEATH);

   save_reload();
}

/**
 * @brief Restart the game, when player want to continue after death but without a savegame
 */
static void menu_death_restart( unsigned int wid, char* str )
{
   (void) str;

   window_destroy( wid );
   menu_Close(MENU_DEATH);

   player_new();
}

/**
 * @brief Player death menu, appears when player got creamed.
 */
void menu_death (void)
{
   unsigned int wid;
   
   wid = window_create( "Death", -1, -1, DEATH_WIDTH, DEATH_HEIGHT );

   /* Propose the player to continue if the samegame exist, if not, propose to restart */
   char path[PATH_MAX];
   snprintf(path, PATH_MAX, "%ssaves/%s.ns", nfile_basePath(), player.name);
   if (nfile_fileExists(path))
      window_addButton( wid, 20, 20 + BUTTON_HEIGHT*2 + 20*2, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnContinue", "Continue", menu_death_continue );
   else
      window_addButton( wid, 20, 20 + BUTTON_HEIGHT*2 + 20*2, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnRestart", "Restart", menu_death_restart );

   window_addButton( wid, 20, 20 + (BUTTON_HEIGHT+20),
         BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnMain", "Main Menu", menu_death_main );
   window_addButton( wid, 20, 20, BUTTON_WIDTH, BUTTON_HEIGHT,
         "btnExit", "Exit Game", menu_exit );
   menu_Open(MENU_DEATH);

   /* Makes it all look cooler since everything still goes on. */
   unpause_game();
}
/**
 * @brief Closes the player death menu.
 *    @param str Unused.
 */
static void menu_death_main( unsigned int wid, char* str )
{
   (void) str;

   window_destroy( wid );
   menu_Close(MENU_DEATH);

   /* Game will repause now since toolkit closes and reopens. */
   menu_main();
}


/**
 * @brief Opens the menu options from a button.
 */
static void menu_options_button( unsigned int wid, char *str )
{
   (void) wid;
   (void) str;
   opt_menu();
}

