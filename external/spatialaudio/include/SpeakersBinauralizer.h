#ifndef BINAURALIZER_H
#define BINAURALIZER_H

#include <vector>

#include "AmbisonicCommons.h"
#include "BFormat.h"
#include "AmbisonicSpeaker.h"
#include "AmbisonicBinauralizer.h"
#include "sa_api.h"

/** Binaural decoder. */

class SA_API SpeakersBinauralizer : public CAmbisonicBinauralizer
{
public:
    SpeakersBinauralizer();
    bool Configure(unsigned nSampleRate,
                   unsigned nBlockSize,
                   CAmbisonicSpeaker *speakers,
                   unsigned nSpeakers,
                   unsigned& tailLength,
                   std::string HRTFPath = "");

    void Process(float** pBFSrc, float** ppfDst);

protected:
    unsigned m_nSpeakers;

    virtual void AllocateBuffers();
};

#endif // BINAURALIZER_H
