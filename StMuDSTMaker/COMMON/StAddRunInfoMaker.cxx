/***************************************************************************
 *
 * $Id: StAddRunInfoMaker.cxx,v 1.1 2003/03/06 01:34:18 laue Exp $
 * Author: Frank Laue, BNL, laue@bnl.gov
 ***************************************************************************/
#include "StAddRunInfoMaker.h"

#include "StEvent/StEvent.h"
#include "StEvent/StRunInfo.h"
#include "StEvent/StEventInfo.h"
#include "StEvent/StEventSummary.h"
#include "StarClassLibrary/SystemOfUnits.h"

StAddRunInfoMaker::StAddRunInfoMaker(const char* name) : StMaker(name) {
  /* no-op */
}

StAddRunInfoMaker::~StAddRunInfoMaker() { 
  /* no=op */
}
    
int StAddRunInfoMaker::Init(){
  return 0;
}
 
void StAddRunInfoMaker::Clear() {
  /* no-op */
}


int StAddRunInfoMaker::Make(){  ///< create a StEvent from the muDst and put it into the .data tree 
  StEvent* event = (StEvent*) GetInputDS("StEvent");
  StEventInfo* eventInfo = event->info();
   if ( event ) { 
    // ok, we got the event
    if ( !event->runInfo() ) { // ok, we got no run info, so create, fill and add it
      StRunInfo *pRunInfo = new StRunInfo();
      pRunInfo->setRunId( eventInfo->runId() );
      pRunInfo->setProductionTime(20011007);
      pRunInfo->setProductionVersion("P01hi");
      pRunInfo->setCenterOfMassEnergy(130. * GeV);
      pRunInfo->setBeamMassNumber(east, 197);
      pRunInfo->setBeamMassNumber(west, 197);
      pRunInfo->setBeamEnergy(east, 65. * GeV);
      pRunInfo->setBeamEnergy(west, 65. * GeV);
      //    pRunInfo->setBeamLifeTime(StBeamDirection, float);
      //    pRunInfo->setBeamFillNumber(StBeamDirection, float);
      //    pRunInfo->setInitialBeamIntensity(StBeamDirection, float);
      pRunInfo->setMagneticField(event->summary()->magneticField());
      //    pRunInfo->setTpcDriftVelocity(StBeamDirection, double);
      event->setRunInfo(pRunInfo);
    }
  }
  return 0;
}

int StAddRunInfoMaker::Finish() { 
  return 0;
}
    

ClassImp(StAddRunInfoMaker)

/***************************************************************************
 *
 * $Log: StAddRunInfoMaker.cxx,v $
 * Revision 1.1  2003/03/06 01:34:18  laue
 * StAddRunInfoMaker is a make helper maker to add the StRunInfo for the
 * only year1 Au+Au 130GeV data
 *
 *
 **************************************************************************/