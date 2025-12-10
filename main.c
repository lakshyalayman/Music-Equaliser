#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <raylib.h>

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])

typedef struct {
  float left;
  float right;
} Frame;

Frame global_frames[4410*2] = {0};
size_t global_frame_count = 0;

void callback(void *bufferData,unsigned int frames){
  size_t cap = ARRAY_LEN(global_frames);
  if(frames <= cap - global_frame_count){
    memcpy(global_frames+global_frame_count,bufferData,frames*sizeof(Frame));
    global_frame_count += frames;
  }else if(frames <= cap){
    memmove(global_frames,global_frames + frames,(cap-frames)*sizeof(Frame));
    memcpy(global_frames + (cap - frames),bufferData,frames*sizeof(Frame));
  }else{
    memcpy(global_frames,bufferData,cap*sizeof(Frame));
    global_frame_count = cap;
  }


  /*
  if(frames > ARRAY_LEN(global_frames)){
    frames = ARRAY_LEN(global_frames);
  }
  memcpy(global_frames,bufferData,frames*sizeof(int64_t));
  global_frame_count = frames;
  */
}
void closeFunc(Music music){
  UnloadMusicStream(music);
  CloseAudioDevice();
  CloseWindow();
}
int main(void)
{
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
      // closeFunc(music);
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
    // printf("%d %d\n",w,h);
    BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
    float cell_width = (float)w/global_frame_count;
    for(size_t i = 0;i<global_frame_count;++i){
        float t = global_frames[i].left;
        // float *fp = (float *)&packed;
        // float left = fp[0];
        // float right = fp[1];
        // float t = left;
        // float p = right;
        int x = (int)(i*cell_width);
        int barW = (int)(cell_width > 1.0f ? cell_width : 1.0f);
        if(t >= 0){
          // int barH = (int)(t * (h/2.0f));
          // DrawRectangle(x,h/2 - barH,barW,barH*2,RED);
          // DrawRectangle(x,h/2 - barH,1,barH*2,RED);
          // DrawRectangle(x,h/2-barH,1,barH,RED);
          DrawRectangle(i*cell_width,h/2-h/2*t,1,h/2*t,RED);
        }else{
          // int barH = (int)(-t * (h/2.0f));
          DrawRectangle(i*cell_width,h/2,1,h/2*t,MAGENTA);
        }
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

