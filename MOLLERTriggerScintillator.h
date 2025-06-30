#ifndef MOLLERTRIGGERSCINTILLATOR_H
#define MOLLERTRIGGERSCINTILLATOR_H

#include "THaNonTrackingDetector.h"
#include "DetectorData.h"
#include "FADCData.h"
#include <vector>
#include <set>

class FADCData;
class TCloanesArray;
class MOLLERTriggerScintillator : public THaNonTrackingDetector {
public:
    enum Eside {kNone = -1, kRight = 0, kLeft = 1, kSingle = 2}; // Added kSingle in case if single PMT is used
    using Idx_t = std::pair<Eside, Int_t>;

    //enum class PMTMode {Single, Double};                         // Flag to determine if single PMT or Dual PMT case
    //PMTMode fPMTMode;                                            // PMTMode should be defined in either a db file or somewhere else

    explicit MOLLERTriggerScintillator(const char* name, const char* description = "",
                                THaApparatus* a = nullptr);
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

        Int_t   pad;        // Paddle number
        Data_t  time;       // Average of corrected time (s)
        Data_t  dtime;      // Uncertainty of average time (s)
        Data_t  yt;         // Transverse track position from time difference (m)
        Data_t  ya;         // Transverse track position from amplitude ratio (m)
        Data_t  ampl;       // Estimate energy deposition dE/dx (a.u.)
    };

    virtual void        Clear(Option_t* opt="");
    virtual Int_t       Decode(const THaEvData&);
    virtual Int_t       CoarseProcess(TClonesArray& tracks);
    virtual Int_t       FineProcess(TClonesArray& tracks);

    Int_t               GetNHits() const {return static_cast<Int_t>(fHits.size());}
    const HitData_t&    GetHit(Int_t i) {return fHits[i];}
    const HitData_t&    GetPad(Int_t i) {return fPadData[i];}

protected:

    // Calibration parameters
    Data_t      fCn;                // Speed of light in the material (m/s)
    Data_t      fAttenuation;       // Attenuation length of the material (1/m)
    Data_t      fResolution;        // Average time resolution per PMT (s)

    // per-event data
    Podd::PMTData*          fPMTs;      // Raw PMT data - in case of a single PMT
    Podd::PMTData*          fRightPMTs; // Raw PMT data for right side PMTs - 2 PMT case
    Podd::PMTData*          fLeftPMTs;  // Raw PMT data for left side PMTs
    std::set<Idx_t>         fHitIdx;    // Idices of PMTs with data
    std::vector<HitData_t>  fHits;      // Calculated hit data per hit
    // fPadData duplicates the info in fHits for direct access via paddle number
    std::vector<HitData_t>  fPadData;   // Calculated hit data per paddle

    virtual Int_t       StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data) override;
    OptUInt_t           LoadData(const THaEvData& evdata,
                                const DigitizerHitInfo_t& hitinfo) override;

    virtual void        PrintDecodedData(const THaEvData& evdata) const;
    virtual Int_t       ApplyCorrections();
    virtual Data_t      TimeWalkCorrection(Idx_t idx, Data_t adc);
    virtual Int_t       FindPaddleHits();
    virtual Int_t       ReadDatabase(const TDatime& date);
    virtual Int_t       DefineVariables( EMode mode = kDefine );

    FADCData* fFADCData; // Is this neccessary?
    ClassDef(MOLLERTriggerScintillator, 1)
};
#endif
