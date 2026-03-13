#include "plug.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N (1 << 14)
// #define SINE_WAVE
#define MULTI_WAVE
// #define LINE
#define EQ_MODE

// Plug struct for hot reloading,plug->music for drag and drop
typedef struct {
  bool error;
  Music music;
  float loadL;
  float loadR;
  float midL;
  float midR;
  float lowL;
  float lowR;
  float volume;
  float bassGain;
  float trebleGain;
  float midGain;
  float bassCutoff;
  float trebleCutoff;
  float midLow;
  float midHigh;
} Plug;

typedef struct {
  float data[N];
  size_t head;
} RingBuffer;

Plug *plug = NULL;
RingBuffer ring;

typedef enum {
  CALLBACK,
  CALLBACK_LPF,
  CALLBACK_HPF,
  CALLBACK_PAN,
  CALLBACK_BB,
  CALLBACK_TRB,
  CALLBACK_CLR,
  CALLBACK_EQ_MODE
} filterType;

#ifdef EQ_MODE
filterType currentFilter = CALLBACK_EQ_MODE;
#else
filterType currentFilter = CALLBACK;
#endif

float in[N];
float in1[N];
float complex out[N];
float out_log[N];
float out_smooth[N];

void rb_push(RingBuffer *rb, float sample) {
  rb->data[rb->head] = sample;
  rb->head = (rb->head + 1) % N;
}

void rb_read(RingBuffer *rb, float dst[N]) {
  size_t h = rb->head;
  for (size_t i = 0; i < N; i++) {
    dst[i] = rb->data[(h + i) % N];
  }
}

// Amplitude funciton for a complex input which is under root of square of both
// real and imaginary part of the complex number
float amp(float complex z) {
  float a = crealf(z);
  float b = cimagf(z);
  return logf(a * a + b * b);
}

// Callback function which accepts the music buffer and frames, frames is the
// number of frames per second, the function iterates over the number of frames
// and performs a ring buffer by placing a single sample in the buffer and
// removing the other from other side of the buffer
void callback(void *bufferData, unsigned int frames) {
  // Frame *fs = bufferData;
  float (*fs)[2] = bufferData;
  for (size_t i = 0; i < frames; ++i) {
    rb_push(&ring, fs[i][0]);
  }
}

void callbackLPF(void *bufferdata, unsigned int frames) {
  float (*fs)[2] = bufferdata;
  const float cutoff = 1000.0f / (plug->music.stream.sampleRate);
  const float k = cutoff / (cutoff + 0.1591549431f);
  for (size_t i = 0; i < frames; ++i) {
    plug->loadL += k * (fs[i][0] - plug->loadL);
    plug->loadR += k * (fs[i][1] - plug->loadR);
    fs[i][0] = plug->loadL;
    fs[i][1] = plug->loadR;
    rb_push(&ring, plug->loadL);
  }
}

void callbackHPF(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;
  const float cutoff = 3330.0f / (plug->music.stream.sampleRate);
  const float k = cutoff / (cutoff + 0.1591549431f);
  for (size_t i = 0; i < frames; ++i) {
    plug->loadL += k * (fs[i][0] - plug->loadL);
    plug->loadR += k * (fs[i][1] - plug->loadR);
    fs[i][0] = fs[i][0] - plug->loadL;
    fs[i][1] = fs[i][1] - plug->loadR;
    rb_push(&ring, fs[i][0]);
  }
}

void callbackPan(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;
  float cutoff = 1000.0f;
  float fsr = plug->music.stream.sampleRate;
  float k = 1.0f - expf(-2.0f * PI * cutoff / fsr);
  float pan = 0.5f + 0.5f * sinf(GetTime() * 2.0f);
  const float gainL = cosf(pan * M_PI_2);
  const float gainR = sinf(pan * M_PI_2);
  for (size_t i = 0; i < frames; ++i) {
    plug->loadL += k * (fs[i][0] - plug->loadL);
    plug->loadR += k * (fs[i][1] - plug->loadR);

    float lowLeft = plug->loadL;
    float lowRight = plug->loadR;

    float highLeft = fs[i][0] - lowLeft;
    float highRight = fs[i][1] - lowRight;

    fs[i][0] = lowLeft + highLeft * gainL;
    fs[i][1] = lowRight + highRight * gainR;

    rb_push(&ring, fs[i][0]);
  }
}

void callbackBassBoost(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;
  float cutoff = 200.0f / plug->music.stream.sampleRate;
  float k = cutoff / (cutoff + 0.1591549431f);
  float gain = 3.0f;
  for (size_t i = 0; i < frames; ++i) {
    plug->loadL += k * (fs[i][0] - plug->loadL);
    plug->loadR += k * (fs[i][1] - plug->loadR);
    fs[i][0] = fs[i][0] + (plug->loadL * (gain - 1.0f));
    fs[i][1] = fs[i][1] + (plug->loadR * (gain - 1.0f));
    rb_push(&ring, fs[i][0]);
  }
}

void callbackTrebleBoost(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;
  float cutoff = 4000.0f / plug->music.stream.sampleRate;
  float k = cutoff / (cutoff + 0.1591549431f);
  float gain = 2.5f;
  for (size_t i = 0; i < frames; i++) {
    plug->loadL += k * (fs[i][0] - plug->loadL);
    plug->loadR += k * (fs[i][1] - plug->loadR);
    float highL = fs[i][0] - plug->loadL;
    float highR = fs[i][0] - plug->loadL;
    fs[i][0] = fs[i][0] + (highL * (gain - 1.0f));
    fs[i][1] = fs[i][1] + (highR * (gain - 1.0f));
    rb_push(&ring, fs[i][0]);
  }
}

void callbackClear(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;
  float fsr = plug->music.stream.sampleRate;
  float cutLow = 1000.0f / fsr;
  float cutHigh = 5000.0f / fsr;
  float kL = cutLow / (cutLow + 0.1591549431f);
  float kH = cutHigh / (cutHigh + 0.1591549431f);
  float gain = 2.5f;
  for (size_t i = 0; i < frames; ++i) {
    plug->lowL += kL * (fs[i][0] - plug->lowL);
    plug->lowR += kL * (fs[i][1] - plug->lowR);
    plug->midL += kH * (fs[i][0] - plug->midL);
    plug->midR += kH * (fs[i][1] - plug->midR);
    float bandL = plug->midL - plug->lowL;
    float bandR = plug->midR - plug->lowR;
    fs[i][0] = fs[i][0] + bandL * (gain - 1.0f);
    fs[i][1] = fs[i][1] + bandR * (gain - 1.0f);
    rb_push(&ring, fs[i][0]);
  }
}

void callbackEQ(void *bufferData, unsigned int frames) {
  float (*fs)[2] = bufferData;
  float fsr = plug->music.stream.sampleRate;

  float kBass = plug->bassCutoff / (plug->bassCutoff + fsr * 0.1591549431f);
  float kTreble =
      plug->trebleCutoff / (plug->trebleCutoff + fsr * 0.1591549431f);

  for (size_t i = 0; i < frames; ++i) {
    float inL = fs[i][0];
    float inR = fs[i][1];

    plug->loadL += kBass * (inL - plug->loadL);
    plug->loadR += kBass * (inR - plug->loadR);
    float bassL = plug->loadL;
    float bassR = plug->loadR;

    plug->midL += kTreble * (inL - plug->midL);
    plug->midR += kTreble * (inR - plug->midR);
    float trebleL = inL - plug->midL;
    float trebleR = inR - plug->midR;

    float midL = inL - bassL - trebleL;
    float midR = inR - bassR - trebleR;

    fs[i][0] = bassL * plug->bassGain + midL * plug->midGain +
               trebleL * plug->trebleGain;
    fs[i][1] = bassR * plug->bassGain + midR * plug->midGain +
               trebleR * plug->trebleGain;
    rb_push(&ring, fs[i][0]);
  }
}
void detachFilter(Music music) {
  if (!IsMusicValid(music))
    return;
  if (currentFilter == CALLBACK)
    DetachAudioStreamProcessor(music.stream, callback);
  else if (currentFilter == CALLBACK_LPF)
    DetachAudioStreamProcessor(music.stream, callbackLPF);
  else if (currentFilter == CALLBACK_HPF)
    DetachAudioStreamProcessor(music.stream, callbackHPF);
  else if (currentFilter == CALLBACK_PAN)
    DetachAudioStreamProcessor(music.stream, callbackPan);
  else if (currentFilter == CALLBACK_BB)
    DetachAudioStreamProcessor(music.stream, callbackBassBoost);
  else if (currentFilter == CALLBACK_TRB)
    DetachAudioStreamProcessor(music.stream, callbackTrebleBoost);
  else if (currentFilter == CALLBACK_CLR)
    DetachAudioStreamProcessor(music.stream, callbackClear);
  else if (currentFilter == CALLBACK_EQ_MODE)
    DetachAudioStreamProcessor(music.stream, callbackEQ);
}

void switchFilter(Music music, filterType nextFilter) {
  if (!IsMusicValid(music))
    return;
  if (currentFilter == nextFilter)
    return;
  detachFilter(plug->music);
  if (nextFilter == CALLBACK)
    AttachAudioStreamProcessor(music.stream, callback);
  else if (nextFilter == CALLBACK_LPF)
    AttachAudioStreamProcessor(music.stream, callbackLPF);
  else if (nextFilter == CALLBACK_HPF)
    AttachAudioStreamProcessor(music.stream, callbackHPF);
  else if (nextFilter == CALLBACK_PAN)
    AttachAudioStreamProcessor(music.stream, callbackPan);
  else if (nextFilter == CALLBACK_BB)
    AttachAudioStreamProcessor(music.stream, callbackBassBoost);
  else if (nextFilter == CALLBACK_TRB)
    AttachAudioStreamProcessor(music.stream, callbackTrebleBoost);
  else if (nextFilter == CALLBACK_CLR)
    AttachAudioStreamProcessor(music.stream, callbackClear);
  else if (nextFilter == CALLBACK_EQ_MODE)
    AttachAudioStreamProcessor(music.stream, callbackEQ);
}
// A fast fourier transform implementation
void fft(float in[], size_t stride, float complex out[], size_t n) {
  assert(n > 0);
  if (n == 1) {
    out[0] = in[0];
    return;
  }

  fft(in, stride * 2, out, n / 2);
  fft(in + stride, stride * 2, out + n / 2, n / 2);

  for (size_t k = 0; k < n / 2; ++k) {
    float t = (float)k / n;
    float complex v = cexp(-2 * PI * I * t) * out[k + n / 2];
    float complex e = out[k];
    out[k] = e + v;
    out[k + n / 2] = e - v;
  }
}

// initialization without any source (drag and drop)
void plug_init(void) {
  plug = malloc(sizeof(*plug));
  assert(plug != NULL && "assert failure at plug_init");
  memset(plug, 0, sizeof(*plug));
}

// initialization using source + (drag and drop)
void plug_src_init(const char *src) {
  if (plug == NULL) {
    plug = malloc(sizeof(*plug));
    assert(plug != NULL && "assert failure at plug_src_init");
    memset(plug, 0, sizeof(*plug));
  }
  if (IsMusicValid(plug->music)) {
    detachFilter(plug->music);
    UnloadMusicStream(plug->music);
  }
  plug->music = LoadMusicStream(src);
  if (IsMusicValid(plug->music)) {
    PlayMusicStream(plug->music);
    printf("%i\n", plug->music.stream.sampleRate);
#ifdef EQ_MODE
    AttachAudioStreamProcessor(plug->music.stream, callbackEQ);
#else
    AttachAudioStreamProcessor(plug->music.stream, callback);
#endif /* ifdef EQ_MODE */
    plug->bassGain = 1.0f;
    plug->trebleGain = 1.0f;
    plug->midGain = 1.0f;
    plug->bassCutoff = 200.0f;
    plug->trebleCutoff = 4000.0f;
    plug->midLow = 800.0f;
    plug->midHigh = 3000.0f;
    plug->volume = 0.5f;
    SetMusicVolume(plug->music, plug->volume);
  }
}

// ending function unloads everything
void plug_unload_stream(void) {
  if (plug != NULL) {
    if (IsMusicValid(plug->music)) {
      StopMusicStream(plug->music);
      detachFilter(plug->music);
      UnloadMusicStream(plug->music);
    }
    free(plug);
    printf("Freeing plug\n");
    plug = NULL;
  }
  CloseAudioDevice();
  CloseWindow();
}

void plug_key_functions(void) {
// Filter (Normal + LPF + HPF + Pan + BassBoost + TrebleBoost + Clear)
#ifndef EQ_MODE
  if (IsKeyPressed(KEY_COMMA)) {
    switchFilter(plug->music, CALLBACK_LPF);
    currentFilter = CALLBACK_LPF;
  }
  if (IsKeyPressed(KEY_PERIOD)) {
    switchFilter(plug->music, CALLBACK);
    currentFilter = CALLBACK;
  }
  if (IsKeyPressed(KEY_SLASH)) {
    switchFilter(plug->music, CALLBACK_HPF);
    currentFilter = CALLBACK_HPF;
  }
  if (IsKeyPressed(KEY_P)) {
    switchFilter(plug->music, CALLBACK_PAN);
    currentFilter = CALLBACK_PAN;
  }
  if (IsKeyPressed(KEY_B)) {
    switchFilter(plug->music, CALLBACK_BB);
    currentFilter = CALLBACK_BB;
  }
  if (IsKeyPressed(KEY_T)) {
    switchFilter(plug->music, CALLBACK_TRB);
    currentFilter = CALLBACK_TRB;
  }
  if (IsKeyPressed(KEY_C)) {
    switchFilter(plug->music, CALLBACK_CLR);
    currentFilter = CALLBACK_CLR;
  }
#else
  if (IsKeyDown(KEY_Z)) {
    plug->bassGain = Clamp(plug->bassGain - 0.01f, 0.0f, 4.0f);
    printf("BASS: %f\n", plug->bassGain);
  }
  if (IsKeyDown(KEY_X)) {
    plug->bassGain = Clamp(plug->bassGain + 0.01f, 0.0f, 4.0f);
    printf("BASS: %f\n", plug->bassGain);
  }
  if (IsKeyDown(KEY_C)) {
    plug->midGain = Clamp(plug->midGain - 0.01f, 0.0f, 4.0f);
    printf("MID-GAIN: %f\n", plug->midGain);
  }
  if (IsKeyDown(KEY_V)) {
    plug->midGain = Clamp(plug->midGain + 0.01f, 0.0f, 4.0f);
    printf("MID-GAIN: %f\n", plug->midGain);
  }
  if (IsKeyDown(KEY_B)) {
    plug->trebleGain = Clamp(plug->trebleGain - 0.01f, 0.0f, 4.0f);
    printf("TREBLE-GAIN: %f\n", plug->trebleGain);
  }
  if (IsKeyDown(KEY_N)) {
    plug->trebleGain = Clamp(plug->trebleGain + 0.01f, 0.0f, 4.0f);
    printf("TREBLE-GAIN: %f\n", plug->trebleGain);
  }
#endif
  // Volume Controls (default = 0.5f);
  if (IsKeyPressed(KEY_UP) && IsMusicValid(plug->music)) {
    if (plug->volume < 1.0f) {
      plug->volume += 0.1f;
      SetMusicVolume(plug->music, plug->volume);
    }
  }
  if (IsKeyPressed(KEY_DOWN) && IsMusicValid(plug->music)) {
    if (plug->volume > 0.1f) {
      plug->volume -= 0.1f;
      SetMusicVolume(plug->music, plug->volume);
    }
  }
  if (IsKeyPressed(KEY_SPACE) && IsMusicValid(plug->music)) {
    if (IsMusicStreamPlaying(plug->music)) {
      PauseMusicStream(plug->music);
    } else {
      ResumeMusicStream(plug->music);
    }
  }
}

// Pre reload and post reload functions for hot reloading
Plug *plug_pre_reload(void) {
  if (IsMusicValid(plug->music))
    detachFilter(plug->music);
  return plug;
}
void plug_post_reload(Plug *state) {
  plug = state;
  if (IsMusicValid(plug->music)) {
#ifndef EQ_MODE
    AttachAudioStreamProcessor(plug->music.stream, callback);
#else
    AttachAudioStreamProcessor(plug->music.stream, callbackEQ);
    plug->bassGain = 1.0f;
    plug->trebleGain = 1.0f;
    plug->midGain = 1.0f;
    plug->bassCutoff = 200.0f;
    plug->trebleCutoff = 4000.0f;
    plug->midLow = 800.0f;
    plug->midHigh = 3000.0f;
    plug->volume = 0.5f;
#endif
  }
}

// main function for updating each frame and draw the visualization
void plug_update(void) {
  if (IsMusicValid(plug->music))
    UpdateMusicStream(plug->music);
  float dt = GetFrameTime();
  // drag and drop
  if (IsFileDropped()) {
    printf("file drop triggered\n");
    FilePathList dfile = LoadDroppedFiles();
    printf("%i\n%i\n", dfile.capacity, dfile.count);
    const char *dropped_path = dfile.paths[0];
    if (IsMusicValid(plug->music)) {
      StopMusicStream(plug->music);
      detachFilter(plug->music);
      UnloadMusicStream(plug->music);
      plug->music = (Music){0};
    }
    plug->music = LoadMusicStream(dropped_path);
    if (IsMusicValid(plug->music)) {
      plug->error = false;
      PlayMusicStream(plug->music);
      currentFilter = CALLBACK;
      AttachAudioStreamProcessor(plug->music.stream, callback);
      SetMusicVolume(plug->music, plug->volume);
    } else
      plug->error = true;
    UnloadDroppedFiles(dfile);
  }
  plug_key_functions();
  // Play pause

  float w = (float)GetRenderWidth();
  float h = (float)GetRenderHeight();
  BeginDrawing();
  ClearBackground((Color){0, 0, 0, 0});
  if (plug->music.ctxData != NULL) {
    // https://en.wikipedia.org/wiki/Hann_function
    rb_read(&ring, in);
    for (size_t i = 0; i < N; i++) {
      float t = (float)i / (N - 1);
      float hanning = 0.5 - 0.5 * cosf(2 * PI * t);
      in1[i] = in[i] * hanning;
    }
    fft(in1, 1, out, N);

    float max_amp = 1.0f;
    float step = 1.06f;
    size_t m = 0;
    float lowf = 1.0f;
    float smoothness = 10.0f;

    for (float f = lowf; (size_t)f <= N / 2; f = ceilf(f * step)) {
      float f1 = ceilf(f * step);
      float a = 0.0f;
      for (size_t q = (size_t)f; q <= N / 2 && q < (size_t)f1; ++q) {
        float b = amp(out[q]);
        if (b > a)
          a = b;
      }
      if (max_amp < a)
        max_amp = a;
      out_log[m++] = a;
    }
    for (size_t i = 0; i < m; i++) {
      out_log[i] /= max_amp;
    }

    for (size_t i = 0; i < m; i++) {
      out_smooth[i] += smoothness * (out_log[i] - out_smooth[i]) * dt;
    }
#ifdef SINE_WAVE
    int POINTS = 150;
    float waveStep = (float)(w / POINTS);
    Vector2 startPoint = {0, h / 2};
    for (size_t i = 0; i < (size_t)POINTS; i++) {
      float x = i * waveStep;
      float binIndex = (float)i / POINTS * m;
      int index = (int)binIndex;
      float fraction = binIndex - index;
      float amplitude = 0;
      if (index < m - 1) {
        amplitude = out_smooth[index] * (1.0f - fraction) +
                    out_smooth[index + 1] * fraction;
      } else {
        amplitude = out_smooth[index];
      }
      amplitude = amplitude * plug->volume;
      float sineWave = sinf(GetTime() * 10.0f + i * 0.5f);
      float y = (h / 2.0f) - (IsAudioStreamPlaying(plug->music.stream) *
                              amplitude * (h / 3.0f) * sineWave);
      Vector2 currentPoint = {x, y};

      if (i > 0) {
        float hue = 240.0f - (amplitude * 240.0f);
        DrawLineEx(startPoint, currentPoint, 3.0f,
                   ColorFromHSV(hue, 1.0f, 1.0f));
      }

      startPoint = currentPoint;
    }
#endif
#ifdef MULTI_WAVE
    int POINTS = 55;
    float waveStep = (float)(w / POINTS);
    float jumpStep = 1.07f;
    // printf("%zu\n",m);
    for (size_t wave = 1; wave <= m; wave = ceilf(jumpStep * wave)) {
      float amplitude = out_smooth[wave] * plug->volume;
      if (amplitude < 0.05f)
        continue;
      float hue = 240.0f - (240.0f * ((float)wave / m));
      Color color = ColorFromHSV(hue, 1.0f, 1.0f);
      // color.a = 90;
      Vector2 startPoint = {0, h / 2};
      for (int i = 0; i < POINTS; i++) {
        float x = i * waveStep;
        float visualFrequency = 0.05f + ((float)wave / m) * 0.5f;
        float timeOffset = GetTime() * 5.0f;
        float phase = wave * 0.12f;
        float sineWave = sinf(timeOffset + (i * visualFrequency) + phase);
        float y = (h / 2.0f) - (IsAudioStreamPlaying(plug->music.stream) *
                                amplitude * (h / 2.0f) * sineWave);
        Vector2 currentPoint = {x, y};
        if (i > 0)
          DrawLineEx(startPoint, currentPoint, 1.0f, color);
        startPoint = currentPoint;
      }
    }
#endif
#ifdef LINE
    float cell_width = (float)w / m;
    for (size_t i = 0; i < m; i++) {
      float t = out_smooth[i];
      float hue = 240.0f - (240.0f * t);
      if (hue < 0.0f)
        hue = 0.0f;
      Color color = ColorFromHSV(hue, 1.0f, 1.0f);
      Vector2 startPos = {cell_width * (0.5 + i),
                          h - (h * t * sqrtf(plug->volume))};
      Vector2 endPos = {cell_width * (0.5 + i), h};
      DrawLineEx(startPos, endPos, cell_width / 2, color);
      DrawCircleV(startPos, cell_width * (t), color);
      DrawCircleLinesV(startPos, cell_width * (t) * 2, color);
    }
#endif
  } else {
    if (plug->error)
      DrawText("Eroor :(", 0, 0, 70, RED);
    else
      DrawText("Drop music", 1, 1, 70, GOLD);
  }

  EndDrawing();
}
