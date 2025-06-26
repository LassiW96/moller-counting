#ifndef MOLLERTRIGGERSCINTILLATOR_H
#define MOLLERTRIGGERSCINTILLATOR_H

#include "THaNonTrackingDetector.h"
#include "FADCData.h"
#include <vector>
#include <set>

class FADCData;
class TCloanesArray;
class MOLLERTriggerScintillator : public THaNonTrackingDetector {
public:
    explicit MOLLERTriggerScintillator(const char* name, const char* description = "",
                                THaApparatus* a = nullptr );
    MOLLERTriggerScintillator();
    virtual ~MOLLERTriggerScintillator();

    class HitData_t {
    public :
        HitData_t() :
            pad(-1), time(kBig), dtime(kBig), yt(kBig), ya(kBig), ampl(kBig) {}
        HitData_t(Int_t pad, Data_t time, Data_t dtime, Data_t yt, Data_t ya, Data_t ampl) :
            pad(pad), time(time), dtime(dtime), yt(yt), ya(ya), ampl(ampl) {}

        void clear() {time = dtime = yt = ampl = ya = kBig;}
        bool operator<(const HitData_t& rhs) const {return time < rhs.time;}

        Int_t pad; // Paddle number
        Data_t time; // Average of corrected time (s)
        Data_t dtime; // Uncertainty of average time (s)
        Data_t yt; // Transverse track position from time difference (m)
        Data_t ya; // Transverse track position from amplitude ratio (m)
        Data_t ampl; // Estimate energy deposition dE/dx (a.u.)
    };

    virtual void    Clear(Option_t* opt="");
    virtual Int_t   CoarseProcess( TClonesArray& tracks );
    virtual Int_t   FineProcess( TClonesArray& tracks );

protected:
    virtual Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
    OptUInt_t LoadData( const THaEvData& evdata,
                    const DigitizerHitInfo_t& hitinfo ) override;

    virtual void    PrintDecodedData(const THaEvData& evdata) const;
    virtual Int_t   ApplyCorrections();
    //virtual Data_t TimeWalkCorrection( Idx_t idx, Data_t adc );
    virtual Int_t   FindPaddleHits();
    virtual Int_t   ReadDatabase( const TDatime& date );
    virtual Int_t   DefineVariables( EMode mode = kDefine );

    FADCData* fFADCData; // Moller uses only one PMT per scintillator
    ClassDef(MOLLERTriggerScintillator, 1)
};
#endif
