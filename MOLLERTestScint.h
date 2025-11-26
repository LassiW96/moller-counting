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
    virtual Int_t CoarseProcess(TClonesArray& tracks);
    virtual Int_t FineProcess(TClonesArray& tracks);
    //virtual void   Print( Option_t* opt="" ) const;

protected:
    virtual Int_t Decode( const THaEvData& evdata ) override;
    virtual OptUInt_t LoadData( const THaEvData& evdata, const DigitizerHitInfo_t& hitinfo ) override;
    virtual Int_t StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
    virtual Int_t ReadDatabase(const TDatime& date); // Read config parameters from the database
    virtual Int_t DefineVariables(EMode mode); // Define global analysis vars

    HallA::FADCData*               fPMT;      //  A pointer to a FADCData object (how to declair an array of unknown length)

    ClassDef(MOLLERTestScint, 0)

};

#endif
