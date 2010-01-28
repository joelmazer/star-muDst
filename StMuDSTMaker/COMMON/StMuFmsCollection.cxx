/***************************************************************************
 *
 * $Id: StMuFmsCollection.cxx,v 1.1 2010/01/25 03:57:39 tone421 Exp $
 *
 * Author: Jingguo Ma, Dec 2009
 ***************************************************************************
 *
 * Description: FMS data interface to StMuFmsHit, StMuFmsCluster and StMuFmsPoint
 *
 ***************************************************************************
 *
 * $Log: StMuFmsCollection.cxx,v $
 * Revision 1.1  2010/01/25 03:57:39  tone421
 * Added FMS and Roman pot arrays
 *
 **************************************************************************/
#include "StMuFmsCollection.h"
#include "StMuFmsHit.h"

static const char rcsid[] = "$Id: StMuFmsCollection.cxx,v 1.1 2010/01/25 03:57:39 tone421 Exp $";

ClassImp(StMuFmsCollection)

StMuFmsCollection::StMuFmsCollection() { mHits = 0;}

StMuFmsCollection::~StMuFmsCollection() { delete mHits; mHits = 0; }

void StMuFmsCollection::init() { mHits = new TClonesArray("StMuFmsHit",0); }

void StMuFmsCollection::addHit(){
  if(!mHits) init();
  int counter = mHits->GetEntriesFast();
  new ((*mHits)[counter]) StMuFmsHit();
  return;
}

unsigned int StMuFmsCollection::numberOfHits() const{
  if(!mHits) return 0;
  return mHits->GetEntries();
}

StMuFmsHit*  StMuFmsCollection::getHit(int hitId){
  if(!mHits) return NULL;
  return (StMuFmsHit*) mHits->At(hitId);
}
