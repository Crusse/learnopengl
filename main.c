#include <stdio.h>
#include <glad/glad.h>
#include <SDL2/SDL.h>

// Physics and other game-related stuff is running at a different rate than
// screen updates
#define TICKS_PER_SEC 125
#define MS_PER_TICK ( 1000.0 / (double) TICKS_PER_SEC )
#define INITIAL_WIN_W 640
#define INITIAL_WIN_H 480
#define MAX_GL_INFO_LOG 512
#define TOOLBAR_H 100

void game_tick( double dt ) {

  for ( int i = 0; i < dt * 100000; i++ ) ;
}

const char* get_file_contents( const char *path ) {

  FILE *file = fopen( path, "r" );
  if ( !file )
    return NULL;

  fseek( file, 0L, SEEK_END );
  long filesize = ftell( file );
  rewind( file );

  char *ret = (char*) malloc( filesize );
  char *i = ret;
  char c;

  while ( ( c = fgetc( file ) ) != EOF ) {
    *i = c;
    ++i;
  }

  fclose( file );

  i = '\0';

  return ret;
}

unsigned compile_shader( char *filePath, GLenum shaderType ) {

  unsigned shader = glCreateShader( shaderType );
  const char * const shaderStr = get_file_contents( filePath );

  if ( !shaderStr )
    return 0;

  glShaderSource( shader, 1, &shaderStr, NULL );

  int success = 0;
  char infoLog[ MAX_GL_INFO_LOG ] = { 0 };

  glCompileShader( shader );
  glGetShaderiv( shader, GL_COMPILE_STATUS, &success );

  if ( !success ) {
    glGetShaderInfoLog( shader, MAX_GL_INFO_LOG, NULL, infoLog );
    fprintf( stderr, "Shader compilation failed: %s\n", infoLog );
    return 0;
  }

  return shader;
}

unsigned create_shader_program( unsigned *shaders, int numShaders ) {

  unsigned shaderProgram = glCreateProgram();

  for ( int i = 0; i < numShaders; ++i )
    glAttachShader( shaderProgram, shaders[ i ] );

  int success = 0;
  char infoLog[ MAX_GL_INFO_LOG ] = { 0 };

  glLinkProgram( shaderProgram );
  glGetProgramiv( shaderProgram, GL_LINK_STATUS, &success );

  if ( !success ) {
    glGetProgramInfoLog( shaderProgram, MAX_GL_INFO_LOG, NULL, infoLog );
    fprintf( stderr, "Shader program linking failed: %s\n", infoLog );
    return 0;
  }

  return shaderProgram;
}

int main() {

  int quit = 0;
  SDL_Event event;

  if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
    fprintf( stderr, "Could not init SDL: %s\n", SDL_GetError() );
    return 1;
  }

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
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
  glViewport( 0, TOOLBAR_H, INITIAL_WIN_W, INITIAL_WIN_H );

  // ----------------------------------------------------

  unsigned vao;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );

  /*
    "Once your vertex coordinates have been processed in the vertex shader,
    they should be in normalized device coordinates which is a small space
    where the x, y and z values vary from -1.0 to 1.0."
   */
  float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.0f, 0.5f, 0.0f,
     0.5f,  -0.5f, 0.0f
  };
  unsigned vbo;
  glGenBuffers( 1, &vbo );
  // "OpenGL has many types of buffer objects and the buffer type of a vertex
  // buffer object is GL_ARRAY_BUFFER. OpenGL allows us to bind to several
  // buffers at once as long as they have a different buffer type."
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

  unsigned vertexShader = compile_shader( "shader.vert", GL_VERTEX_SHADER );
  unsigned fragShader = compile_shader( "shader.frag", GL_FRAGMENT_SHADER );
  unsigned shaders[] = { vertexShader, fragShader };
  unsigned shaderProgram = create_shader_program( shaders, 2 );

  glDeleteShader( vertexShader );
  glDeleteShader( fragShader );

  // "The vertex attribute is a vec3 so it is composed of 3 values."
  // "...the position vertex attribute in the vertex shader with layout (location = 0). This sets the location of the vertex attribute to 0..."
  // "The third argument specifies the type of the data which is GL_FLOAT (a vec* in GLSL consists of floating point values)."
  // "The next argument specifies if we want the data to be normalized."
  // "The fifth argument is known as the stride and tells us the space between consecutive vertex attribute sets."
  // "The last parameter is of type void* and thus requires that weird cast. This is the offset of where the position data begins in the buffer."
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*) 0 );
  glEnableVertexAttribArray( 0 );
  glUseProgram( shaderProgram );

  // ----------------------------------------------------

  Uint32 lastUpdate = SDL_GetTicks();
  double updateTimeLeft = 0;
  Uint32 gameTicks = 0;
  unsigned frames = 0;
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
            glViewport( 0, TOOLBAR_H, event.window.data1, event.window.data2 );
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
    glBindVertexArray( vao );
    glDrawArrays( GL_TRIANGLES, 0, 3 );
    glBindVertexArray( 0 );

    // FIXME: "On Mac OS X make sure you bind 0 to the draw framebuffer before
    // swapping the window, otherwise nothing will happen"
    SDL_GL_SwapWindow( window );
    ++frames;

    // ----------------------------------------------------

    if ( now - fpsStart >= 1000 ) {
      double dt = ( (double) now - (double) fpsStart ) / 1000.0;
      printf( "Ticks/sec: %d, FPS: %d\n", (int) ( gameTicks / dt ), (int) ( frames / dt ) );
      fpsStart = now;
      gameTicks = 0;
      frames = 0;
    }

    lastUpdate = now;
  }

  SDL_GL_DeleteContext( glCtx );
  SDL_DestroyWindow( window );
  SDL_Quit();

  return 0;
}

