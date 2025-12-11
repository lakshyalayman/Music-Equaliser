#include <assert.h>
#include <complex.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

float pi;

void dft(float in[],float complex out[],size_t n){
  for(size_t f = 0;f<n;++f){
    out[f] = 0;
    for(size_t i = 0;i<n;++i){
      float t = (float)i/n;
      out[f] += in[i]*cexp(I*2*pi*f*t);
    }
  }
  for(size_t f = 0;f<n;++f){
    printf("%02zu: real(%.2f) complex(%.2fi)\n",f,creal(out[f]),cimag(out[f]));
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
    float complex v = cexp(-2*pi*I*t)*out[k+n/2];
    float complex e = out[k];
    out[k] = e+v;
    out[k+n/2] = e-v;
  }
}

int main(){
  printf("Hello, World\n");

  size_t n = 8;
  float in[n];
  pi = atan2f(1,1)*4;
  float complex out[n],outf[n];
  for(size_t i = 0;i<n;++i){
    float t = (float)i/n;
    in[i] = cosf(2*pi*t*3) + sinf(2*pi*t*2) + cosf(2*pi*t*1);
  }
  // for(size_t f = 0;f<n/2;++f){
  //   out[f] = 0;
  //   out[f + n/2] = 0;
  //
  //   for(size_t i = 0;i<n;i+=2){
  //     float t = (float)i/n;
  //     float complex cmpx = in[i]*cexp(2*I*pi*t*f);
  //     out[f] += cmpx;
  //     out[f+n/2] += cmpx;
  //   }
  //   for(size_t i = 1;i<n;i+=2){
  //     float t = (float)i/n;
  //     float complex cmpx = in[i]*cexp(2*I*pi*t*f);
  //     out[f] += cmpx;
  //     out[f+n/2] -= cmpx;
  //   }
  // }
  printf("DFT\n\n");
  dft(in,out,n);
  printf("FFT\n\n");
  fft(in,1,outf,n);

  for(size_t f = 0;f<n;++f){
    printf("%02zu: real(%.2f) complex(%.2fi)\n",f,creal(outf[f]),cimag(outf[f]));
  }
  // printf("\n %.2f | %.2f\n",a,b);
  return 0;
}
