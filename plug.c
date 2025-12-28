#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <stdio.h>
#include "plug.h"
#include <math.h>

typedef struct {
  bool error;
  Music music;
} Plug;

Plug *plug = NULL;
#define N (1 << 14)

float in[N];
float complex out[N];

float amp(float complex z){
  float a = fabsf(crealf(z));
  float b = fabsf(cimagf(z));
  if(a > b) return a;
  return b;
}

void callback(void *bufferData,unsigned int frames){
  // Frame *fs = bufferData;
  float (*fs)[plug->music.stream.channels] = bufferData;
  for(size_t i = 0;i<frames;++i){
    memmove(in,in+2,(N-2)*sizeof(in[0]));
    in[N-1] = fs[i][0];
  }
}

void fft(float in[],size_t stride,float complex out[],size_t n){
  assert(n > 0);
  if(n == 1){
    out[0] = in[0];
    return;
  }

  fft(in,stride*2,out,n/2);
  fft(in+stride,stride*2,out+n/2,n/2);

  for(size_t k = 0;k<n/2;++k){
    float t = (float)k/n;
    float complex v = cexp(-2*PI*I*t)*out[k+n/2];
    float complex e = out[k];
    out[k] = e+v;
    out[k+n/2] = e-v;
  }
}

void plug_init(void){
  plug = malloc(sizeof(*plug));
  assert(plug != NULL && "assert failure at plug_init");
  memset(plug,0,sizeof(*plug));
}

void plug_src_init(const char *src){
  plug = malloc(sizeof(*plug));
  assert(plug != NULL && "assert failure at plug_src_init");
  memset(plug,0,sizeof(*plug));
  plug->music = LoadMusicStream(src);
  PlayMusicStream(plug->music);
  AttachAudioStreamProcessor(plug->music.stream,callback);
}

void plug_unload_stream(void){
  UnloadMusicStream(plug->music);
  CloseAudioDevice();
  CloseWindow();
  printf("Freeing plug\n");
  free(plug);
}

Plug *plug_pre_reload(void){
  if(IsMusicValid(plug->music))
    DetachAudioStreamProcessor(plug->music.stream,callback);
  return plug;
}

void plug_post_reload(Plug *state){
  plug = state;
  if(IsMusicValid(plug->music))
    AttachAudioStreamProcessor(plug->music.stream,callback);
}

bool marker = false;
void plug_update(void){
  if(IsMusicValid(plug->music)) UpdateMusicStream(plug->music);

  if(IsFileDropped()){
    FilePathList dfile = LoadDroppedFiles();
    printf("%i\n%i\n",dfile.capacity,dfile.count);
    const char *dropped_path = dfile.paths[0];
    if(IsMusicValid(plug->music)){
      StopMusicStream(plug->music);
      UnloadMusicStream(plug->music);
    }
    plug->music = LoadMusicStream(dropped_path);
    if(IsMusicValid(plug->music)){
      plug->error = false;
      PlayMusicStream(plug->music);
      AttachAudioStreamProcessor(plug->music.stream,callback);
      UnloadDroppedFiles(dfile);
    }else plug->error = true;
  }

  if(IsKeyPressed(KEY_SPACE) && IsMusicValid(plug->music)){
      if(IsMusicStreamPlaying(plug->music)){
        PauseMusicStream(plug->music);
      }else{
        ResumeMusicStream(plug->music);
      }
  }
    
  if(IsKeyPressed(KEY_MINUS) && IsMusicValid(plug->music)){
    if(!marker){
      SetMusicVolume(plug->music,0.0f);
    }else{
      SetMusicVolume(plug->music,0.5f);
    }
    marker = !marker;
  }

  int w = GetRenderWidth();
  float h = (float)GetRenderHeight();
    // printf("%d %d\n",w,h);
  BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
    if(plug->music.ctxData != NULL){
      fft(in,1,out,N);
      float max_amp = 0.0f;

      for(size_t i = 0;i<N;++i){
        float a = amp(out[i]);
        if(max_amp < a) max_amp = a;
      }

      float step = 1.1;
      size_t m = 0;
      for(float f = 20.0;(size_t) f < N; f*= step)m+=1;
    
      float cell_width = (float)w/m;
      m = 0;
      // float cell_width = 10;
      for(float f = 20.0;(size_t)f<N;f*=step){
        // float t = (float) amp(out[i]); 
        float f1 = f*step;
        float a = 0.0f;
        for(size_t q = (size_t) f;q<N&&q<(size_t) f1;++q){
          a+= amp(out[q]);
        }
        a/=(size_t)f1 - (size_t) f + 1;
        float t = a/(max_amp);
        // float t = a/100;
        DrawRectangle(m*cell_width,h-h/2*t,cell_width,h/2*t,GOLD);
        m+=1;
        // printf()
      }
  }else{
      if(plug->error) DrawText("Eroor :(",0,0,70,RED);
      else DrawText("Drop music",1,1,70,GOLD);
    }

  EndDrawing();
}
