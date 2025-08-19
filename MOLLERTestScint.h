/////////////////////////////////////////////////////////////////////////////////////////
// Header file for the test scintillator
//
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef Podd_MOLLERTestScint_h_
#define Podd_MOLLERTestScint_h_

#include "THaNonTrackingDetector.h"
#include "FADCData.h"
#include "Fadc250Module.h"
#include <vector>

using namespace Decoder;

class FADCData;

class MOLLERTestScint: public THaNonTrackingDetector {

public:
    // Constructor (for the creation of detectors in the analysis script)
    explicit MOLLERTestScint(const char* name, const char* description = "",
                            THaApparatus* a = nullptr);

    // Default constructor
    MOLLERTestScint() = default;

    // Destructor (to clean up global vars, if there are any)
    virtual ~MOLLERTestScint();

    // Public base functions
    virtual void Clear(Option_t* opt="");
    virtual Int_t     Decode( const THaEvData& );
    //virtual Int_t StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data);
    virtual Int_t CoarseProcess(TClonesArray& tracks);
    virtual Int_t FineProcess(TClonesArray& tracks);
    //virtual void   Print( Option_t* opt="" ) const;

    Int_t GetNhits() const { return static_cast<Int_t>(fEventData.size()); }

protected:
    //virtual OptUInt_t LoadData( const THaEvData& evdata, const DigitizerHitInfo_t& hitinfo ) override;
    virtual Int_t ReadDatabase(const TDatime& date); // Read config parameters from the database
    virtual Int_t DefineVariables(EMode mode); // Define global analysis vars

    // FADC decoding 
    // std::vector<Decoder::Fadc250Module*> fFadcModules; // vector for more than 1 FADC modules
    Fadc250Module* fFadc;
    TString fFadcName;
    HallA::FADCData*               fPMT;      // An array for the number of PMTs - from fadc data (how to declair an array of unknown length)
    //---- Data stored with this detector follow here ----
    typedef std::vector<Data_t> DataVec_t;
    
    // Calibration data from database
    DataVec_t fPed;       // ADC pedestals
    DataVec_t fGain;      // ADC gains
    DataVec_t fTime;        // FADC time
    DataVec_t fIntegral;    // FADC integral

    // Per-event data
    // Define a structure to hold the information of one hit
    class EventData {
    public:
    Int_t   fChannel;   // Logical channel number
    Data_t  fRawADC;    // Raw ADC data
    Data_t  fCalADC;    // Pedestal-subtracted and gain-calibrated ADC data
    Double_t fTime;      // Time from FADCs
    Double_t fIntegral;     // FADC integral
    // Define a constructor so we can fill all fields in one line
    EventData(Int_t chan, Data_t raw, Data_t cal, Double_t t, Double_t i)
    : fChannel(chan), fRawADC(raw), fCalADC(cal), fTime(t), fIntegral(i) {}
    };

    // Vector with the hit information for the current event
    std::vector<EventData> fEventData;

private:
    // Stores only physics channels: indexed by logical detchan -> {crate, slot, chan}
    //std::vector<THaDetMap::ChanDef> fChanMap;

    // Stores only reference channels
    //std::vector<THaDetMap::ChanDef> fRefChanMap;

    ClassDef(MOLLERTestScint, 0)

};

#endif
