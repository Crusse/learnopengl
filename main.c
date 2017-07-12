#include <stdio.h>
#include <math.h>
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

typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3_t;

typedef struct shader {
  unsigned programId;
} shader_t;

typedef struct game_object {
  unsigned vao;
  unsigned vertex_count;
} game_object_t;

SDL_Window *window;
SDL_GLContext *glContext;

int r_init() {

  if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
    fprintf( stderr, "Could not init SDL: %s\n", SDL_GetError() );
    return 0;
  }

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

  window = SDL_CreateWindow(
    "OpenGL test",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    INITIAL_WIN_W,
    INITIAL_WIN_H,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
  );

  if ( window == NULL ) {
    fprintf( stderr, "Could not create window: %s\n", SDL_GetError() );
    return 0;
  }

  glContext = SDL_GL_CreateContext( window );

  if ( !gladLoadGLLoader( (GLADloadproc) SDL_GL_GetProcAddress ) ) {
    fprintf( stderr, "Could not init GLAD\n" );
    return 0;
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
  glViewport( 0, TOOLBAR_H, INITIAL_WIN_W, INITIAL_WIN_H - TOOLBAR_H );

  return 1;
}

void r_destroy() {

  SDL_GL_DeleteContext( glContext );
  SDL_DestroyWindow( window );
  SDL_Quit();
}

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

  *i = '\0';

  return ret;
}

unsigned compile_shader( const char *filePath, GLenum shaderType ) {

  unsigned shader = glCreateShader( shaderType );
  const char * const shaderStr = get_file_contents( filePath );

  if ( !shaderStr ) {
    fprintf( stderr, "File not found: %s\n", filePath );
    return 0;
  }

  glShaderSource( shader, 1, &shaderStr, NULL );

  int success = 0;
  char infoLog[ MAX_GL_INFO_LOG ] = { 0 };

  glCompileShader( shader );
  glGetShaderiv( shader, GL_COMPILE_STATUS, &success );

  if ( !success ) {
    glGetShaderInfoLog( shader, MAX_GL_INFO_LOG, NULL, infoLog );
    fprintf( stderr, "Shader %s compilation failed: %s\n", filePath, infoLog );
    return 0;
  }

  return shader;
}

shader_t* create_shader( const char *vertexShaderPath, const char *fragShaderPath ) {

  unsigned shaderProgram = glCreateProgram();

  unsigned vertexShader = compile_shader( vertexShaderPath, GL_VERTEX_SHADER );
  glAttachShader( shaderProgram, vertexShader );

  unsigned fragShader = compile_shader( fragShaderPath, GL_FRAGMENT_SHADER );
  glAttachShader( shaderProgram, fragShader );

  int success = 0;
  char infoLog[ MAX_GL_INFO_LOG ] = { 0 };

  glLinkProgram( shaderProgram );
  glGetProgramiv( shaderProgram, GL_LINK_STATUS, &success );

  glDeleteShader( vertexShader );
  glDeleteShader( fragShader );

  if ( !success ) {
    glGetProgramInfoLog( shaderProgram, MAX_GL_INFO_LOG, NULL, infoLog );
    fprintf( stderr, "Shader program linking failed: %s\n", infoLog );
    return 0;
  }

  shader_t *ret = malloc( sizeof( shader_t ) );
  ret->programId = shaderProgram;

  return ret;
}

void activate_shader( const shader_t *shader ) {
  glUseProgram( shader->programId );
}

game_object_t* create_object( const vec3_t *locations, const vec3_t *colors, unsigned nVertices ) {

  unsigned vao;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );

  unsigned vbo;
  vec3_t vertices[ nVertices * 2 ]; // VLA

  for ( unsigned i = 0; i < nVertices; ++i ) {
    vertices[ i ] = locations[ i ];
    vertices[ nVertices + i ] = colors[ i ];
  }

  glGenBuffers( 1, &vbo );
  // "OpenGL has many types of buffer objects and the buffer type of a vertex
  // buffer object is GL_ARRAY_BUFFER. OpenGL allows us to bind to several
  // buffers at once as long as they have a different buffer type."
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  // This apparently transfers data to GPU memory.
  // https://cognitivewaves.wordpress.com/opengl-terminology-demystified/
  glBufferData( GL_ARRAY_BUFFER, nVertices * 2 * sizeof( vec3_t ), vertices, GL_STATIC_DRAW );

  // "...the position vertex attribute in the vertex shader with layout (location = 0). This sets the location of the vertex attribute to 0..."
  // "The vertex attribute is a vec3 so it is composed of 3 values."
  // "The third argument specifies the type of the data which is GL_FLOAT (a vec* in GLSL consists of floating point values)."
  // "The next argument specifies if we want the data to be normalized."
  // "The fifth argument is known as the stride and tells us the space between consecutive vertex attribute sets."
  // "The last parameter is of type void* and thus requires that weird cast. This is the offset of where the position data begins in the buffer."
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*) 0 );
  glEnableVertexAttribArray( 0 );

  glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*) ( nVertices * sizeof( vec3_t ) ) );
  glEnableVertexAttribArray( 1 );

  glBindVertexArray( 0 );
  glBindBuffer( GL_ARRAY_BUFFER, 0 );

  game_object_t *obj = malloc( sizeof( game_object_t ) );
  if ( !obj )
    return NULL;

  obj->vao = vao;
  obj->vertex_count = nVertices;

  return obj;
}

void render_object( const game_object_t *obj ) {

  glBindVertexArray( obj->vao );
  glDrawArrays( GL_TRIANGLES, 0, obj->vertex_count );
  glBindVertexArray( 0 );
}

int main() {

  if ( !r_init() )
    return 1;

  // ----------------------------------------------------

  game_object_t *obj1 = create_object(
    (vec3_t[]) { 
      { -1.0f, -1.0f, 0.0f },
      { -0.5f, 0.0f, 0.0f },
      { 0.0f,  -1.0f, 0.0f }
    },
    (vec3_t[]) { 
      { 1.0f, 0.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f }
    },
    3
  );
  game_object_t *obj2 = create_object(
    (vec3_t[]) {
      { 0.0f, 0.0f, 0.0f },
      { 0.5f, 1.0f, 0.0f },
      { 1.0f,  0.0f, 0.0f }
    },
    (vec3_t[]) { 
      { 1.0f, 1.0f, 1.0f },
      { 0.666f, 0.666f, 0.666f },
      { 0.333f, 0.333f, 0.333f }
    },
    3
  );

  shader_t *shader1 = create_shader( "shader.vert",  "shader.frag" );
  shader_t *shader2 = create_shader( "shader.vert",  "shader2.frag" );

  // ----------------------------------------------------

  Uint32 lastUpdate = SDL_GetTicks();
  double updateTimeLeft = 0;
  Uint32 gameTicks = 0;
  unsigned frames = 0;
  Uint32 fpsStart = lastUpdate;
  SDL_Event event;
  int quit = 0;

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
            glViewport( 0, TOOLBAR_H, event.window.data1, event.window.data2 - TOOLBAR_H );
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

    float lightness = sinf( now / 1000.0f ) / 2.0f + 0.5f;
    int lightnessLocation = glGetUniformLocation( shader1->programId, "lightness" );

    activate_shader( shader1 );
    glUniform1f( lightnessLocation, lightness );
    render_object( obj1 );

    activate_shader( shader2 );
    lightnessLocation = glGetUniformLocation( shader2->programId, "lightness" );
    glUniform1f( lightnessLocation, 0.5 );
    render_object( obj2 );

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

  r_destroy();

  return 0;
}

