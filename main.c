#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <raylib.h>

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])

uint64_t global_frames[1024] = {0};
size_t global_frame_count = 0;

void callback(void *bufferData,unsigned int frames){
  if(frames > ARRAY_LEN(global_frames)){
    frames = ARRAY_LEN(global_frames);
  }

  memcpy(global_frames,bufferData,frames*sizeof(uint64_t));
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
  InitWindow(800,600,"Vamos");
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
    BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});
    float cell_width = (float)GetRenderWidth()/global_frame_count;
    for(size_t i = 0;i<global_frame_count;++i){
       int32_t sample = *(int32_t*)&global_frames[i]; 
       printf(" %d ",sample);
     }
    printf("\n");
    // if(global_frame_count > 0) exit(1);
    EndDrawing();
  }
  closeFunc(music);
  return 0;
}
