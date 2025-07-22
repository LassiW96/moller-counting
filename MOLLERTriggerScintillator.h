#ifndef MOLLERTriggerScintillator_h
#define MOLLERTriggerScintillator_h

#include "THaNonTrackingDetector.h"
#include "DetectorData.h"
#include "FADCData.h"
#include <vector>
#include <set>

namespace HallA {
    
class FADCData;
class TCloanesArray;

// This structure has output data when the user wants every hit to be stored
// in the rootfile.
struct MOLLERTrigScintOutputdata {
    //std::vector<Int_t> ped;         //< [] pedestal
    //std::vector<Int_t> a_mult;         //< [] ADC # of hits per channel
    std::vector<Double_t> a;         //< [] ADC integral
    std::vector<Double_t> a_p;         //< [] ADC integral -pedestal
    std::vector<Double_t> a_c;         //< [] (ADC integral -pedestal)*calib
    std::vector<Double_t> a_amp;     //< [] ADC pulse amplitude
    std::vector<Double_t> a_amp_p;     //< [] ADC pulse amplitude -pedestal
    std::vector<Double_t> a_amp_c;     //< [] ADC pulse amplitude -pedestal
    //std::vector<Double_t> a_amptrig_p;     //< [] ADC pulse amplitude -pedestal
    //std::vector<Double_t> a_amptrig_c;     //< [] ADC pulse amplitude -pedestal
    //std::vector<Double_t> a_time;    //< [] ADC pulse time

    void clear() {
        a.clear();
        a_p.clear();
        a_c.clear();
        a_amp.clear();
        a_amp_p.clear();
        a_amp_c.clear();
    }
  
};
class MOLLERTriggerScintillator : public THaNonTrackingDetector {
public:
    enum Eside {kNone = -1, kRight = 0, kLeft = 1};
    using Idx_t = std::pair<Eside, Int_t>;

    //enum class PMTMode {Single, Double};                         // Flag to determine if single PMT or Dual PMT case
    //PMTMode fPMTMode;                                            // PMTMode should be defined in either a db file or somewhere else

    explicit MOLLERTriggerScintillator(const char* name, const char* description = "",
                                THaApparatus* a = nullptr);
    MOLLERTriggerScintillator();
    virtual ~MOLLERTriggerScintillator();

    virtual void        Clear(Option_t* opt="");
    virtual Int_t       CoarseProcess(TClonesArray& tracks);
    virtual Int_t       FineProcess(TClonesArray& tracks);

    //Bool_t WithADC() { return fModeADC != MOLLERModeADC::kNone; };

protected:
    //bool              CheckHitInfo( const DigitizerHitInfo_t& hitinfo ) const;

    Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
    OptUInt_t LoadData( const THaEvData& evdata,
        const DigitizerHitInfo_t& hitinfo ) override;
    
    virtual void        PrintDecodedData(const THaEvData& evdata) const;
    virtual Int_t       ReadDatabase(const TDatime& date);
    virtual Int_t       DefineVariables( EMode mode = kDefine ) override;

    // Calibration parameters
    Data_t      fCn;                // Speed of light in the material (m/s)
    Data_t      fAttenuation;       // Attenuation length of the material (1/m)
    Data_t      fResolution;        // Average time resolution per PMT (s)

    // per-event data
    Int_t      fNhits;     ///< Number of hits in event
    Int_t      fNRefhits;     ///< Number of reference hits in event
    Int_t      fNGoodTDChits;     ///< Number of good TDC hits in event
    Int_t      fNGoodADChits;     ///< Number of good ADC hits in event

    // PMTData - how to change into FADCdata
    FADCData*               fPMTs;      // An array for the number of PMTs - from fadc data (how to declair an array of unknown length)
    std::set<Idx_t>         fHitIdx;    // Idices of PMTs with data

    MOLLERTrigScintOutputdata fGood;    // Good data output

    //MOLLERModeADC::Mode fModeADC;      //< ADC Mode


    ClassDef(MOLLERTriggerScintillator, 1)
};

}

#endif
