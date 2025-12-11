#include <complex.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

float pi;

int main(){
  printf("Hello, World\n");
  // float complex io = I;
  // printf("%fi\n",cimag(io));
  // printf("%f\n",pi);


  size_t n = 8;
  float in[n];
  pi = atan2f(1,1)*4;
  float complex out[n];
  for(size_t i = 0;i<n;++i){
    float t = (float)i/n;
    in[i] = sinf(2*pi*t) + cosf(2*pi*t*3);
  }
  
  for(size_t f = 0;f<n/2;++f){
    out[f] = 0;
    out[f + n/2] = 0;

    for(size_t i = 0;i<n;i+=2){
      float t = (float)i/n;
      float complex cmpx = in[i]*cexp(2*I*pi*t*f);
      out[f] += cmpx;
      out[f+n/2] += cmpx;
    }
    for(size_t i = 1;i<n;i+=2){
      float t = (float)i/n;
      float complex cmpx = in[i]*cexp(2*I*pi*t*f);
      out[f] += cmpx;
      out[f+n/2] -= cmpx;
    }
  }

  // for(size_t f = 0;f<n;++f){
  //   out[f] = 0;
  //   for(size_t i = 0;i<n;++i){
  //     float t = (float)i/n;
  //     out[f] += in[i]*cexp(I*2*pi*f*t);
  //     // out[f] += in[i]*sinf(2*pi*f*t);
  //   }
  //   
  // }
  for(size_t f = 0;f<n;++f){
    printf("%02zu: real(%.2f) complex(%.2fi)\n",f,creal(out[f]),cimag(out[f]));
  }
  return 0;
}
