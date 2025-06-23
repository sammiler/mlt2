//
// Created by sammiller on 2025/6/22.
//

#ifndef SA_API_H
#define SA_API_H

#if defined(_WIN32) && defined(_MSC_VER)
  #ifdef SA_EXPORTS
    #define SA_API __declspec(dllexport)
  #else
    #define SA_API __declspec(dllimport)
  #endif
#else
  #define SA_API
#endif

#endif //SA_API_H
