#include <dlfcn.h>
#include <stdio.h>
#include "plug.h"

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])
Plug plug = {0};

void closeFunc(Plug *plug){
  UnloadMusicStream(plug->music);
  CloseAudioDevice();
  CloseWindow();
}

const char *libplug_file_name = "libplug.so";
void *libplug = NULL;

#define PLUG(name) name##_t name = NULL;
LIST_OF_PLUGS
#undef PLUG

int reload_libplug(void){
  if(libplug != NULL) dlclose(libplug);

  libplug = dlopen(libplug_file_name,RTLD_NOW);

  if(libplug == NULL){
    fprintf(stderr,"ERROR:could not load %s: %s",libplug_file_name,dlerror());
    return 1;
  }
  #define PLUG(name) \
    name = dlsym(libplug,#name); \ 
    if(name == NULL){ \
      fprintf(stderr,"ERROR:could not find %s symbol in %s: %s",#name,libplug_file_name,dlerror()); \ 
      return 1; \
    } 
  LIST_OF_PLUGS
  #undef PLUG
  // plug_hello = dlsym(libplug,"plug_hello");
  // if(plug_hello == NULL){
  //   fprintf(stderr,"ERROR:could not find plug_hello symbol in %s: %s",libplug_file_name,dlerror());
  //   return 1;
  // }
  // plug_init = dlsym(libplug,"plug_init");
  // if(plug_init == NULL){
  //   fprintf(stderr,"ERROR:could not find plug_init symbol in %s: %s",libplug_file_name,dlerror());
  //   return 1;
  // }
  // plug_update = dlsym(libplug,"plug_update");
  // if(plug_update == NULL){
  //   fprintf(stderr,"ERROR:could not find plug_update symbol in %s: %s",libplug_file_name,dlerror());
  //   return 1;
  // }
  // plug_pre_reload = dlsym(libplug,"plug_pre_reload");
  // if(plug_pre_reload == NULL){
  //   fprintf(stderr,"ERROR:could not find plug_pre_reload symbol in %s: %s",libplug_file_name,dlerror());
  //   return 1;
  // }
  // plug_post_reload = dlsym(libplug,"plug_post_reload");
  // if(plug_post_reload == NULL){
  //   fprintf(stderr,"ERROR:could not find plug_post_reload symbol in %s: %s",libplug_file_name,dlerror());
  //   return 1;
  // }
  plug_hello();
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

  plug_init(&plug);

  while(!WindowShouldClose()){
    if(IsKeyPressed(KEY_R)){
      plug_pre_reload(&plug);
      if(reload_libplug())return 1;
      plug_post_reload(&plug);
    }
    plug_update(&plug);
    if(IsKeyPressed(KEY_Q))break;
  }
  closeFunc(&plug);
  return 0;
}
