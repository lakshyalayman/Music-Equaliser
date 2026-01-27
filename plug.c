#include <assert.h>
#include <raylib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "plug.h"
#include <math.h>

bool filter = false;
//Plug struct for hot reloading,plug->music for drag and drop
typedef struct {
  bool error;
  Music music;
  float loadL;
  float loadR;
} Plug;

Plug *plug = NULL;

typedef enum {
  CALLBACK,
  CALLBACK_LPF,
  CALLBACK_HPF,
} filterType;

filterType currentFilter = CALLBACK;

#define N (1 << 13) 

float in[N];
float in1[N];
float complex out[N];
float out_log[N];
float out_smooth[N];

// Amplitude funciton for a complex input which is under root of square of both real and imaginary part of the complex number
float amp(float complex z){
  float a = crealf(z);
  float b = cimagf(z);
  return logf(a*a + b*b);
}


//Callback function which accepts the music buffer and frames, frames is the number of frames per second, the function iterates over the number of frames and performs a ring buffer by placing a single sample in the buffer and removing the other from other side of the buffer
void callback(void *bufferData,unsigned int frames){
  // Frame *fs = bufferData;
  float (*fs)[2] = bufferData;
  for(size_t i = 0;i<frames;++i){
    memmove(in,in+1,(N-1)*sizeof(in[0]));
    in[N-1] = fs[i][0];
  }
}

void callbackLPF(void *bufferdata,unsigned int frames){
  float(*fs)[2] = bufferdata;
  const float cutoff = 1000.0f/44100.0f;
  const float k = cutoff/(cutoff + 0.1591549431f);
  for(size_t i = 0;i<frames;++i){
    // const float change = fs[i][0];
    plug->loadL += k*(fs[i][0] - plug->loadL);
    plug->loadR += k*(fs[i][1] - plug->loadR);
    fs[i][0] = plug->loadL;
    fs[i][1] = plug->loadR;
    memmove(in,in+1,(N-1)*sizeof(in[0]));
    in[N-1] = plug->loadL;
  }
}

void callbackHPF(void *bufferData,unsigned int frames){
  float(*fs)[2] = bufferData;
  const float cutoff = 4000.0f/44100.0f;
  const float k = cutoff/(cutoff+0.1591549431f);
  for(size_t i = 0;i<frames;++i){
    plug->loadL += k*(fs[i][0] - plug->loadL);   
    plug->loadR += k*(fs[i][1] - plug->loadR);
    fs[i][0] = fs[i][0] - plug->loadL;
    fs[i][1] = fs[i][1] - plug->loadR;
    memmove(in,in+1,(N-1)*sizeof(in[0]));
    in[N-1] = fs[i][0];
  }
}

void switchFilter(Music music,filterType nextFilter){
  if(!IsMusicValid(music)) return;
  if(currentFilter == CALLBACK)DetachAudioStreamProcessor(music.stream, callback);
  else if(currentFilter==CALLBACK_LPF)DetachAudioStreamProcessor(music.stream,callbackLPF);
  else if(currentFilter==CALLBACK_HPF)DetachAudioStreamProcessor(music.stream,callbackHPF);

  if(nextFilter == CALLBACK)AttachAudioStreamProcessor(music.stream, callback);
  else if(nextFilter == CALLBACK_LPF)AttachAudioStreamProcessor(music.stream,callbackLPF);
  else if(nextFilter == CALLBACK_HPF)AttachAudioStreamProcessor(music.stream,callbackHPF);
}
// A fast fourier transform implementation 
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

// initialization without any source (drag and drop)
void plug_init(void){
  plug = malloc(sizeof(*plug));
  assert(plug != NULL && "assert failure at plug_init");
  memset(plug,0,sizeof(*plug));
}

// initialization using source + (drag and drop)
void plug_src_init(const char *src){
  if(plug==NULL){
    plug = malloc(sizeof(*plug));
    assert(plug != NULL && "assert failure at plug_src_init");
    memset(plug,0,sizeof(*plug));
  }
  if(IsMusicValid(plug->music))UnloadMusicStream(plug->music);
  plug->music = LoadMusicStream(src);
  if(IsMusicValid(plug->music)){
    PlayMusicStream(plug->music);
    AttachAudioStreamProcessor(plug->music.stream,callback);
    SetMusicVolume(plug->music,0.3f);
  }
}

// ending function unloads everything 
void plug_unload_stream(void){
  if(plug!=NULL){
    if(IsMusicValid(plug->music)){
      StopMusicStream(plug->music);
      if(filter == false)
        DetachAudioStreamProcessor(plug->music.stream,callback);
      else
        DetachAudioStreamProcessor(plug->music.stream,callbackLPF);
      UnloadMusicStream(plug->music);
    }
    free(plug);
    printf("Freeing plug\n");
    plug=NULL;
  }
  CloseAudioDevice();
  CloseWindow();
}

// Pre reload and post reload functions for hot reloading 
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

//main function for updating each frame and draw the visualization
bool marker = false;
void plug_update(void){
  if(IsMusicValid(plug->music)) UpdateMusicStream(plug->music);

  // drag and drop 
  if(IsFileDropped()){
    printf("file drop triggered\n");
    FilePathList dfile = LoadDroppedFiles();
    printf("%i\n%i\n",dfile.capacity,dfile.count);
    const char *dropped_path = dfile.paths[0];
    if(IsMusicValid(plug->music)){
      StopMusicStream(plug->music);
      DetachAudioStreamProcessor(plug->music.stream,callback);
      UnloadMusicStream(plug->music);
    }
    plug->music = LoadMusicStream(dropped_path);
    if(IsMusicValid(plug->music)){
      plug->error = false;
      PlayMusicStream(plug->music);
      AttachAudioStreamProcessor(plug->music.stream,callback);
    }else plug->error = true;
    UnloadDroppedFiles(dfile);
  }

  // Play pause
  if(IsKeyPressed(KEY_SPACE) && IsMusicValid(plug->music)){
      if(IsMusicStreamPlaying(plug->music)){
        PauseMusicStream(plug->music);
      }else{
        ResumeMusicStream(plug->music);
      }
  }
  if(IsKeyPressed(KEY_COMMA)){
    switchFilter(plug->music,CALLBACK_LPF);
    currentFilter = CALLBACK_LPF;
  } 
  if(IsKeyPressed(KEY_PERIOD)){
    switchFilter(plug->music,CALLBACK);
    currentFilter = CALLBACK;
  }
  if(IsKeyPressed(KEY_SLASH)){
    switchFilter(plug->music,CALLBACK_HPF);
    currentFilter = CALLBACK_HPF;
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
  float dt = GetFrameTime();
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

      float max_amp = 1.0f;
      float step = 1.06f;
      size_t m = 0;
      float lowf =1.0f;
      float noise_floor = -7.0f;
      float smoothness = 10.0f;
      
      for(float f = lowf;(size_t)f<=N/2;f=ceilf(f*step)){
        float f1 = ceilf(f*step);
        float a = 0.0f;
        for(size_t q = (size_t) f;q<=N/2&&q<(size_t) f1;++q){
          float b= amp(out[q]);
          if(b > a) a = b;
        }
        if(max_amp < a) max_amp = a;
        out_log[m++] = a;
      }
      for(size_t i = 0;i<m;i++){
        out_log[i] /= max_amp;
      }
      float cell_width = (float)w/m;
      
      for(size_t i = 0;i<m;i++){
        out_smooth[i] += smoothness*(out_log[i] - out_smooth[i])*dt;
      }
      for(size_t i = 0;i<m;i++){
        float t = out_smooth[i];
        float hue = 240.0f - (240.0f*t);
        if(hue < 0.0f) hue = 0.0f;
        Color color = ColorFromHSV(hue,1.0f,1.0f);
        // DrawRectangle(i*cell_width,h-h/2*t,ceilf(cell_width),h/2*t,color);
        Vector2 startPos = {
          cell_width*(0.5 + i),
          h-7*h/8*t
        };
        Vector2 endPos = {
          cell_width*(0.5 + i),
          h
        };
        DrawLineEx(startPos,endPos,cell_width/2,color);
        DrawCircleV(startPos,cell_width*sqrtf(t),color);
        // DrawCircleLinesV(startPos,cell_width*sqrtf(t), color);

      }
  }else{
      if(plug->error) DrawText("Eroor :(",0,0,70,RED);
      else DrawText("Drop music",1,1,70,GOLD);
    }

  EndDrawing();
}
