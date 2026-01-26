#include <dlfcn.h>
#include <stdio.h>
#include "plug.h"

//dynamic library
const char *libplug_file_name = "libplug.so";
void *libplug = NULL;

//X-macro
#define PLUG(name,...) name##_t *name = NULL;
LIST_OF_PLUGS
#undef PLUG

int reload_libplug(void){
  if(libplug != NULL) dlclose(libplug);

  libplug = dlopen(libplug_file_name,RTLD_NOW);

  if(libplug == NULL){
    fprintf(stderr,"ERROR:could not load %s: %s",libplug_file_name,dlerror());
    return 1;
  }
  #define PLUG(name,...) \
    name = dlsym(libplug,#name); \ 
    if(name == NULL){ \
      fprintf(stderr,"ERROR:could not find %s symbol in %s: %s",#name,libplug_file_name,dlerror()); \ 
      return 1; \
    } 
  LIST_OF_PLUGS
  #undef PLUG
  return 0;
}

int main(void)
{
  if(reload_libplug()){
    return 1;
  }
  InitWindow(1200,800,"Vamos");
  SetTargetFPS(60);
  InitAudioDevice();
  char *file_path = "m1.mp3";

  /*
    plug_init for drag and drop setup by default 
    plug_src_init for a source file init by default
    */

  // plug_init();
  plug_src_init(file_path);
  while(!WindowShouldClose()){
    if(IsKeyPressed(KEY_R)){
      void *state = plug_pre_reload();
      if(reload_libplug())return 1;
      plug_post_reload(state);
      printf("reloaded\n");
    }
    plug_update();
    if(IsKeyPressed(KEY_Q))break;
  }
  plug_unload_stream();
  return 0;
}
