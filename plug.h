#ifndef PLUG_H_
#define PLUG_H_

#include <stddef.h>
#include <complex.h>
#include <raylib.h>

#define LIST_OF_PLUGS \
  PLUG(plug_init,void,const char *file_path) \
  PLUG(plug_update,void,void) \
  PLUG(plug_pre_reload,void*,void) \
  PLUG(plug_post_reload,void,void*) \
  PLUG(plug_unload_stream,void,void)

#define PLUG(name,ret,...) typedef ret (name##_t)(__VA_ARGS__);
LIST_OF_PLUGS
#undef PLUG

#endif
