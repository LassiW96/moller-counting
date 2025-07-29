/////////////////////////////////////////////////////////////////////////////////////////
// Header file for the test scintillator
//
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef Podd_MOLLERTestScint_h_
#define Podd_MOLLERTestScint_h_

#include "THaNonTrackingDetector.h"
#include <vector>

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
    virtual Int_t StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data);
    virtual Int_t CoarseProcess(TClonesArray& tracks);
    virtual Int_t FineProcess(TClonesArray& tracks);
    virtual void   Print( Option_t* opt="" ) const;

    Int_t GetNhits() const { return static_cast<Int_t>(fEventData.size()); }

protected:
    virtual Int_t ReadDatabase(const TDatime& date); // Read config parameters from the database
    virtual Int_t DefineVariables(EMode mode); // Define global analysis vars

    //---- Data stored with this detector follow here ----
    typedef std::vector<Data_t> DataVec_t;
    
    // Calibration data from database
    DataVec_t fPed;       // ADC pedestals
    DataVec_t fGain;      // ADC gains

    // Per-event data
    // Define a structure to hold the information of one hit
    class EventData {
    public:
    Int_t   fChannel;   // Logical channel number
    Data_t  fRawADC;    // Raw ADC data
    Data_t  fCalADC;    // Pedestal-subtracted and gain-calibrated ADC data
    // Define a constructor so we can fill all fields in one line
    EventData(Int_t chan, Data_t raw, Data_t cal)
    : fChannel(chan), fRawADC(raw), fCalADC(cal) {}
    };

    // Vector with the hit information for the current event
    std::vector<EventData> fEventData;

    ClassDef(MOLLERTestScint, 0)

};

#endif
