#include <stdio.h>
#include <glad/glad.h>
#include <SDL2/SDL.h>

// Physics and other game-related stuff is running at a different rate than
// screen updates
#define TICKS_PER_SEC 125
#define MS_PER_TICK ( 1000.0 / (double) TICKS_PER_SEC )
#define INITIAL_WIN_W 640
#define INITIAL_WIN_H 480

void game_tick( double dt ) {

  for ( int i = 0; i < dt * 100000; i++ ) ;
}

int main() {

  int quit = 0;
  SDL_Event event;

  if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
    fprintf( stderr, "Could not init SDL: %s\n", SDL_GetError() );
    return 1;
  }

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 ); 
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 ); 
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE ); 

  SDL_Window *window = SDL_CreateWindow(
    "OpenGL test",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    INITIAL_WIN_W,
    INITIAL_WIN_H,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
  );

  if ( window == NULL ) {
    fprintf( stderr, "Could not create window: %s\n", SDL_GetError() );
    return 1;
  }

  SDL_GLContext *glCtx = SDL_GL_CreateContext( window );

  if ( !gladLoadGLLoader( (GLADloadproc) SDL_GL_GetProcAddress ) ) {
    fprintf( stderr, "Could not init GLAD\n" );
    return 1;
  }

  // Vsync
  SDL_GL_SetSwapInterval( 1 );

  glClearColor( 0.0f, 0.5f, 0.0f, 1.0f );

  /*
   * "Behind the scenes OpenGL uses the data specified via glViewport to
   * transform the 2D coordinates it processed to coordinates on your screen.
   * For example, a processed point of location (-0.5,0.5) would (as its final
   * transformation) be mapped to (200,450) in screen coordinates. Note that
   * processed coordinates in OpenGL are between -1 and 1 so we effectively map
   * from the range (-1 to 1) to (0, 800) and (0, 600)."
   */
  glViewport( 0, 200, INITIAL_WIN_W, INITIAL_WIN_H );

  Uint32 lastUpdate = SDL_GetTicks();
  double updateTimeLeft = 0;
  Uint32 gameTicks = 0;
  Uint32 fpsStart = lastUpdate;

  printf( "MS_PER_TICK: %f\n", MS_PER_TICK );

  while ( !quit ) {

    while ( SDL_PollEvent( &event ) != 0 ) {
      if ( event.type == SDL_QUIT ) {
        quit = 1;
      }
      else if ( event.type == SDL_KEYDOWN ) {
        SDL_Keycode key = event.key.keysym.sym;
        if ( key == SDLK_q )
          quit = 1;
      }
      else if ( event.type == SDL_WINDOWEVENT ) {
        switch ( event.window.event ) {
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            glViewport( 0, 200, event.window.data1, event.window.data2 );
            printf( "Window size: %d x %d\n", event.window.data1, event.window.data2 );
            break;
        }
      }
    }

    Uint32 now = SDL_GetTicks();
    updateTimeLeft += now - lastUpdate;

    while ( updateTimeLeft >= MS_PER_TICK ) {
      game_tick( MS_PER_TICK );
      updateTimeLeft -= MS_PER_TICK;
      ++gameTicks;
    }

    // ----------------------------------------------------
    
    glClear( GL_COLOR_BUFFER_BIT );

    // FIX ME: "On Mac OS X make sure you bind 0 to the draw framebuffer before
    // swapping the window, otherwise nothing will happen"
    SDL_GL_SwapWindow( window );

    // ----------------------------------------------------

    if ( now - fpsStart >= 1000 ) {
      printf( "Ticks/sec: %d\n", (int) ( gameTicks / ( ( (double) now - (double) fpsStart ) / 1000.0 ) ) );
      fpsStart = now;
      gameTicks = 0;
    }

    lastUpdate = now;
  }

  SDL_GL_DeleteContext( glCtx );
  SDL_DestroyWindow( window );
  SDL_Quit();

  return 0;
}

