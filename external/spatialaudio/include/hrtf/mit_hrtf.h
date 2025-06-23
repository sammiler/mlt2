#ifndef MIT_HRTF_H
#define MIT_HRTF_H

#include "hrtf.h"
#include "sa_api.h"
#ifdef HAVE_MIT_HRTF

class SA_API MIT_HRTF : public HRTF
{
public:
    MIT_HRTF(unsigned i_sampleRate);
    bool get(float f_azimuth, float f_elevation, float **pfHRTF);
};

#endif

#endif // MIT_HRTF_H
