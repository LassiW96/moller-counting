#ifndef MOLLERTRIGGERSCINTILLATOR_H
#define MOLLERTRIGGERSCINTILLATOR_h

#include "THaNonTrackingDetector.h"
#include "FADCData.h"

namespace MOLLERData {
    class MOLLERTriggerScintillator : public THaNonTrackingDetector {
    public:
        MOLLERTriggerScintillator(const char* name, const char* description = "",
                                    THaApparatus* a = nullptr );
        virtual ~MOLLERTriggerScintillator();

        virtual Int_t   ReadDatabase( const TDatime& date ) override;
        virtual Int_t   DefineVariables( EMode mode = kDefine ) override;
        virtual Int_t   CoarseProcess( TClonesArray& tracks );

    protected:
        Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
        OptUInt_t LoadData( const THaEvData& evdata,
                        const DigitizerHitInfo_t& hitinfo ) override;

        FADCData* fFADCData; // Moller uses only one PMT per scintillator
        classDef(MOLLERTriggerScintillator, 0);
    };
};
#endif
