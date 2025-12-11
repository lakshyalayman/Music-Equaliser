#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <raylib.h>
#include <assert.h>
#include <math.h>
#include <complex.h>

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])
#define N 512

float pi;
float max_amp;

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
    float complex v = cexp(-2*pi*I*t)*out[k+n/2];
    float complex e = out[k];
    out[k] = e+v;
    out[k+n/2] = e-v;
  }
}

void callback(void *bufferData,unsigned int frames){
  if(frames < N) return;
  Frame *fs = bufferData;
  for(size_t i = 0;i<frames;++i){
    in[i] = fs[i].left;
  }
  fft(in,1,out,N);
  max_amp = 0.0f;

  for(size_t i = 0;i<frames;++i){
    float a = amp(out[i]);
    if(max_amp < a) max_amp = a;
  }
}

void closeFunc(Music music){
  UnloadMusicStream(music);
  CloseAudioDevice();
  CloseWindow();
}
int main(void)
{
  pi = atan2f(1,1)*4;
  printf("Hello World\n");
  InitWindow(1200,800,"Vamos");
  SetTargetFPS(60);

  InitAudioDevice();
  Music music = LoadMusicStream("daft.mp3");
  PlayMusicStream(music);
  float x = GetMusicTimeLength(music);
  printf("%f\n",x/60.0);
  printf("frameCount: %u\n",music.frameCount);
  printf("channels: %u\n",music.stream.channels);
  printf("sampleRate: %u\n",music.stream.sampleRate);
  printf("sampleSize: %u\n",music.stream.sampleSize);
  AttachAudioStreamProcessor(music.stream,callback);
  while(!WindowShouldClose()){
    UpdateMusicStream(music);
    if(IsKeyPressed(KEY_Q)){
      break;
    }
    if(IsKeyPressed(KEY_SPACE)){
      if(IsMusicStreamPlaying(music)){
        PauseMusicStream(music);
      }else{
        ResumeMusicStream(music);
      }
    }
    int w = GetRenderWidth();
    float h = (float)GetRenderHeight();
    float cell_width = (float)w/N * 2;
    // printf("%d %d\n",w,h);
    BeginDrawing();
      ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
      for(size_t i = 0;i<N/2;++i){
        float t = (float) amp(out[i]); 
        // float t = (float) amp(out[i])/max_amp;
        DrawRectangle(i*cell_width,h-h/2*t,cell_width,h/2*t,RED);
        // DrawRectangle(int posX, int posY, int width, int height, Color color)
      }

    EndDrawing();
  }
  closeFunc(music);
  return 0;
}
    /*
    for(size_t i = 0;i<global_frame_count;++i){
       int32_t sample = *(int32_t*)&global_frames[i]; 
       // printf(" %d ",sample);
       if(sample > 0){
        float t = (float)sample/INT32_MAX;
        DrawRectangle(i*cell_width,(float)h/2 - (float)h/2*t,cell_width,(float)h/2*t,RED);
       }else{
        float t = (float)sample/INT32_MIN;
        DrawRectangle(i*cell_width,h/2,cell_width,(float)h/2*t,RED);
        // DrawRectangle(int posX, int posY, int width, int height, Color color)
       }
     }*/
    // for(size_t i = 0;i<global_frame_count;++i){
    //     float t = global_frames[i].left;
    //     int x = (int)(i*cell_width);
    //     int barW = (int)(cell_width > 1.0f ? cell_width : 1.0f);
    //     if(t >= 0){
    //       DrawRectangle(i*cell_width,h/2-h/2*t,1,h/2*t,RED);
    //     }else{
    //       DrawRectangle(i*cell_width,h/2,1,h/2*t,MAGENTA);
    //     }
    //   }
