#include <assert.h>
#include <stdio.h>
#include "plug.h"
#include <math.h>

typedef struct {
  float left;
  float right;
} Frame;

float in[N];
float complex out[N];

float amp(float complex z){
  float a = fabsf(crealf(z));
  float b = fabsf(cimagf(z));
  if(a > b) return a;
  return b;
}

void callback(void *bufferData,unsigned int frames){
  if(frames < N) return;
  Frame *fs = bufferData;
  for(size_t i = 0;i<frames;++i){
    in[i] = fs[i].left;
  }
}

void closeFunc(Music music){
  UnloadMusicStream(music);
  CloseAudioDevice();
  CloseWindow();
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

void plug_hello(){
  printf("hello, world\n");
}

void plug_init(Plug *plug){

  plug->music = LoadMusicStream("letItHappen.mp3");
  SetMusicVolume(plug->music,0.3);
  PlayMusicStream(plug->music);
  // float x = GetMusicTimeLength(plug->music);
  // printf("%f\n",x/60.0);
  printf("frameCount: %u\n",plug->music.frameCount);
  printf("channels: %u\n",plug->music.stream.channels);
  printf("sampleRate: %u\n",plug->music.stream.sampleRate);
  printf("sampleSize: %u\n",plug->music.stream.sampleSize);
  AttachAudioStreamProcessor(plug->music.stream,callback);
}

void plug_update(Plug *plug){
    UpdateMusicStream(plug->music);

    if(IsKeyPressed(KEY_SPACE)){
      if(IsMusicStreamPlaying(plug->music)){
        PauseMusicStream(plug->music);
      }else{
        ResumeMusicStream(plug->music);
      }
    }

    int w = GetRenderWidth();
    float h = (float)GetRenderHeight();
    float cell_width = (float)w/N;
    // printf("%d %d\n",w,h);
    BeginDrawing();
      ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
      fft(in,1,out,N);
      float max_amp = 0.0f;

      for(size_t i = 0;i<N;++i){
        float a = amp(out[i]);
        if(max_amp < a) max_amp = a;
      }

      
      for(size_t i = 0;i<N;++i){
        // float t = (float) amp(out[i]); 
        float t = (float) amp(out[i])/max_amp;
        DrawRectangle(i*cell_width,h-h/2*t,cell_width,h/2*t,WHITE);
      }

    EndDrawing();
}
