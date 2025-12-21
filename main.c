#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include <unistd.h>
#include <raylib.h>
#include <assert.h>
// #include <math.h>
#include <complex.h>
#include "plug.h"

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])
#define N 512 
Plug plug = {0};

// typedef struct {
//   float left;
//   float right;
// } Frame;

// float amp(float complex z){
//   float a = fabsf(crealf(z));
//   float b = fabsf(cimagf(z));
//   if(a > b) return a;
//   return b;
// }

// void fft(float in[],size_t stride,float complex out[],size_t n){
//   assert(n > 0);
//   if(n == 1){
//     out[0] = in[0];
//     return;
//   }
//
//   fft(in,stride*2,out,n/2);
//   fft(in+stride,stride*2,out+n/2,n/2);
//
//   for(size_t k = 0;k<n/2;++k){
//     float t = (float)k/n;
//     float complex v = cexp(-2*pi*I*t)*out[k+n/2];
//     float complex e = out[k];
//     out[k] = e+v;
//     out[k+n/2] = e-v;
//   }
// }

// void callback(void *bufferData,unsigned int frames){
//   if(frames < N) return;
//   Frame *fs = bufferData;
//   for(size_t i = 0;i<frames;++i){
//     in[i] = fs[i].left;
//   }
//   fft(in,1,out,N);
//   max_amp = 0.0f;
//
//   for(size_t i = 0;i<frames;++i){
//     float a = amp(out[i]);
//     if(max_amp < a) max_amp = a;
//   }
// }

void closeFunc(Plug *plug){
  UnloadMusicStream(plug->music);
  CloseAudioDevice();
  CloseWindow();
}

const char *libplug_file_name = "libplug.so";
void *libplug = NULL;
plug_hello_t plug_hello = NULL;
plug_init_t plug_init = NULL;
plug_update_t plug_update = NULL;

int reload_libplug(void){
  if(libplug != NULL) dlclose(libplug);

  libplug = dlopen(libplug_file_name,RTLD_NOW);

  if(libplug == NULL){
    fprintf(stderr,"ERROR:could not load %s: %s",libplug_file_name,dlerror());
    return 1;
  }
  plug_hello = dlsym(libplug,"plug_hello");
  if(plug_hello == NULL){
    fprintf(stderr,"ERROR:could not find plug_hello symbol in %s: %s",libplug_file_name,dlerror());
    return 1;
  }
  plug_init = dlsym(libplug,"plug_init");
  if(plug_init == NULL){
    fprintf(stderr,"ERROR:could not find plug_init symbol in %s: %s",libplug_file_name,dlerror());
    return 1;
  }
  plug_update = dlsym(libplug,"plug_update");
  if(plug_update == NULL){
    fprintf(stderr,"ERROR:could not find plug_update symbol in %s: %s",libplug_file_name,dlerror());
    return 1;
  }

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
      if(reload_libplug())return 1;
    }
    plug_update(&plug);
    if(IsKeyPressed(KEY_Q))break;
  }
  closeFunc(&plug);
  return 0;
}
