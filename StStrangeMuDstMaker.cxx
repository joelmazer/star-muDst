//////////////////////////////////////////////////////////////////////////
//                                                                      //
// StStrangeMuDstMaker strangeness micro DST maker                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
#include "StStrangeMuDstMaker.h"
#include "TFile.h"
#include "TChain.h"
#include "StTree.h"
#include "StEvent/StEvent.h"
#include "StMcEventMaker/StMcEventMaker.h"
#include "StEvent.h"
#include "StStrangeEvMuDst.hh"
#include "StStrangeCuts.hh"
#include "StMcEventTypes.hh"
#include "StParticleDefinition.hh"
#include "StMessMgr.h"
#include "StuJitterBug.hh"

// Set maximum file size to 1.9 GB (Root has a 2GB limit)
#define MAXFILESIZE 1900000000

#define EachDoT(proc) \
  for (Int_t i=1; i<strDstT; i++) { \
    if (doT[i]) proc; }

#define EachController(proc) EachDoT(cont[i]->proc);

// Defaults file and tree names;
static char* defFileName = "evMuDst.root";
static char* defName = "StrangeMuDst";
static char* altName = "MuDst";
static char* defTitle = "Strangeness Micro-DST";

Int_t thisRun,thisEvent,lastRun,lastEvent,readEventNumber;
TFile* lastFile;

ClassImp(StStrangeMuDstMaker)
//_____________________________________________________________________________
StStrangeMuDstMaker::StStrangeMuDstMaker(const char *name) : StMaker(name) {

  muDst = 0;
  tree = 0;
  chain = 0;
  evClonesArray = 0;
  evMcArray = 0;
  cutsArray = 0;
  dstMaker = 0;
  cuts = new StStrangeCuts();
  SetNumber(-2);
  outFileNum = 1;
  doT0JitterAbort = kFALSE;

  for (Int_t i=0; i<strDstT; i++) {
    doT[i] = kFALSE;
    cont[i] = 0;
    bsize[i] = 0;
  }

  file = defFileName;

  doMc = kFALSE;
  rw = StrangeNoFile;
  abortEvent = kFALSE;

  bsize[evT] = 64000;

  thisRun = -1;
  thisEvent = -1;
  lastRun = -1;
  lastEvent = -1;
  lastFile = 0;
}
//_____________________________________________________________________________
StStrangeMuDstMaker::~StStrangeMuDstMaker() {

  if (!dstMaker) {           // Don't delete other maker's array!
    delete evClonesArray; evClonesArray = 0;
    if (evMcArray) { delete evMcArray; evMcArray = 0; }
  }
  delete cutsArray; cutsArray = 0;
  delete cuts; cuts = 0;
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::Init() {

  abortEvent = kFALSE;
  firstEvent = kTRUE;
  if (Debug()) gMessMgr->Debug() << "In StStrangeMuDstMaker::Init() ... "
                               << GetName() << endm; 
  if ((rw == StrangeWrite) && (OpenFile() == kStErr)) return kStErr;
  if (!dstMaker) {
    evClonesArray = new TClonesArray("StStrangeEvMuDst",1);
    if (doMc) evMcArray = new TClonesArray("StStrangeEvMuDst",1);
  }
  cutsArray = new TClonesArray("TCut",0);
  StStrangeControllerBase::currentMaker = this;
  {EachDoT(cont[i] = StStrangeControllerBase::Instantiate(i));}

  v0 = cont[v0T];
  xi = cont[xiT];
  kink = cont[kinkT];

  if (rw == StrangeNoKeep) {
    {EachDoT( cont[i]->SetBufferSize(32000) );}
  } else {
    {EachDoT( if (bsize[i]) cont[i]->SetBufferSize(bsize[i]) );}
  }

  if (rw == StrangeRead) {            // READING  the Micro Dst
    InitReadDst();
    if (dstMaker) {
      gMessMgr->Error() << "StStrangeMuDstMaker: cannot read a dst and create "
                        << "a dst with one maker.\n"
			<< "  Will read only." << endm;
      dstMaker = 0;
    }
  } else {                            // CREATING the Micro Dst
    InitCreateDst();
  }
  
  return StMaker::Init();
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::InitReadDst() {
  
  if (!((tree) && (muDst))) {
    gMessMgr->Error(
      "StStrangeMuDstMaker: no appropriate tree in input file(s)!");
    return;
  }

  // 5/20/02 - GVB
  // Correct for a problem with setting branch file names in old MuDSTs.
  // Setting of branch file names has now been removed (never worked anyhow).
  if (strcmp(tree->GetBranch("Event")->GetFileName(),"")) {
    TObjArray* listOfBranches = tree->GetListOfBranches();
    for (int i=0; i<listOfBranches->GetEntriesFast(); i++)
      ((TBranch*) listOfBranches->At(i))->SetFile(muDst);
  }

  tree->SetBranchStatus("*",0);
  tree->SetBranchStatus("Event.*",1);
  tree->SetBranchAddress("Event",&evClonesArray);
  if (doMc) {
    tree->SetBranchStatus("McEvent.*",1);
    tree->SetBranchAddress("McEvent",&evMcArray);
  }
  EachController(InitReadDst());

  if (tree->GetBranch("StrangeCuts")) {
    tree->SetBranchStatus("StrangeCuts.*",1);
    tree->SetBranchAddress("StrangeCuts",&cutsArray);
  } else {
    TOrdCollection* ordCuts = (TOrdCollection*) muDst->Get("StrangeCuts");
    if (ordCuts) {
      cuts->Reset(ordCuts);
    } else {
      gMessMgr->Warning("StStrangeMuDstMaker: no StrangeCuts");
      cuts->UnknownCuts();
    }
  }
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::InitCreateDst() {
  
  tree = new TTree(defName,defTitle);
  tree->SetDirectory(muDst);
  EachController(InitCreateDst());

  Int_t split = 99;
  TBranch* branch = tree->Branch("StrangeCuts",&cutsArray,bsize[evT],split);
  if (!dstMaker) {
    branch = tree->Branch("Event",&evClonesArray,bsize[evT],split);
    if (doMc) {
      branch = tree->Branch("McEvent",&evMcArray,bsize[evT],split);
    }
  }

  if (firstEvent) cuts->Init();
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::InitCreateSubDst() {

  Int_t split=2;
  evClonesArray = dstMaker->GetEvClonesArray();
  TBranch* branch = tree->Branch("Event",&evClonesArray,bsize[evT],split);
  if (doMc) {
    evMcArray = dstMaker->GetEvMcArray();
    branch = tree->Branch("McEvent",&evMcArray,bsize[evT],split);
  }
  EachController(InitCreateSubDst());
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::Make() {

  if (Debug()) gMessMgr->Debug() << "In StStrangeMuDstMaker::Make() ... "
                               << GetName() << endm; 

  if (rw == StrangeRead) {            // READING  the Micro Dst
    return MakeReadDst();
  } else if (!dstMaker) {             // CREATING a new Micro Dst
    return MakeCreateDst();
    
  // Else creating a sub-Micro Dst, done in the Clear() stage.
  // However, since the old Micro Dst's Init() is called AFTER the new
  // one's, special initialization for the subDST must be done on the
  // first time through Make().
  } else if (!evClonesArray) {
    InitCreateSubDst();
  }
  return kStOK;
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::MakeReadDst() {

  if (!((tree) && (muDst))) return kStErr;
  Int_t makerEventNumber = GetNumber();
  if (makerEventNumber == -2) {    // If event numbers aren't supplied,
    readEventNumber = 0;           // start at 0 and increment ourselves.
    SetNumber(-1);                 // Use m_Number = -1 to indicate this.
  } else if (makerEventNumber == -1) {
    readEventNumber++;
  } else {
    readEventNumber = makerEventNumber;
  }

  // LoadTree() returns -4 if no tree is found in the file
  // Identify and skip those files
  Int_t curTree = chain->GetTreeNumber() + 1;
  Bool_t skippingFile = kFALSE;
  for (; curTree < chain->GetNtrees(); curTree++) {
    if ((chain->LoadTree(readEventNumber)) != -4) break;
    skippingFile = kTRUE;
    SkipChainFile(curTree);
  }

  if (((skippingFile) && (curTree == chain->GetNtrees())) ||
      (tree->GetEvent(readEventNumber) <= 0)) return kStErr; // Read the event

  TFile* thisFile = chain->GetFile();
  if (thisFile != lastFile) {
    gMessMgr->Info() << "StStrangeMuDstMaker: Now reading from event file:\n  "
        << thisFile->GetName() << endm;
    lastFile = thisFile;
  }

  // Overcome a bug where event wasn't meant to be recorded into 
  // a muDst, but was anyhow (with the previous event's info)
  thisRun = GetEvent()->run();
  thisEvent = GetEvent()->event();
  if ((thisRun == lastRun) && (thisEvent == lastEvent)) {
    if (makerEventNumber < 0 ) {
      // User is calling Make() (not asking for a specific event).
      // Automatically skip to next event.
      return MakeReadDst();
    } else {
      // User is calling Make(i) (asking for a specific event i).
      // Return a "cleared" event.
      gMessMgr->Warning() <<
        "StStrangeMuDstMaker: event on file has bad event info\n" <<
        "    and should be skipped!" << endm;
      GetEvent()->Clear();
      EachController(Clear());
      return kStOK;
      //return kStSkip;
    }
  } else {
    lastRun = thisRun;
    lastEvent = thisEvent;
  }

  if (cutsArray->GetEntriesFast()) cuts->Reset(cutsArray);

  EachController(MakeReadDst());
 
  return kStOK;
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::MakeCreateDst() {

  if (abortEvent) return kStOK;

  // Get the cut parameters and fill StrangeCuts on the first event
  if (firstEvent) {
    Int_t iSize = cuts->GetSize();
    cuts->Fill(strTypeNames[v0T], GetDataSet("ev0par2"));
    cuts->Fill(strTypeNames[xiT], GetDataSet("exipar"));
    cuts->Fill(strTypeNames[kinkT], GetDataSet("tkf_tkfpar"));
    Int_t fSize = cuts->GetSize();
    if (!(fSize-iSize)) {
      gMessMgr->Warning("StStrangeMuDstMaker: no cut parameters found.");
      cuts->UnknownCuts();
    }
    firstEvent = kFALSE;
  }  

  // Get event
  StEvent* event = (StEvent*) GetInputDS("StEvent");
  if (!event) return kStOK; 

  new((*evClonesArray)[0]) StStrangeEvMuDst(*event);
  cuts->UpdateArray(cutsArray);

  if (doT0JitterAbort && t0JitterAbort(event)) {
    gMessMgr->Warning("StStrangeMuDstMaker: T0 jitter; skipping event.");
    return kStWarn;
  }
  if (!(event->primaryVertex())) {
    gMessMgr->Warning("StStrangeMuDstMaker: no primary vertex; skipping event.");
    return kStWarn;
  }

  EachController(MakeCreateDst(*event));
  if (doMc) MakeCreateMcDst();

  CheckFile();
  tree->Fill();

  return kStOK;
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::MakeCreateMcDst() {

  gMessMgr->Info("StStrangeMuDstMaker::MakeCreateMcDst(): running...");
  
  StMcEvent* mcEvent = 0;
  StMcEventMaker* mcEventMaker = (StMcEventMaker*) GetMaker("StMcEvent");
  if (mcEventMaker) mcEvent = mcEventMaker->currentMcEvent();
  if (!mcEvent)   {
    gMessMgr->Error() << "StStrangeMuDstMaker: no StMcEvent! \n" 
	 << " mc branch and assoc branch  will not be filled. " << endm;       
    return 0;
  }
  new((*evMcArray)[0]) StStrangeEvMuDst(*mcEvent);
  if (!GetMaker("StAssociationMaker")) {
    gMessMgr->Warning() << "StStrangeMuDstMaker: no associated info! \n" 
		      << " assoc branch will not be filled. " << endm;
  }

  //Loop over all MC vertices
  StSPtrVecMcVertex& mcVertices = mcEvent->vertices();
  for (StMcVertexIterator mcVertexIt = mcVertices.begin();
                          mcVertexIt != mcVertices.end(); mcVertexIt++) {
    const StMcTrack* parent = (*mcVertexIt)->parent();
    if (parent) switch (parent->geantId()) {

      // Xi vertex candidates
      case (23) : // Xi-
      case (31) : // AntiXi+
      case (24) : // Omega-
      case (32) : // AntiOmega+
        if (doT[xiT]) cont[xiT]->MakeCreateMcDst(*mcVertexIt);
        break;

      // V0 vertex candidates
      case (10) : // Kaon0Long
      case (16) : // Kaon0Short
      case (18) : // Lambda
      case (26) : // AntiLambda
        // Do all V0's here only if not doing Xi's
        if (doT[v0T] && !(doT[xiT])) cont[v0T]->MakeCreateMcDst(*mcVertexIt);
        break;

      // Kink vertex candidates
      case ( 5) : // Muon+
      case ( 6) : // Muon-
      case ( 8) : // Pion+
      case ( 9) : // Pion-
      case (11) : // Kaon+
      case (12) : // Kaon-
        // Keep all kinks
        if (doT[kinkT]) cont[kinkT]->MakeCreateMcDst(*mcVertexIt);
        break;

      default   : {}
    }
  }//end of loop over MC  vertices

  // If doing Xi's, all Xi V0 daughters are done concurrently with their Xi.
  // Do all non-Xi V0's at the end...
  if (doT[xiT] && doT[v0T]) {
    for (StMcVertexIterator mcVertexIt = mcVertices.begin();
                          mcVertexIt != mcVertices.end(); mcVertexIt++) {
      Bool_t notFromXi = kTRUE;
      const StMcTrack* parent = (*mcVertexIt)->parent();
      if (parent) switch (parent->geantId()) {
        case (18) : // Lambda
        case (26) : // AntiLambda
          { // Check for cascade parentage
            const StMcTrack* parent2 = parent->parent();
            if (parent2) switch (parent2->geantId()) {
              case (23) : // Xi-
              case (31) : // AntiXi+
              case (24) : // Omega-
              case (32) : // AntiOmega+
                notFromXi = kFALSE;
              default   : {}
            }
          }
        case (10) : // Kaon0Long
        case (16) : // Kaon0Short
	  if (notFromXi) cont[v0T]->MakeCreateMcDst(*mcVertexIt);
        default   : {}
      }
    }
  }//end of loop over MC  vertices
  EachController(PrintNumMc());

  return kStOK;
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::MakeCreateSubDst() {

  if (abortEvent) return kStOK;

  EachController(MakeCreateSubDst());
  CheckFile();
  cuts->Reset(dstMaker->Cuts());
  cuts->UpdateArray(cutsArray);
  tree->Fill();

  return kStOK;
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::Clear(Option_t *option) {

  if (Debug()) gMessMgr->Debug() << "In StStrangeMuDstMaker::Clear() ... "
                               << GetName() << endm; 

  for (Int_t i=0; i<subMakers.GetEntries(); i++) {
    StStrangeMuDstMaker* subMaker = (StStrangeMuDstMaker*) (subMakers[i]);
    subMaker->ClearForReal(option);
  }
  if (!dstMaker) {
    ClearForReal(option);
  }
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::ClearForReal(Option_t *option) {

  if (Debug()) gMessMgr->Debug() <<
    "In StStrangeMuDstMaker::ClearForReal() ... " << GetName() << endm; 

  if (tree) {
    if (dstMaker) {                                   // Making a subDST
      MakeCreateSubDst();
    } else {
      if (evClonesArray) evClonesArray->Clear();      // Not if making a subDST
      if (evMcArray) evMcArray->Clear();              // Not if making a subDST
    }
    cutsArray->Clear();

    EachController(Clear());
    if (rw == StrangeNoKeep) tree->Reset();
  }

  abortEvent = kFALSE;
  StMaker::Clear(option);
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::Finish() {

  if (Debug()) gMessMgr->Debug() << "In StStrangeMuDstMaker::Finish() ... "
                               << GetName() << endm; 
  if (rw==StrangeWrite) CloseFile();
  EachController(Finish());

  return kStOK;
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetWrite(const char* eFile) {
  rw = StrangeWrite;
  SetFile(eFile);
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetRead(const char* eFile, char* treeName) {
  rw = StrangeRead;
  if (!eFile) eFile = defFileName;

  if (tree) {
    treeName = const_cast<char*> (tree->GetName());
  } else {
    if (!treeName) treeName = defName;
    tree = (TTree*) (chain = new TChain(treeName,defTitle));
  }

  Int_t nFiles = chain->GetNtrees();
  chain->Add(eFile);
  cuts->ForceUpdateArray();

  if (muDst) return;

  // Have not yet found an appropriate TTree - attempting to find one
  for (Int_t curTree=nFiles; curTree<chain->GetNtrees(); curTree++) {
    Int_t loadResult = chain->LoadTree(0);
    if (loadResult == -2) break;
    muDst = chain->GetFile();
    if (!(loadResult)) return;
    if (strcmp(treeName,defName)) {
      SetTreeName(defName);
      loadResult = chain->LoadTree(0);
      muDst = chain->GetFile();
      if (!(loadResult)) return;
    }
    if (strcmp(treeName,altName)) {
      SetTreeName(altName);
      loadResult = chain->LoadTree(0);
      muDst = chain->GetFile();
      if (!(loadResult)) return;
    }

    // Unable to find tree of name treeName, defName, or altName in file,
    // try with next file...
    SkipChainFile(curTree);
    SetTreeName(treeName);
  }

  // Appropriate tree not found in any files so far
  muDst = 0;
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetRead(StFile* eFiles, char* treeName) {
  while (!(eFiles->GetNextBundle()))
    SetRead(eFiles->GetFileName(0),treeName);
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetNoKeep() {
  rw = StrangeNoKeep;
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetFile(const char* eFile) {
  if (eFile) file = const_cast<char*> (eFile);
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetTreeName(const char* treeName) {
  if ((treeName) && (strcmp(treeName,tree->GetName()))) {
    tree->SetName(treeName);
    TObjArray* chainElems = chain->GetListOfFiles();
    for (Int_t i=0; i<chainElems->GetEntriesFast(); i++)
      ((TNamed*) (chainElems->At(i)))->SetName(treeName);
  }
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SkipChainFile(Int_t curTree) {
  Int_t* listOfOffsets = chain->GetTreeOffset();
  TNamed* chainElem = (TNamed*) (chain->GetListOfFiles()->At(curTree));
  gMessMgr->Warning() <<
    "StStrangeMuDstMaker: Skipped event file (no appropriate TTree):\n  "
    << chainElem->GetTitle() << endm;
  listOfOffsets[curTree+1] = listOfOffsets[curTree];
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::OpenFile() {
  if( ! (muDst = new TFile(file,"RECREATE")) ) {
    gMessMgr->Error() << "StStrangeMuDstMaker: Error opening event file:\n  "
                      << file << endm;
    return kStErr;
  }

  gMessMgr->Info() << "StStrangeMuDstMaker: Opened file for writing:\n"
                   << file << endm;     
  cuts->ForceUpdateArray();
  return kStOk;
}
//_____________________________________________________________________________
Int_t StStrangeMuDstMaker::CloseFile() {
  if (muDst) {
    muDst->Write();
    muDst->cd();
    muDst->Close();
    tree = 0;
  }
  return kStOk;
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::CheckFile() {
  if (muDst) {
    if (muDst->GetBytesWritten() > MAXFILESIZE) {
      gMessMgr->Warning() << "StStrangeMuDstMaker: File size limit "
			  << MAXFILESIZE << " exceeded!\n"
			  << "           Closing file " << file << endm;
      CloseFile();
      char buf_[10];
      sprintf(buf_,"_%d",(++outFileNum));
	TString fixer = file;
	size_t len = fixer.Length();
	if (outFileNum>2) {
	  TString suffix = strrchr(file,'.');
	  size_t last_ = fixer.Last('_');
	  size_t len_ = len - last_;
	  fixer.Remove(last_,len_).Append(buf_).Append(suffix);
	  len = fixer.Length();
	} else {
	  size_t lastdot = fixer.Last('.');
	  fixer.Insert(lastdot,buf_);
	  len = fixer.Length();
	  file = new char[len + 5];
	}
	strncpy(file,fixer.Data(),len);
	(file)[len] = 0;
      OpenFile();
      InitCreateDst();
      if (dstMaker) InitCreateSubDst();
    }
  }
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SelectEvent() {
  EachController(Select(-1));
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::UnselectEvent() {
  EachController(Unselect(-1));
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SubDst(StStrangeMuDstMaker* maker) {
  dstMaker = maker;
  if (!(maker->subMakers.FindObject((TObject*) this)))
    maker->subMakers.Add(this);
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetCorrectionFile(char* fname) {
  StStrangeEvMuDst::SetCorrectionFile(fname);
}
//_____________________________________________________________________________
void StStrangeMuDstMaker::SetFractionFile(char* fname) {
  StStrangeEvMuDst::SetFractionFile(fname);
}
//_____________________________________________________________________________
char* StStrangeMuDstMaker::GetFile() const {
  if (chain) {
    TFile* fptr = chain->GetFile();
    if (fptr) return const_cast<char*> (fptr->GetName());
  }
  return file; 
}       

//_____________________________________________________________________________
// $Id: StStrangeMuDstMaker.cxx,v 3.23 2003/02/10 17:55:55 genevb Exp $
// $Log: StStrangeMuDstMaker.cxx,v $
// Revision 3.23  2003/02/10 17:55:55  genevb
// Output currently read file
//
// Revision 3.22  2003/02/10 16:02:24  genevb
// Now read files using TChains; no splitting of MuDst file
//
// Revision 3.21  2003/01/16 20:38:36  genevb
// Reduce buffer size for NoKeep option
//
// Revision 3.20  2002/06/21 02:43:59  genevb
// handle events without primary vertex better
//
// Revision 3.19  2002/06/19 15:08:40  genevb
// Allow all MC kinks to be kept
//
// Revision 3.18  2002/05/29 19:08:16  genevb
// Better handling of improperly closed files
//
// Revision 3.17  2002/05/20 21:37:12  genevb
// Fixed problem with file names for branches
//
// Revision 3.16  2002/05/10 20:59:31  genevb
// Fixed bug with branch status and changed cuts split level
//
// Revision 3.15  2002/04/30 16:02:48  genevb
// Common muDst, improved MC code, better kinks, StrangeCuts now a branch
//
// Revision 3.14  2001/11/05 23:41:06  genevb
// Add more dEdx, B field info, careful of changes to TTree unrolling
//
// Revision 3.13  2001/09/14 21:39:02  genevb
// Adjustments to not depend on order in which maker Clear() is called
//
// Revision 3.12  2001/08/23 13:20:55  genevb
// Many bug workarounds...
//
// Revision 3.11  2001/05/04 20:15:14  genevb
// Common interfaces and reorganization of components, add MC event info
//
// Revision 3.10  2001/04/25 18:20:53  perev
// HPcorrs
//
// Revision 3.9  2001/01/30 04:06:54  genevb
// Better handling of file switches
//
// Revision 3.8  2001/01/04 01:03:23  genevb
// Add CheckFile() for creating sub-dsts
//
// Revision 3.7  2000/12/18 21:35:18  genevb
// Introduced variable buffer-sizing
//
// Revision 3.6  2000/10/27 21:55:31  genevb
// Allow use of event.root files
//
// Revision 3.5  2000/09/28 20:16:05  jones
// Added doT0JitterAbort() optio; added fix to CheckFile in case of no file
//
// Revision 3.4  2000/09/07 02:22:09  genevb
// Added AbortEvent() functionality
//
// Revision 3.3  2000/08/10 01:14:17  genevb
// Upped basket size of event branch; Set old file format
//
// Revision 3.2  2000/07/17 20:28:40  genevb
// File size limitation workaround, some under the hood improvements
//
// Revision 3.1  2000/07/14 21:28:34  genevb
// Added V0Mc index for XiMc, fixed bug with entries for XiMc, cleaned up controllers
//
// Revision 3.0  2000/07/14 12:56:49  genevb
// Revision 3 has event multiplicities and dedx information for vertex tracks
//
// Revision 2.1  2000/06/09 22:17:10  genevb
// Allow MC data to be copied between DSTs, other small improvements
//
// Revision 2.0  2000/06/02 22:11:54  genevb
// New version of Strangeness micro DST package
//
// Revision 1.8  2000/04/27 14:17:34  genevb
// Safer deletes
//
// Revision 1.7  2000/04/27 14:13:04  genevb
// Added old tree deletion
//
// Revision 1.6  2000/04/18 02:30:04  genevb
// Added multi-file capabilities
//
// Revision 1.5  2000/04/07 18:18:30  genevb
// Additional crash protection
//
// Revision 1.4  2000/04/06 14:51:11  genevb
// Fixed bug with storing event info when making subDST
//
// Revision 1.3  2000/04/05 20:23:53  genevb
// Introduce creating sub-Micro DSTs, dynamic expansion of clones arrays as needed, SetNoKeep() function
//
// Revision 1.2  2000/03/29 20:52:13  genevb
// Added StKinkMuDst, replaced arrays
//
// Revision 1.1  2000/03/29 03:10:07  genevb
// Introduction of Strangeness Micro DST package
//
