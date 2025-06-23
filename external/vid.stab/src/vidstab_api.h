// 文件: external/vid.stab/src/vidstab_api.h

#ifndef VIDSTAB_API_H
#define VIDSTAB_API_H

#if defined(_WIN32) && defined(_MSC_VER)
  #ifdef VIDSTAB_EXPORTS // 这个宏将在编译 vidstab.dll 时被定义
    #define VS_API __declspec(dllexport)
  #else // 编译 mltvidstab.dll (或其他客户端) 时
    #define VS_API __declspec(dllimport)
  #endif
#else // 对于非 MSVC 编译器 (GCC, Clang)
  #define VS_API
#endif

#endif // VIDSTAB_API_H