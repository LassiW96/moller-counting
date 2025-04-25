#ifndef MOLLERTRIGGERSCINTILLATOR_H
#define MOLLERTRIGGERSCINTILLATOR_h

#include "THaScintillator.h"

namespace MOLLERData {
    class MOLLERTriggerScintillator : public THaScintillator {
    public:
        MOLLERTriggerScintillator(const char* name, const char* description = "",
                                    THaApparatus* a = nullptr );
        virtual ~MOLLERTriggerScintillator();

        virtual Int_t ReadDatabase( const TDatime& date ) override;
        virtual Int_t DefineVariables( EMode mode = kDefine ) override;

    protected:
        classDef(MOLLERTriggerScintillator, 0)
    }
}
#endif