#ifndef MOLLERTriggerScintillator_h
#define MOLLERTriggerScintillator_h

#include "THaNonTrackingDetector.h"
#include "DetectorData.h"
#include "FADCData.h"
#include <vector>
#include <set>

class FADCData;
class TCloanesArray;
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
    virtual Int_t       Decode(const THaEvData&);
    virtual Int_t       CoarseProcess(TClonesArray& tracks);
    virtual Int_t       FineProcess(TClonesArray& tracks);

protected:

    // Calibration parameters
    Data_t      fCn;                // Speed of light in the material (m/s)
    Data_t      fAttenuation;       // Attenuation length of the material (1/m)
    Data_t      fResolution;        // Average time resolution per PMT (s)

    // per-event data
    
    FADCData*               fPMTs;      // An array for the number of PMTs - from fadc data
    std::set<Idx_t>         fHitIdx;    // Idices of PMTs with data
    
    virtual void        PrintDecodedData(const THaEvData& evdata) const;
    virtual Int_t       ApplyCorrections();
    virtual Int_t       FindPaddleHits();
    virtual Int_t       ReadDatabase(const TDatime& date);
    virtual Int_t       DefineVariables( EMode mode = kDefine );

    ClassDef(MOLLERTriggerScintillator, 0)
};
#endif
