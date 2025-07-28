//////////////////////////////////////////////////////////////////////////////////////
// Trying a test scintillator class according to JLab Hall A SDK
// Following UserDetector
//
//////////////////////////////////////////////////////////////////////////////////////

#include "MOLLERTestScint.h"
#include "VarDef.h"
#include "THaDetMap.h"
#include "TMath.h"
#include "Helper.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace Podd;

// Hard coded maximum number of channels
static const int MAXCHAN = 100;

// Initial basic constructor
//////////////////////////////////////////////////////////////////////////////////////
MOLLERTestScint::MOLLERTestScint(const char* name, const char* description,
                                THaApparatus* apparatus) :
                                THaNonTrackingDetector(name, description, apparatus)
{
    // Constructor
}

// Destructor
///////////////////////////////////////////////////////////////////////////////////////
MOLLERTestScint::~MOLLERTestScint()
{
    // Remove vars from the global list
    RemoveVariables();
}

// Read the DB for this detector
///////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::ReadDatabase(const TDatime& date)
{
    // See analyzer/SDK for more information about opening the database file
    const char* const here = "RaedeDatabase";

    FILE* file = OpenFile(date);
    if (!file) return kFileError;

    // Clear all variables and vectors
    Int_t nelem = 0;
    Double_t angle = 0.0;
    fPed.clear();
    fGain.clear();

    // Detector map
    vector<Int_t> detmap;

    // Read the DB. Using a try block to handle exeptions
    Int_t err = 0;
    try {
        // Ignoring the automatic data type determination for now
        const DBRequest request[] = {
            // Required items
            {"detmap",      &detmap,        kIntV},
            {"nelem",       &nelem,         kInt,       0,      false,      -1}, // Number of element. eg. PMTs
            {"angle",       &angle,         kDouble,    0,  true}, // Rotational angle (this is optional)
            {nullptr} // Last element should be a null pointer
        };

        // Read the requested values
        err = LoadDB(file, date, request);

        // If no error, parse the detmap. See THaDetMap for more details about flags
        if (err ==kOK) {
            if (FillDetMap(detmap, THaDetMap::kFillLogicalChannel, here) <= 0) {
                err = kInitError;
            }
        }
    }

    // Catch the exeptions and close the file
    catch(...) {
        fclose(file);
        throw;
    }

    // Normal end of reading the DB
    fclose(file);
    if (err != kOK) return err;

    // Sanity checks
    // This has to modify according to MOLLER trigger scintillator 
    // Using the following parameters as an example
    if (nelem <= 0) {
        Error (Here(here), "Cannot have a zero or negetive number of elements. "
                "Fix the DB.");
        return kInitError;
    }
    if (nelem > MAXCHAN) {
        Error ( Here(here), "Illegal number of elements = %d. Must be <= %d. "
	            "Fix database.", nelem, MAXCHAN );
        return kInitError;
    }

    // Prevent the global variables dynamic allocation
    if (fIsInit && nelem !=fNelem) {
        ostringstream ostr;
        ostr << "Cannot re-initialize with different number of elements. "
            << "(was: " << fNelem << ", now: " << nelem << "). "
            << "Detector not re-initialized.";
        Error( Here(here), "%s", ostr.str().c_str() );
        return kInitError;
    }

    // Store nelem
    fNelem = nelem;

    // Check the detector map size
    // Assuming each detector element uses only one hardware channel, for now
    // Have to change this appropreately
    UInt_t nchan = fDetMap->GetTotNumChan(), nval = nelem;
    if (nchan != nval) {
        ostringstream ostr;
        ostr << "Incorrect number of detector map channels = " << nchan
            << ". Must equal nelem = " << nval << ". Fix database.";
        Error( Here(here), "%s", ostr.str().c_str() );
        return kInitError;
    }

    // Sanity checks for pedestals and gains
    // Add these if necessary

    // If any of the pedestal or gain values are not given in the DB, set them to default values
    if (fPed.empty()) fPed.assign(nelem, 0.0);
    if (fGain.empty()) fGain.assign(nelem, 1.0);

    // Use the Rotation angle to set the axes vectors
    const Double_t degrad = TMath::Pi()/180.0;
    DefineAxes(angle*degrad);

    // Finish up
    fIsInit = true;
    return kOK;
}
