// $Id: StXiController.cxx,v 3.2 2000/07/17 20:28:40 genevb Exp $
// $Log: StXiController.cxx,v $
// Revision 3.2  2000/07/17 20:28:40  genevb
// File size limitation workaround, some under the hood improvements
//
// Revision 3.1  2000/07/14 21:28:34  genevb
// Added V0Mc index for XiMc, fixed bug with entries for XiMc, cleaned up controllers
//
// Revision 3.0  2000/07/14 12:56:50  genevb
// Revision 3 has event multiplicities and dedx information for vertex tracks
//
// Revision 2.1  2000/06/09 22:17:11  genevb
// Allow MC data to be copied between DSTs, other small improvements
//
// Revision 2.0  2000/06/05 05:19:46  genevb
// New version of Strangeness micro DST package
//
//
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// StXiController strangeness micro DST controller for Xis              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
#include "TTree.h"
#include "StEvent/StEvent.h"
#include "StMcEventMaker/StMcEventMaker.h"
#include "StAssociationMaker/StAssociationMaker.h"
#include "StAssociationMaker/StTrackPairInfo.hh"
#include "StTrack.h"
#include "StGlobalTrack.h"
#include "StXiVertex.h"
#include "StXiMuDst.hh"
#include "StXiMc.hh"
#include "StMcEventTypes.hh"
#include "StParticleDefinition.hh"
#include "StTrackDetectorInfo.h"

#include "StStrangeControllerInclude.h"  // Location of header for this class

class StStrangeEvMuDst;

//_____________________________________________________________________________
StXiController::StXiController() : StStrangeControllerBase(xiT) {
}
//_____________________________________________________________________________
StXiController::~StXiController() {
}
//_____________________________________________________________________________
Int_t StXiController::MakeReadDst() {

  StStrangeEvMuDst* ev = masterMaker->GetEvent();      // Tell the vertices
  entries = GetN();                                    // about the event
  for (Int_t j = 0; j<entries; j++) {
    StXiMuDst* xi = (StXiMuDst*) (*dataArray)[j];
    xi->SetEvent(ev);
  }
  PrintNumCand("read",entries);
  nEntries += entries;

  return kStOK;
}
//_____________________________________________________________________________
Int_t StXiController::MakeCreateDst(StEvent& event) {

  // Loop over vertices to build array of candidates
  Int_t nBad = 0;
  StSPtrVecXiVertex& xiVertices = event.xiVertices();
  entries = xiVertices.size();
  Int_t asize = dataArray->GetSize();
  if (entries > asize) dataArray->Expand(entries+increment);
  StStrangeEvMuDst* ev = masterMaker->GetEvent();
  for (Int_t i=0; i<entries; i++) {
    StXiVertex* xiVertex = xiVertices[i];
    StV0Vertex* v0Vertex = xiVertex->v0Vertex();
    if (v0Vertex) {
      new((*dataArray)[i]) StXiMuDst(xiVertex,v0Vertex,ev);
    } else nBad++;
  }
  PrintNumCand("found",entries);
  if (nBad) gMessMgr->Warning() << "StXiController: " << nBad
                                << "with missing V0 vertices" << endm;
  nEntries += entries;

  return kStOK;
}
//_____________________________________________________________________________
Int_t StXiController::MakeCreateMcDst(StMcVertex* mcVert) {  

  mcXiMapType* theMcXiMap = 0;
  mcTrackMapType* theMcTrackMap = 0;
  if (assocMaker) {
    theMcXiMap = assocMaker->mcXiMap();
    theMcTrackMap = assocMaker->mcTrackMap();
  }
  StXiVertex* rcXiPartner = 0;
  StMcTrack* Bach = 0;
  StMcTrack* V0daughter = 0;
  Int_t indexRecoArray = -1;
  Int_t count = theMcXiMap->count(mcVert);
  StSPtrVecMcTrack& Daughters = mcVert->daughters();
  
  for (StMcTrackIterator DTrackIt = Daughters.begin();
                         DTrackIt != Daughters.end(); DTrackIt++) {
    if ((Int_t)(*DTrackIt)->particleDefinition()->charge())
      Bach = (*DTrackIt);
    else
      V0daughter = (*DTrackIt);
  }

  if (Bach) {
    StXiMc* xiMc = new((*mcArray)[mcEntries++]) StXiMc(mcVert,Bach);
    if (V0daughter) {
      StStrangeControllerBase* v0Cont = masterMaker->Get(v0T);
      if (v0Cont) {
        StMcVertex* v0Vertex = V0daughter->stopVertex();
        if (v0Vertex) {
          Int_t before = v0Cont->GetNMc();
          v0Cont->MakeCreateMcDst(v0Vertex);
          Int_t after = v0Cont->GetNMc();
          if (!(before==after)) xiMc->SetV0Index(before);
        }
      }
    }
    if ((assocMaker)&&(count>0)) {
      pair<mcXiMapIter,mcXiMapIter> mcXiBounds =
            theMcXiMap->equal_range(mcVert);
      indexRecoArray = -1;
      for(mcXiMapIter mcXiMapIt = mcXiBounds.first;
		      mcXiMapIt != mcXiBounds.second; ++mcXiMapIt) {
        rcXiPartner = (*mcXiMapIt).second;
        // stupid way
        for(Int_t i = 0; i <= GetN(); i++) {
          StXiMuDst* tmpXi = (StXiMuDst*) dataArray->At(i);
          if( fabs(rcXiPartner->position().x()-tmpXi->decayVertexXiX()) < 0.00001 &&
              fabs(rcXiPartner->position().y()-tmpXi->decayVertexXiY()) < 0.00001 &&
              fabs(rcXiPartner->position().z()-tmpXi->decayVertexXiZ()) < 0.00001 )
          { indexRecoArray = i; break; }
        }
      }
      new((*assocArray)[assocEntries++]) 
		    StStrangeAssoc(indexRecoArray,(mcEntries-1));
      if(indexRecoArray!=-1) {
        pair<mcTrackMapIter,mcTrackMapIter> mcTrackBounds = 
              theMcTrackMap->equal_range(Bach);
        StTrackPairInfo*   bestPairInfo = (*mcTrackBounds.first).second;
        for(mcTrackMapIter mcMapIt = mcTrackBounds.first;
                           mcMapIt != mcTrackBounds.second; ++mcMapIt) {
          if ((*mcMapIt).second->commonTpcHits() > bestPairInfo->commonTpcHits())
	         bestPairInfo = (*mcMapIt).second;
        } 
        if (mcTrackBounds.first != mcTrackBounds.second) {
          xiMc->SetHitInfo(bestPairInfo->commonTpcHits());
        }
      }
    }
  }
  
  return kStOK;
}
//_____________________________________________________________________________
ClassImp(StXiController)
