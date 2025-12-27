#include <assert.h>
#include <string.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include "plug.h"
#include <math.h>

#define N (1 << 14)

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
  Frame *fs = bufferData;
  for(size_t i = 0;i<frames;++i){
    memmove(in,in+2,(N-2)*sizeof(in[0]));
    in[N-1] = fs[i].left;
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

  plug->music = LoadMusicStream("loser.mp3");
  SetMusicVolume(plug->music,0.2);
  PlayMusicStream(plug->music);
  // float x = GetMusicTimeLength(plug->music);
  // printf("%f\n",x/60.0);
  printf("frameCount: %u\n",plug->music.frameCount);
  printf("channels: %u\n",plug->music.stream.channels);
  printf("sampleRate: %u\n",plug->music.stream.sampleRate);
  printf("sampleSize: %u\n",plug->music.stream.sampleSize);
  AttachAudioStreamProcessor(plug->music.stream,callback);
}

void plug_set_volume(Plug *plug,float t){
  SetMusicVolume(plug->music,t);
}

void plug_pre_reload(Plug *plug){
  DetachAudioStreamProcessor(plug->music.stream,callback);
}

void plug_post_reload(Plug *plug){
  AttachAudioStreamProcessor(plug->music.stream,callback);
}

bool marker = true;
void plug_update(Plug *plug){
  UpdateMusicStream(plug->music);

  if(IsFileDropped()){
    FilePathList dfile = LoadDroppedFiles();
    printf("File dropped\n");
    // if(dfile.capacity == 0){
    //   printf("can't work\n");
    // }else{
      // printf("%i\n%i\n",dfile.capacity,dfile.count);
      // for(size_t i = 0;i<dfile.count;++i){
      //   printf("\t%s\n",dfile.paths[i]);
      // }
    UnloadDroppedFiles(dfile);
  }    

  if(IsKeyPressed(KEY_SPACE)){
    if(IsMusicStreamPlaying(plug->music)){
      PauseMusicStream(plug->music);
    }else{
      ResumeMusicStream(plug->music);
    }
  }
    
  if(IsKeyPressed(KEY_MINUS)){
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
    fft(in,1,out,N);
    float max_amp = 0.0f;

    for(size_t i = 0;i<N;++i){
      float a = amp(out[i]);
      if(max_amp < a) max_amp = a;
    }

    float step = 1.06;
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
      // float t = a;
      DrawRectangle(m*cell_width,h-h/2*t,cell_width,h/2*t,SKYBLUE);
      m+=1;
      // printf()
    }

  EndDrawing();
}
