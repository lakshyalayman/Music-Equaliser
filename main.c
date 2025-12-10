#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <raylib.h>

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])

int64_t global_frames[4800] = {0};
size_t global_frame_count = 0;

void callback(void *bufferData,unsigned int frames){
  if(frames > ARRAY_LEN(global_frames)){
    frames = ARRAY_LEN(global_frames);
  }

  memcpy(global_frames,bufferData,frames*sizeof(int64_t));
  global_frame_count = frames;
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
    int h = GetRenderHeight();
    // printf("%d %d\n",w,h);
    BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
    float cell_width = (float)w/global_frame_count;
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
    for(size_t i = 0;i<global_frame_count;++i){
        uint64_t packed = global_frames[i];
        float *fp = (float *)&packed;
        float left = fp[0];
        float right = fp[1];
        float t = left;
        int x = (int)(i*cell_width);
        int barW = (int)(cell_width > 1.0f ? cell_width : 1.0f);
        if(t >= 0){
          int barH = (int)(t * (h/2.0f));
          DrawRectangle(x,h/2 - barH,barW,barH,RED);
        }else{
          int barH = (int)(-t * (h/2.0f));
          DrawRectangle(x,h/2,barW,barH,RED);
        }
      }
    // printf("%lu\n",global_frame_count);
    // printf("\n");
    // if(global_frame_count > 0) exit(1);
    EndDrawing();
  }
  closeFunc(music);
  return 0;
}
