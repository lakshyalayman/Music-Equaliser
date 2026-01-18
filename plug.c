#include <assert.h>
#include <stddef.h>
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
float in1[N];
float complex out[N];

float amp(float complex z){
  float a = crealf(z);
  float b = cimagf(z);
  return logf(a*a + b*b);
}

void callback(void *bufferData,unsigned int frames){
  // Frame *fs = bufferData;
  float (*fs)[plug->music.stream.channels] = bufferData;
  for(size_t i = 0;i<frames;++i){
    memmove(in,in+1,(N-1)*sizeof(in[0]));
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
  SetMusicVolume(plug->music,0.3f);
}

void plug_unload_stream(void){
  UnloadMusicStream(plug->music);
  free(plug);
  printf("Freeing plug\n");
  CloseAudioDevice();
  CloseWindow();
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
      SetMusicVolume(plug->music,0.2f);
    }
    marker = !marker;
  }

  int w = GetRenderWidth();
  float h = (float)GetRenderHeight();
    // printf("%d %d\n",w,h);
  BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
    if(plug->music.ctxData != NULL){
      //https://en.wikipedia.org/wiki/Hann_function
      for(size_t i = 0;i<N;i++){
        float t = (float)i/(N-1);
        float hanning = 0.5 - 0.5*cosf(2*PI*t);
        in1[i] = in[i]*hanning;
      }
      fft(in1,1,out,N);
      float max_amp = -100.0f;

      for(size_t i = 0;i<N;++i){
        float a = amp(out[i]);
        if(max_amp < a) max_amp = a;
      }

      float step = 1.06f;
      size_t m = 1;
      float lowf =20.0f;
      for(float f = lowf;(size_t) f <= N/2; f=ceilf(f*step))m+=1;
      float noise_floor = -7.0f;
      float cell_width = (float)w/m;
      m = 1;
      // float cell_width = 10;
      for(float f = lowf;(size_t)f<=N/2;f=ceilf(f*step)){
        // float t = (float) amp(out[i]); 
        float f1 = ceilf(f*step);
        float a = noise_floor;
        for(size_t q = (size_t) f;q<=N/2&&q<(size_t) f1;++q){
          float b= amp(out[q]);
          if(b > a) a = b;
        }
        float t = (a-noise_floor)/(max_amp-noise_floor);
        if(t<0.0f)t = 0.0f;
        DrawRectangle(m*cell_width,h-h/2*t,cell_width,h/2*t,GOLD);
        m+=1;
      }
  }else{
      if(plug->error) DrawText("Eroor :(",0,0,70,RED);
      else DrawText("Drop music",1,1,70,GOLD);
    }

  EndDrawing();
}
