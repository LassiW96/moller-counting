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
    MOLLERTestScint();

    // Destructor (to clean up global vars, if there are any)
    virtual ~MOLLERTestScint();

protected:
    virtual Int_t ReadDatabase(const TDatime& date); // Read config parameters from the database

    //---- Data stored with this detector follow here ----
    typedef std::vector<Data_t> DataVec_t;
    
    // Calibration data from database
    DataVec_t fPed;       // ADC pedestals
    DataVec_t fGain;      // ADC gains

};

#endif
