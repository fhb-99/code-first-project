#ifndef FFMPEG_UTIL_H
#define FFMPEG_UTIL_H

// 解决 VS 编译 FFmpeg 报错的关键代码（必须放在最前面！）
#ifdef _MSC_VER
#define __STDC_CONSTANT_MACROS
#endif

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include <libavutil/imgutils.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include <libswresample/swresample.h>
#include <libavutil/time.h>
}

#endif // FFMPEG_UTIL_H
