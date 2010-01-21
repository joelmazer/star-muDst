/***************************************************************************
 *
 * $Id: StMuDstMaker.cxx,v 1.95 2010/01/21 02:08:17 fine Exp $
 * Author: Frank Laue, BNL, laue@bnl.gov
 *
 **************************************************************************/
#include "TRegexp.h"
#include "Stiostream.h"
#include "Stsstream.h"
#include "StChain.h"
#include "THack.h"
#include "StEvent/StEvent.h"
#include "StEvent/StTrack.h"
#include "StEvent/StTrackNode.h"
#include "StEvent/StRichSpectra.h"
#include "StEvent/StDetectorState.h"
#include "StEvent/StEventTypes.h"
#include "StEvent/StRunInfo.h"
#include "StEvent/StEventInfo.h"
#include "StEvent/StDcaGeometry.h"
#include "StEventUtilities/StuRefMult.hh"
#include "StEventUtilities/StuProbabilityPidAlgorithm.h"

#include "StarClassLibrary/StPhysicalHelixD.hh"
#include "StarClassLibrary/StTimer.hh"
#include "StarClassLibrary/StMatrixF.hh"

#include "StIOMaker/StIOMaker.h"
#include "StTreeMaker/StTreeMaker.h"

#include "StStrangeMuDstMaker/StStrangeMuDstMaker.h"
#include "StStrangeMuDstMaker/StStrangeEvMuDst.hh"
#include "StStrangeMuDstMaker/StV0MuDst.hh"
#include "StStrangeMuDstMaker/StV0Mc.hh"
#include "StStrangeMuDstMaker/StXiMuDst.hh"
#include "StStrangeMuDstMaker/StXiMc.hh"
#include "StStrangeMuDstMaker/StKinkMuDst.hh"
#include "StStrangeMuDstMaker/StKinkMc.hh"
#include "StStrangeMuDstMaker/StStrangeCuts.hh"


#include "StMuException.hh"
#include "StMuEvent.h"
#include "StMuPrimaryVertex.h"
#include "StMuTrack.h"
#include "StMuDebug.h"
#include "StMuCut.h"
#include "StMuFilter.h"
#include "StMuL3Filter.h"
#include "StMuChainMaker.h"
#include "StMuEmcCollection.h"
#include "StMuEmcUtil.h"
#include "StMuPmdCollection.h"
#include "StMuPmdUtil.h"
#include "StMuPmdHit.h"
#include "StMuPmdCluster.h"
#include "StMuTofHit.h"
#include "StMuTofHitCollection.h"
#include "StMuTofUtil.h"
/// dongx
#include "StEvent/StBTofCollection.h"
#include "StEvent/StBTofRawHit.h"
#include "StEvent/StBTofHeader.h"
#include "StMuBTofHit.h"
#include "StMuBTofHitCollection.h"
#include "StMuBTofUtil.h"
#include "StMuPrimaryTrackCovariance.h"
#include "StMuEzTree.h"
#include "EztEventHeader.h"
#include "EztEmcRawData.h"
#include "EztTrigBlob.h"
#include "EztFpdBlob.h"

#include "StMuDstMaker.h"
#include "StMuDst.h"

#include "TFile.h"
#include "TTree.h"
#include "TClass.h"
#include "TChain.h"
#include "TStreamerInfo.h"
#include "TClonesArray.h"
#include "TEventList.h"

#include "THack.h"
ClassImp(StMuDstMaker)

#if !(ST_NO_NAMESPACES)
  using namespace units;
#endif


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   The default constructor as it is right now was written in order to run
   the StMuDstMaker during reconstruction in the bfc.
   Since the PID table that is needed for muDst production is not passed as
   an argument to the bfc, this default constructor
   sets a specific PID table. This table has to be updated when changing to
   a new production version.
   Also, the standard track and l3 track filters are set.
 */
StMuDstMaker::StMuDstMaker(const char* name) : StIOInterFace(name),
  mStEvent(0), mStMuDst(0), mStStrangeMuDstMaker(0),
  mIOMaker(0), mTreeMaker(0),
  mIoMode(1), mIoNameMode((int)ioTreeMaker), mEventList(0),
  mTrackType(256), mReadTracks(1),
  mReadV0s(1), mReadXis(1), mReadKinks(1), mFinish(0),
  mTrackFilter(0), mL3TrackFilter(0),
  mCurrentFile(0),
  mChain (0), mTTree(0),
  mSplit(99), mCompression(9), mBufferSize(65536*4), mVtxList(100),
  mProbabilityPidAlgorithm(0), mEmcCollectionArray(0), mEmcCollection(0),
  mPmdCollectionArray(0), mPmdCollection(0)

{
  assignArrays();

  mDirName="./";
  mFileName="";
  streamerOff();
  zeroArrays();
  if (mIoMode==ioRead) openRead();
  if (mIoMode==ioWrite) mProbabilityPidAlgorithm = new StuProbabilityPidAlgorithm();

  mEventCounter=0;
  mStMuDst = new StMuDst();
  mEmcUtil = new StMuEmcUtil();
  mPmdUtil = new StMuPmdUtil();
  mTofUtil = new StMuTofUtil();
  mBTofUtil = new StMuBTofUtil();   /// dongx
  mEzTree  = new StMuEzTree();
  if ( ! mStMuDst || ! mEmcUtil || ! mPmdUtil  || ! mTofUtil || ! mBTofUtil || ! mEzTree ) /// dongx
    throw StMuExceptionNullPointer("StMuDstMaker:: constructor. Something went horribly wrong, cannot allocate pointers",__PRETTYF__);


  createArrays();


  setProbabilityPidFile();
  StMuL3Filter* l3Filter = new StMuL3Filter(); setL3TrackFilter(l3Filter);
  StMuFilter* filter = new StMuFilter();       setTrackFilter(filter);
  FORCEDDEBUGMESSAGE("ATTENTION: use standard MuFilter");
  FORCEDDEBUGMESSAGE("ATTENTION: use standard l3 MuFilter");


}

/*! 
 * This method assigns individual TCloneArrays location from one
 * big global one. Dirty init MUST follow the order in StMuArrays.
 * This allows for block initialization/zeroing without problems
 * or side effects such as the one cause by array boundary overwrite
 * if the first TClone in a list of declared variables is used.
 */
void StMuDstMaker::assignArrays()
{
  mArrays         = mAArrays       + 0;       
  mStrangeArrays  = mArrays        + __NARRAYS__;
  mEmcArrays      = mStrangeArrays + __NSTRANGEARRAYS__;    
  mPmdArrays      = mEmcArrays     + __NEMCARRAYS__;    
  mTofArrays      = mPmdArrays     + __NPMDARRAYS__;    
  mBTofArrays     = mTofArrays     + __NTOFARRAYS__;  /// dongx
  mEztArrays      = mBTofArrays    + __NBTOFARRAYS__; /// dongx
}

void StMuDstMaker::clearArrays()
{
  const int ezIndex=__NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+
    __NPMDARRAYS__+__NTOFARRAYS__+__NBTOFARRAYS__;  /// dongx
  for ( int i=0; i<ezIndex; i++) {
    mAArrays[i]->Clear();
    StMuArrays::arrayCounters[i]=0;
  }
  // ezTree classes need Delete, because of TArrayS
  for ( int i=ezIndex; i<__NALLARRAYS__; i++) {
    mAArrays[i]->Delete();
    StMuArrays::arrayCounters[i]=0;
  }
}

void StMuDstMaker::zeroArrays()
{
  memset(mAArrays,0,sizeof(void*)*__NALLARRAYS__);
  memset(mStatusArrays,(char)1,sizeof(mStatusArrays) ); //default all ON
  // ezt arrays switched off
  memset(&mStatusArrays[__NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+
			__NPMDARRAYS__+__NTOFARRAYS__+__NBTOFARRAYS__],(char)0,__NEZTARRAYS__);  /// dongx
  
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/*!
  Selecting SetBranchStatus for particular MuDst branches
  Special names:
   MuEventAll - all branches related to StMuEvent
   StrangeAll - all branches related to StrangeMuDst
   EmcAll     - all branches related to Emc
   PmdAll     - all branches related to Pmd
   TofAll     - all branches related to Tof
   BTofAll    - all branches related to BTof  /// dongx

  By default all branches of MuDst are read. If user wants to read only some of
  them, then:
   SetStatus("*",0)           // all branches off
   SetStatus("MuEventAll",1)  // all standard MuEvent branches ON
   SetStatus("StrangeAll",1)  // all standard Strange branches ON
   SetStatus("EmcAll"    ,1)  // all standard Emc     branches ON
   SetStatus("PmdAll"    ,1)  // all standard Pmd     branches ON
   SetStatus("TofAll"    ,1)  // all standard Tof     branches ON
   SetStatus("BTofAll"   ,1)  // all standard BTof    branches ON  /// dongx
 
   SetStatus("XiAssoc"    ,1) // Strange branch "XiAssoc" is ON  

  Names of branches look StMuArrays::arrayTypes[]
*/
void StMuDstMaker::SetStatus(const char *arrType,int status)
{
  static const char *specNames[]={"MuEventAll","StrangeAll","EmcAll","PmdAll","TofAll","BTofAll","EztAll",0};  /// dongx
  static const int   specIndex[]={
    0, 
    __NARRAYS__,
    __NARRAYS__+__NSTRANGEARRAYS__,
    __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__,
    __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+__NPMDARRAYS__,
    __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+__NPMDARRAYS__+__NTOFARRAYS__,
    __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+__NPMDARRAYS__+__NTOFARRAYS__+__NBTOFARRAYS__,                  /// dongx
    __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+__NPMDARRAYS__+__NTOFARRAYS__+__NBTOFARRAYS__+__NEZTARRAYS__,   /// dongx
    -1};

  if (strncmp(arrType,"St",2)==0) arrType+=2;  //Ignore first "St"
  for (int i=0;specNames[i];i++) {
    if (strcmp(arrType,specNames[i])) continue;
    char *sta=mStatusArrays+specIndex[i];
    int   num=specIndex[i+1]-specIndex[i];
    memset(sta,status,num);
    LOG_INFO << "StMuDstMaker::SetStatus " << status << " to " << specNames[i] << endm;
    if (mIoMode==ioRead)
      setBranchAddresses(mChain);
    return;
  }
  
  TRegexp re(arrType,1);
  for (int i=0;i<__NALLARRAYS__;i++) {
    Ssiz_t len;
    if (re.Index(StMuArrays::arrayNames[i],&len) < 0)	continue;
    LOG_INFO << "StMuDstMaker::SetStatus " << status << " to " << StMuArrays::arrayNames[i] << endm;
    mStatusArrays[i]=status;
  }
  if (mIoMode==ioRead)
    setBranchAddresses(mChain);
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
StMuDstMaker::StMuDstMaker(int mode, int nameMode, const char* dirName, const char* fileName, const char* filter, int maxFiles, const char* name) : StIOInterFace(name),
  mStEvent(0), mStMuDst(0), mStStrangeMuDstMaker(0),
  mIOMaker(0), mTreeMaker(0),
  mIoMode(mode), mIoNameMode(nameMode), 
  mDirName(dirName), mFileName(fileName), mFilter(filter), 
  mMaxFiles(maxFiles), mEventList(0),
  mTrackType(256), mReadTracks(1),
  mReadV0s(1), mReadXis(1), mReadKinks(1), mFinish(0),
  mTrackFilter(0), mL3TrackFilter(0), mCurrentFile(0),
  mSplit(99), mCompression(9), mBufferSize(65536*4),
  mProbabilityPidAlgorithm(0), mEmcCollectionArray(0), mEmcCollection(0),
  mPmdCollectionArray(0), mPmdCollection(0)
{
  assignArrays();
  streamerOff();
  zeroArrays();
  createArrays();
  if (mIoMode==ioRead) openRead();
  if (mIoMode==ioWrite) mProbabilityPidAlgorithm = new StuProbabilityPidAlgorithm();

  setProbabilityPidFile();

  mEventCounter=0;
  mStMuDst = new StMuDst();
  mEmcUtil = new StMuEmcUtil();
  mPmdUtil = new StMuPmdUtil();
  mTofUtil = new StMuTofUtil();
  mBTofUtil= new StMuBTofUtil();  /// dongx
  mEzTree  = new StMuEzTree();
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
StMuDstMaker::~StMuDstMaker() {
  DEBUGMESSAGE1("");
  //clear(999);
  delete mStMuDst;
  delete mTofUtil;
  delete mBTofUtil;   /// dongx
  DEBUGMESSAGE3("after arrays");
  saveDelete(mProbabilityPidAlgorithm);
  saveDelete(mTrackFilter);
  saveDelete(mL3TrackFilter);
  DEBUGMESSAGE3("after filter");
  if (mIoMode== ioWrite ) closeWrite();
  if (mIoMode== ioRead ) closeRead();
  DEBUGMESSAGE3("after close");
  saveDelete(mChain);
//VP  saveDelete(mTTree);
  delete mEmcCollectionArray;
  delete mPmdCollectionArray;
  DEBUGMESSAGE3("out");
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/** Switch of the TObject part of some streamers, so that only the
    datTClonesamenbers of the derived classes are written to disk, but not
    the data members of the base class TObject
*/
void  StMuDstMaker::streamerOff() {
  StMuEvent::Class()->IgnoreTObjectStreamer();
  StMuL3EventSummary::Class()->IgnoreTObjectStreamer();

  StStrangeMuDst::Class()->IgnoreTObjectStreamer();
  StV0MuDst::Class()->IgnoreTObjectStreamer();
  StXiMuDst::Class()->IgnoreTObjectStreamer();
  StKinkMuDst::Class()->IgnoreTObjectStreamer();
  StV0Mc::Class()->IgnoreTObjectStreamer();
  StXiMc::Class()->IgnoreTObjectStreamer();
  StKinkMc::Class()->IgnoreTObjectStreamer();
  StMuTrack::Class()->IgnoreTObjectStreamer();
  StMuPrimaryVertex::Class()->IgnoreTObjectStreamer();
  //  StDcaGeometry::Class()->IgnoreTObjectStreamer();
  StMuPrimaryTrackCovariance::Class()->IgnoreTObjectStreamer();
  StMuHelix::Class()->IgnoreTObjectStreamer();
  StMuEmcHit::Class()->IgnoreTObjectStreamer();
  StMuEmcTowerData::Class()->IgnoreTObjectStreamer();
  StMuPmdHit::Class()->IgnoreTObjectStreamer();
  StMuPmdCluster::Class()->IgnoreTObjectStreamer();
  EztEventHeader::Class()->IgnoreTObjectStreamer();
  EztTrigBlob::Class()->IgnoreTObjectStreamer();
  EztFpdBlob::Class()->IgnoreTObjectStreamer();
  EztEmcRawData::Class()->IgnoreTObjectStreamer();
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::createArrays() {
  /// all stuff
  for ( int i=0; i<__NALLARRAYS__; i++) {
    DEBUGVALUE2(mAArrays[i]);
    clonesArray(mAArrays[i],StMuArrays::arrayTypes[i],StMuArrays::arraySizes[i],StMuArrays::arrayCounters[i]);
    DEBUGVALUE2(mAArrays[i]);
  }
  mStMuDst->set(this);
  // commented to include tof again (subhasis) 
  // mStMuDst->set(mArrays,mStrangeArrays,mEmcArrays,mPmdArrays);
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
TClonesArray* StMuDstMaker::clonesArray(TClonesArray*& p, const char* type, int size, int& counter) {
  DEBUGMESSAGE2("");
  if (p) return p;
  DEBUGVALUE2(type);
  p = new TClonesArray(type, size);
  counter=0;
  return p;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   The Init() routine is used to establish contact to other makers. As it is STAR
   habit (but really really bad coding) we identify the
   other makers by names (instead of passing pointers). Here, the names are
   hard-wired because they have to be identical to the names
   the bfc is assining to the makers. Do not alter these names unless you know
   what you are doing.
*/
int StMuDstMaker::Init(){
  DEBUGMESSAGE2("");
  mIOMaker = (StIOMaker*)GetMaker("IOMaker");
  mTreeMaker = (StTreeMaker*)GetMaker("outputStream");
  mStStrangeMuDstMaker = (StStrangeMuDstMaker*)GetMaker("strangeMuDst");
  TDataSet *muDstSet =  AddObj(mStMuDst,".const");   ///< added for Valeri to be able to pick it up in other makers
  if (muDstSet ) muDstSet ->SetName("muDst");          ///< added for Valeri to be able to pick it up in other makers

  return 0;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::Clear(const char *){
  DEBUGMESSAGE2("");

  if (mIoMode==ioRead)
    return;
  clearArrays();

  DEBUGMESSAGE3("out");
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   Depending on ioMode, calling Make() will initiate the StMuDstMaker to read or
   write the next event. After the Make() function has finished,
   a call to muDst() will return a pointer to an object od type StMuDst. This object
   will hold the current event if the io was successful, or return a null pointer.
*/
int StMuDstMaker::Make(){
  DEBUGMESSAGE2("");
  int returnStarCode = kStOK;
  StTimer timer;
  timer.start();
  if (mIoMode == ioWrite)     returnStarCode = MakeWrite();
  else if (mIoMode == ioRead) returnStarCode = MakeRead();
  DEBUGVALUE2(timer.elapsedTime());
  return returnStarCode;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
Int_t StMuDstMaker::MakeRead(const StUKey &RunEvent)
{
   return MakeRead();
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
Int_t StMuDstMaker::MakeRead() 
{
   int returnStarCode = kStOK;
   if (mIoMode == ioRead) {
     try {
       read();
     }
     catch(StMuExceptionEOF e) {
       e.print();
       returnStarCode = kStEOF;
     }
     catch(StMuException e) {
        e.print();
        returnStarCode = kStERR;
     }
  }
  return returnStarCode;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
Int_t StMuDstMaker::MakeWrite(){
   int returnStarCode = kStOK;
   if (mIoMode == ioWrite) {
     try {
       write();
     }
     catch(StMuExceptionEOF e) {
       e.print();
       returnStarCode = kStEOF;
     }
     catch(StMuException e) {
        e.print();
        returnStarCode = kStERR;
     }
  }
  return returnStarCode;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fill(){
  DEBUGMESSAGE2("");
  mStEvent = (StEvent*) GetInputDS("StEvent");
  if (!mStEvent) {
    DEBUGMESSAGE2("no StEvent");
    throw StMuExceptionNullPointer("no StEvent",__PRETTYF__);
  }
  /// once per event the pid algorithm has to be set up
  /// we make it static for the StMuTrack, because all tracks use the same instance
  if (mProbabilityPidAlgorithm) delete mProbabilityPidAlgorithm;
  mProbabilityPidAlgorithm = new StuProbabilityPidAlgorithm(*mStEvent);
  StMuTrack::setProbabilityPidAlgorithm(mProbabilityPidAlgorithm);
  StMuTrack::setProbabilityPidCentrality(uncorrectedNumberOfNegativePrimaries(*mStEvent));
  try {
    fillTrees(mStEvent);
  }
  catch(StMuException e) {
    e.print();
    throw e;
  }
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::write(){
  DEBUGMESSAGE2("");
  try {
    fill();
  }
  catch (StMuException e) {
    e.print();
    return;
  }

  string ioMakerFileName;
  string theFileName("/dev/null");
  DEBUGVALUE2(mIoNameMode);
  switch (mIoNameMode) {
  case ioFix:
    DEBUGMESSAGE2("===> ioFix\n");
    theFileName = buildFileName( mDirName+"/", basename(mFileName),".MuDst.root");
    break;
  case ioIOMaker:
    DEBUGMESSAGE2("===> ioIOMaker\n");
    ioMakerFileName = string(mIOMaker->GetFile());
    DEBUGVALUE2(ioMakerFileName);
    theFileName = buildFileName( mDirName+"/", basename(ioMakerFileName),".MuDst.root");
    break;
  case ioTreeMaker:
    //    ioMakerFileName = mTreeMaker->GetTree()->GetBaseName();
    ioMakerFileName = mTreeMaker->GetTree()->GetBaseName();
    theFileName = buildFileName(dirname(ioMakerFileName),basename(ioMakerFileName),".MuDst.root");
    break;
  default:
    DEBUGMESSAGE("do not know where to get the filename from");
  }

  DEBUGVALUE2(theFileName.c_str());

  if (theFileName != mCurrentFileName) {
    closeWrite();
    openWrite(theFileName);
    mCurrentFileName = theFileName;
  }

  DEBUGMESSAGE2("now fill tree");
  mTTree->Fill();  THack::IsTreeWritable(mTTree);
  DEBUGMESSAGE2("tree filled");

  return;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
int StMuDstMaker::Finish() {
  DEBUGMESSAGE2("");
  if (mFinish) {
    for ( int i=0; i<10; i++) {
      LOG_INFO << "why are you calling the Finish() again  ???????" << endl;
      LOG_INFO << "are you the stupid chain destructor ???????????" << endl;
    }
  }
  else {
    if (mIoMode== ioWrite ) closeWrite();
    if (mIoMode== ioRead ) closeRead();
    mFinish = true;
  }
  DEBUGMESSAGE3("out");
  return 0;
}
//-----------------------------------------------------------------------
const char* StMuDstMaker::GetFile() const {
  if (mIoMode== ioWrite ) return mCurrentFileName.c_str();
  if (mIoMode== ioRead && mChain && mChain->GetFile())
    return mChain->GetFile()->GetName();
  return 0;
} 
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::setBranchAddresses() {
  setBranchAddresses(mChain);
}

void StMuDstMaker::setBranchAddresses(TChain* chain) {
  // all stuff
  if (!chain) return;
  chain->SetBranchStatus("*",0);
  TString ts;
  Int_t emc_oldformat=0;
  Int_t pmd_oldformat=0;
  for ( int i=0; i<__NALLARRAYS__; i++) {
    if (mStatusArrays[i]==0) continue;
    const char *bname=StMuArrays::arrayNames[i];
    TBranch *tb = chain->GetBranch(bname);
    if(!tb) {
      if (i >= __NARRAYS__+__NSTRANGEARRAYS__ &&
	  i < __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__) {
	emc_oldformat=1;
        continue;
      }

      if (i >= __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__ &&
	  i < __NARRAYS__+__NSTRANGEARRAYS__+__NEMCARRAYS__+__NPMDARRAYS__) {
	pmd_oldformat=1;
        continue;
      } 
      Warning("setBranchAddresses","Branch name %s does not exist",bname);
      continue;
    }
    ts = bname; ts +="*";
    chain->SetBranchStatus (ts,1);
    if (strstr("MuEvent",bname) && mChain->GetBranch("MuEvent.mQA.fX")) {
       // Need to manually switch off Q-vector branches to avoid root warnings
       // Note: the Q-vectors are only present in SL07b
       mChain->SetBranchStatus("MuEvent.mQA*",0);
       mChain->SetBranchStatus("MuEvent.mQB*",0);
       mChain->SetBranchStatus("MuEvent.mQNegEastA*",0);
       mChain->SetBranchStatus("MuEvent.mQNegEastB*",0);
       mChain->SetBranchStatus("MuEvent.mQPosEastA*",0);
       mChain->SetBranchStatus("MuEvent.mQPosEastB*",0);
       mChain->SetBranchStatus("MuEvent.mQNegWestA*",0);
       mChain->SetBranchStatus("MuEvent.mQNegWestB*",0);
       mChain->SetBranchStatus("MuEvent.mQPosWestA*",0);
       mChain->SetBranchStatus("MuEvent.mQPosWestB*",0);
    }
    chain->SetBranchAddress(bname,mAArrays+i);
    assert(tb->GetAddress() == (char*)(mAArrays+i));
  }
  if (emc_oldformat) {
    TBranch *branch=chain->GetBranch("EmcCollection");
    if (branch) {
      Warning("setBranchAddresses","Using backward compatibility mode for EMC");
      if (!mEmcCollectionArray) {
         mEmcCollectionArray=new TClonesArray("StMuEmcCollection",1);
      }
      chain->SetBranchStatus("EmcCollection*",1);
      chain->SetBranchAddress("EmcCollection",&mEmcCollectionArray);
      StMuEmcHit::Class()->IgnoreTObjectStreamer(0);
      mStMuDst->set(this);
    }
  }
  else if (!mEmcCollection) {
    mEmcCollection=new StMuEmcCollection();
    connectEmcCollection();
    mStMuDst->set(this);
  }

  if (pmd_oldformat) {
    TBranch *branch=chain->GetBranch("PmdCollection");
    if (branch) {
      Warning("setBranchAddresses","Using backward compatibility mode for PMD");
      if (!mPmdCollectionArray) {
        mPmdCollectionArray=new TClonesArray("StMuPmdCollection",1);
      }
      chain->SetBranchStatus("PmdCollection*",1);
      chain->SetBranchAddress("PmdCollection",&mPmdCollectionArray);
      StMuPmdCluster::Class()->IgnoreTObjectStreamer(0);
      mStMuDst->set(this);
    }
  }
  else if (!mPmdCollection) {
    mPmdCollection=new StMuPmdCollection();
    connectPmdCollection();
    mStMuDst->set(this);
  }
  mTTree = mChain->GetTree();
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
int StMuDstMaker::openRead() {
  DEBUGVALUE2(mDirName.c_str());
  DEBUGVALUE2(mFileName.c_str());
  DEBUGVALUE2(mFilter.c_str());

  StMuChainMaker chainMaker("MuDst");
  mChain = chainMaker.make(mDirName, mFileName, mFilter, mMaxFiles);

  if (mChain){
    DEBUGVALUE3(mChain);
    setBranchAddresses(mChain);

    mStMuDst->set(this);
  }

  return 0;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::read(){
  if (!mChain){
    DEBUGMESSAGE2("ATTENTION: No StMuChain ... results won't be exciting (nothing to do)");
    throw StMuExceptionNullPointer("No input files",__PRETTYF__);
    return;
  }

  DEBUGMESSAGE2("");
  if (mChain->GetCurrentFile()) {
    DEBUGVALUE2(mChain->GetCurrentFile()->GetName());
  }

  if ( !mEventList ) {
    int bytes = mChain->GetEntry(mEventCounter++);
    while (bytes<=0 ) {
      DEBUGVALUE3(mEventCounter);
      if ( mEventCounter >= mChain->GetEntriesFast() ) throw StMuExceptionEOF("end of input",__PRETTYF__);
      bytes = mChain->GetEntry(mEventCounter++);
      DEBUGVALUE3(bytes);
    }
  }
  else {
    int bytes = mChain->GetEntry( mEventList->GetEntry( mEventCounter++ ) );
    while ( bytes<=0 ) {
      DEBUGVALUE3(mEventCounter);
      if ( mEventCounter >= mEventList->GetN() ) throw StMuExceptionEOF("end of event list",__PRETTYF__);
      bytes = mChain->GetEntry( mEventList->GetEntry( mEventCounter++ ) );
      DEBUGVALUE3(bytes);
    }
  }
  if (GetDebug()>1) printArrays();
  mStMuDst->set(this);
  fillHddr();
  mStMuDst->setVertexIndex(0);
  mStMuDst->collectVertexTracks();   // Make temp list of tracks for current prim vtx
  return;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::closeRead(){
  DEBUGMESSAGE2("");
  if (mChain) mChain->Delete();
  mChain = 0;
 }
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::openWrite(string fileName) {
  DEBUGVALUE2(fileName.c_str());
  // creat a Picoevent and and output file
  DEBUGMESSAGE2("now create file");
  mCurrentFile = new TFile(fileName.c_str(),"RECREATE","StMuDst");

  if (mCurrentFile->IsZombie()) throw StMuExceptionNullPointer("no file openend",__PRETTYF__);

  mCurrentFile->SetCompressionLevel(mCompression);

  // Create a ROOT Tree and one superbranch
  DEBUGMESSAGE2("now create trees and branches");

  TBranch* branch;
  int bufsize = mBufferSize;
  if (mSplit) bufsize /= 4;

  Long64_t MAXLONG=(Long64_t) TMath::Power(2,sizeof(Long64_t)*8)-1; // 1900000000 <=> 1.9 GB
  LOG_INFO << "Tree size MAX will be " << (float) MAXLONG/1000/1000/1000 << " GB " << endm;

  //  all stuff
  mTTree = new TTree("MuDst", "StMuDst",mSplit);
  mTTree->SetMaxTreeSize(MAXLONG);  // limited to 1.9 GB  - set to maximum
  //mTTree->SetAutoSave(1000000);   // autosave when 1 Mbyte written
  DEBUGMESSAGE2("all arrays");
  for ( int i=0; i<__NALLARRAYS__; i++) {
    if (mStatusArrays[i]==0) continue;
    branch = mTTree->Branch(StMuArrays::arrayNames[i],&mAArrays[i], bufsize, mSplit);
  }
  mCurrentFileName = fileName;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::closeWrite(){
  DEBUGMESSAGE(__PRETTYF__);
  if (mTTree && mCurrentFile) {
    LOG_INFO << " ##### " << __PRETTYF__ << endm;
    LOG_INFO << " ##### File=" << mCurrentFile->GetName() << " ";
    LOG_INFO << " NumberOfEvents= " << mTTree->GetEntries() << " ";
    LOG_INFO << " ##### " << endm;
  }
  //if (mTTree) mTTree->AutoSave();
  
  if (mCurrentFile) {
    mCurrentFile->Write();
    mCurrentFile->Close();
  }
  mTTree = 0;
  mCurrentFile = 0;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillTrees(StEvent* ev, StMuCut* cut){
  DEBUGMESSAGE2("");

 try {
    fillEvent(ev);
    fillL3AlgorithmInfo(ev);
    fillDetectorStates(ev);
    fillEmc(ev);
    fillPmd(ev);
    fillTof(ev);
    fillBTof(ev);  /// dongx
    fillEzt(ev);
  }
  catch(StMuException e) {
    e.print();
    throw e;
  }

  try {
    fillVertices(ev);
  }
  catch(StMuException e) {
    e.print();
    throw e;
  }

  try {
    fillTracks(ev,mTrackFilter);
  }
  catch(StMuException e) {
    e.print();
    throw e;
  }

  try {
    fillL3Tracks(ev, mL3TrackFilter);
  }
  catch(StMuException e) {
    e.print();
    throw e;
  }

  try {
    fillStrange(mStStrangeMuDstMaker);
  }
  catch(StMuException e) {
    e.print();
    throw e;
  }
  mStMuDst->set(this);
  mStMuDst->fixTofTrackIndices();
}



//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillEvent(StEvent* ev, StMuCut* cut) {
  DEBUGMESSAGE2("");
  StMuEvent *typeOfEvent=0;
  if (!ev) throw StMuExceptionNullPointer("no StEvent",__PRETTYF__);
  StTimer timer;
  timer.start();
  if (!cut || cut->pass(ev)) {
    DEBUGMESSAGE3("");
    addType(mArrays[muEvent],ev,typeOfEvent);
  }
  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillEmc(StEvent* ev) {
  DEBUGMESSAGE2("");
  StEmcCollection* emccol=(StEmcCollection*)ev->emcCollection();
  if (!emccol)  return; //throw StMuExceptionNullPointer("no StEmcCollection",__PRETTYF__);
  StTimer timer;
  timer.start();

  TClonesArray *tca = mEmcArrays[muEmcTow];
  new((*tca)[0]) StMuEmcTowerData();
  if (!mEmcCollection) {
    mEmcCollection=new StMuEmcCollection();
    connectEmcCollection();
    mStMuDst->set(this);
  }
  mEmcUtil->fillMuEmc(mEmcCollection,emccol);

  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillPmd(StEvent* ev) {
  DEBUGMESSAGE2("");
  StPhmdCollection* phmdColl=(StPhmdCollection*)ev->phmdCollection();
  if (!phmdColl)  return; //throw StMuExceptionNullPointer("no StPhmdCollection",__PRETTYF__);
  StTimer timer;
  timer.start();

  if (!mPmdCollection) {
    mPmdCollection=new StMuPmdCollection();
    connectPmdCollection();
    mStMuDst->set(this);
  }
  mPmdUtil->fillMuPmd(phmdColl,mPmdCollection);

  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void StMuDstMaker::fillTof(StEvent* ev) {
  DEBUGMESSAGE2("");
  StTofCollection *tofcol = ev->tofCollection();
  // run 5 - dongx
  if( !ev || !tofcol || (!tofcol->dataPresent()&&!tofcol->rawdataPresent()) )
    return;  //throw StMuExceptionNullPointer("no StTofDataCollection",__PRETTYF__);
  StTimer timer;
  timer.start();

  // fill tofHit
  StMuTofHitCollection muTofHitColl;
  mTofUtil->fillMuTofHit(&muTofHitColl, tofcol);
  for(size_t i=0; i < muTofHitColl.size(); i++) {
    StMuTofHit* tofMuHit = (StMuTofHit *)muTofHitColl.getHit(i);
    addType( mTofArrays[muTofHit], *tofMuHit );
  }

  // fill tofData
  StSPtrVecTofData &tofData = tofcol->tofData();
  for(size_t i=0; i < tofData.size(); i++) {
    addType( mTofArrays[muTofData], *tofData[i] );
  }

  // run 5 - dongx
  // fill tofRawData
  StSPtrVecTofRawData &tofRawData = tofcol->tofRawData();
  for(size_t i=0; i < tofRawData.size(); i++) {
    addType( mTofArrays[muTofRawData], *tofRawData[i] );
  }

  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/// dongx
void StMuDstMaker::fillBTof(StEvent* ev) {
  DEBUGMESSAGE2("");
  StBTofCollection *btofcol = ev->btofCollection();
  if( !ev || !btofcol || !btofcol->rawHitsPresent() )
    return;  //throw StMuExceptionNullPointer("no StBTofRawHitCollection",__PRETTYF__);
  StTimer timer;
  timer.start();

  // fill btofHit
  StMuBTofHitCollection muBTofHitColl;
  mBTofUtil->fillMuBTofHit(&muBTofHitColl, btofcol);
  for(size_t i=0; i < muBTofHitColl.size(); i++) {
    StMuBTofHit* btofMuHit = (StMuBTofHit *)muBTofHitColl.getHit(i);
    addType( mBTofArrays[muBTofHit], *btofMuHit );
  }

  // fill btofRawHit
  StSPtrVecBTofRawHit &btofRawHits = btofcol->tofRawHits();
  for(size_t i=0; i < btofRawHits.size(); i++) {
    addType( mBTofArrays[muBTofRawHit], *btofRawHits[i] );
  }

  // fill btofHeader
  StBTofHeader *btofHeader = btofcol->tofHeader();
  addType( mBTofArrays[muBTofHeader], *btofHeader);

  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillEzt(StEvent* ev) {
  if (ev==0)
    return;
  char *eztArrayStatus=&mStatusArrays[__NARRAYS__+__NSTRANGEARRAYS__+
				      __NEMCARRAYS__+__NPMDARRAYS__+
				      __NTOFARRAYS__+__NBTOFARRAYS__]; /// dongx
  if(eztArrayStatus[muEztHead]){
    EztEventHeader* header = mEzTree->copyHeader(ev);
    addType(mEztArrays[muEztHead], *header);
  }

  if(eztArrayStatus[muEztTrig]) {
    EztTrigBlob* trig = mEzTree->copyTrig(ev);
    if (trig)
      addType(mEztArrays[muEztTrig], *trig);
  }

  if(eztArrayStatus[muEztFpd]) {
    EztFpdBlob* fpd = mEzTree->copyFpd(ev);
    addType(mEztArrays[muEztFpd], *fpd);
  }

  if(eztArrayStatus[muEztETow] || eztArrayStatus[muEztESmd]) {
    StEmcCollection* emcCol=(StEmcCollection*)ev->emcCollection();
    if(emcCol==0){
      gMessMgr->Message("","W") <<  GetName()<<"::fillEzt(), missing StEmcCollection, EEMC raw data NOT saved in muDst" <<endm;
    } else { //........... EMC-Collection in StEvent exist
      StEmcRawData *eeRaw=emcCol->eemcRawData();
      
      if(eztArrayStatus[muEztETow]) {
	EztEmcRawData* ETow = mEzTree->copyETow(eeRaw);
	addType(mEztArrays[muEztETow], *ETow);
      }
      
      if(eztArrayStatus[muEztESmd]) {
	EztEmcRawData* ESmd = mEzTree->copyESmd(eeRaw);
	addType(mEztArrays[muEztESmd], *ESmd);
      }
    }
  }
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillL3AlgorithmInfo(StEvent* ev) {
  DEBUGMESSAGE2("");
  if ( !ev->l3Trigger() ) return;
  if ( !ev->l3Trigger()->l3EventSummary()) return;

  StTimer timer;
  timer.start();
  StL3EventSummary* l3 = ev->l3Trigger()->l3EventSummary();
  int n = l3->numberOfAlgorithms();
  for (int i=0; i<n; i++) {
    if (l3->algorithms()[i]->accept())
      addType( mArrays[muAccept], *l3->algorithms()[i] );
    else
      addType( mArrays[muReject], *l3->algorithms()[i] );
  }
  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillVertices(StEvent* ev) {
  DEBUGMESSAGE2("");
  StTimer timer;
  timer.start();

  StMuPrimaryVertex *typeOfVertex=0;
  Int_t n_vtx = ev->numberOfPrimaryVertices();
  DEBUGVALUE2(n_vtx);
  mVtxList.Clear();
  for (Int_t i_vtx=0; i_vtx < n_vtx; i_vtx++) {
    const StPrimaryVertex *vtx=ev->primaryVertex(i_vtx);
    addType( mArrays[muPrimaryVertex], vtx, typeOfVertex );
    mVtxList.AddAt(ev->primaryVertex(i_vtx),i_vtx);
  }
  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillTracks(StEvent* ev, StMuCut* cut) {
  DEBUGMESSAGE2("");
  StTimer timer;
  timer.start();

  StSPtrVecTrackNode& nodes= ev->trackNodes();
  DEBUGVALUE2(nodes.size());
  for (StSPtrVecTrackNodeConstIterator iter=nodes.begin(); iter!=nodes.end(); iter++) {
    addTrackNode(ev, *iter, cut, mArrays[muGlobal], mArrays[muPrimary], mArrays[muOther], mArrays[muCovGlobTrack], mArrays[muCovPrimTrack], false);
  }
  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillL3Tracks(StEvent* ev, StMuCut* cut) {
  DEBUGMESSAGE2("");
  if (!ev->l3Trigger()) return;

  StTimer timer;
  timer.start();
  StSPtrVecTrackNode& nodes= ev->l3Trigger()->trackNodes();
  DEBUGVALUE2(nodes.size());
  for (StSPtrVecTrackNodeConstIterator iter=nodes.begin(); iter!=nodes.end(); iter++) {
    addTrackNode(ev, *iter, cut, mArrays[muL3], 0, 0, 0, 0, true );
  }
  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::fillDetectorStates(StEvent* ev) {
  DEBUGMESSAGE2("");
  StTimer timer;
  timer.start();
  for (int i=0; i<StMuArrays::arraySizes[muState]; i++) {
    StDetectorState* state = ev->detectorState((StDetectorId) i);
    if (state) {
      addType( mArrays[muState], *state );
    }
  }
  timer.stop();
  DEBUGVALUE2(timer.elapsedTime());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::addTrackNode(const StEvent* ev, const StTrackNode* node, StMuCut* cut,
				  TClonesArray* gTCA, TClonesArray* pTCA, TClonesArray* oTCA, TClonesArray* covgTCA, TClonesArray* covpTCA, bool l3) {
  DEBUGMESSAGE3("");
  const StTrack* tr=0;

  /// do global track
  int index2Global =-1;
  if (gTCA) {
    const StTrack *pr_tr = node->track(primary);
    const StVertex *vtx = 0;
    if (pr_tr)
       vtx = pr_tr->vertex();
    if (vtx==0)
       vtx = ev->primaryVertex();	
	
    tr= node->track(global);
    if (tr && !tr->bad()) index2Global = addTrack(gTCA, ev, tr, vtx, cut, -1, l3, covgTCA, covpTCA);
  }
  /// do primary track track
  int index;
  if (pTCA) {
    tr = node->track(primary);
    if (tr && !tr->bad()) index = addTrack(pTCA, ev, tr, tr->vertex(), cut, index2Global, l3, covgTCA, covpTCA);
  }
  /// all other tracks
  if (oTCA) {
    size_t nEntries = node->entries();
    for (size_t j=0; j<nEntries; j++) { /// loop over all tracks in tracknode
      tr = node->track(j);
      if (tr && !tr->bad() && (tr->type()!=global) && (tr->type()!=primary) ) { /// exclude global and primary tracks
	index = addTrack(oTCA, ev, tr, tr->vertex(), cut, index2Global, l3);
      }
    }
  }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int StMuDstMaker::addTrack(TClonesArray* tca, const StEvent*event, const StTrack* track, const StVertex *vtx, StMuCut* cut, int index2Global, bool l3,
			   TClonesArray* covgTCA, TClonesArray* covpTCA) {
  DEBUGMESSAGE3("");
  StRichSpectra typeOfStRichSpectra;
  int index = -1;
  int index2RichSpectra=-1;
  /// if (!tca || !track) return index; /// I made sure that the array anf the track is there
  int counter = tca->GetEntries();
  try{
    if (cut && !cut->pass(track)) throw StMuExceptionBadValue("failed track cut",__PRETTYF__);
    // add StRichSpectra if StRichPidTraits are found
    // we have to do this more elegant
    StRichSpectra* rich = richSpectra(track);
    if (rich) {
      index2RichSpectra  =  addType( mArrays[muRich], *rich );
    }
    StMuTrack *muTrack = new((*tca)[counter]) StMuTrack(event, track, vtx, index2Global, index2RichSpectra, l3, &mVtxList);
    if (track->type() == primary) {
      if (covpTCA) {
	Int_t countCOVPTCA = covpTCA->GetEntries();
#if 0
	const StMatrixF covMatrix = track->fitTraits().covariantMatrix();
	new((*covpTCA)[countCOVPTCA]) StMuPrimaryTrackCovariance(covMatrix);
#else
	//	cout << track->fitTraits().covariantMatrix() << endl;
	const Float_t*    cov      = track->fitTraits().covariance();
	new((*covpTCA)[countCOVPTCA]) StMuPrimaryTrackCovariance(cov);
#endif
	muTrack->setIndex2Cov(countCOVPTCA);
      }
    }
    else {
      if (track->type() == global) {
	if (covgTCA) {
	  Int_t countCOVGTCA = covgTCA->GetEntries();
	  const StDcaGeometry *dcaGeometry = ((StGlobalTrack *)track)->dcaGeometry();
	  if (dcaGeometry) {
	    new((*covgTCA)[countCOVGTCA]) StDcaGeometry(*dcaGeometry);
	    muTrack->setIndex2Cov(countCOVGTCA);
	  }
	}
      }
    }
    index = counter;
  }
  catch (StMuException e) {
    IFDEBUG3(e.print());
  }
  return index;  /// return index to self if newly created, else return -1;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
StRichSpectra* StMuDstMaker::richSpectra(const StTrack* track) {
  DEBUGMESSAGE3("");
  const StPtrVecTrackPidTraits& traits = track->pidTraits(kRichId);
  for (StPtrVecTrackPidTraitsConstIterator traitIter=traits.begin();traitIter!=traits.end();++traitIter) {
    StRichPidTraits* pid = dynamic_cast<StRichPidTraits*>(*traitIter);
    if (pid) return pid->getRichSpectra();
  }
  return 0;
}
void StMuDstMaker::fillStrange(StStrangeMuDstMaker* maker) {
  DEBUGMESSAGE2("");
  /// now fill the strangeness stuff
  if (!maker) throw StMuExceptionNullPointer("no StrangeMuDstMaker",__PRETTYF__);

  StStrangeEvMuDst *ev=0;
  StV0MuDst *v0=0;
  StStrangeAssoc *assoc=0;
  StXiMuDst *xi=0;
  StKinkMuDst *kink=0;
  StV0Mc *v0Mc=0;
  StXiMc *xiMc=0;
  StKinkMc *kinkMc=0;
  TCut *strangeCut=0;

  addType(maker->GetEvClonesArray(),  mStrangeArrays[0],ev);
  addType(maker->GetEvMcArray(),      mStrangeArrays[1],ev);

  addType(maker->GetV0ClonesArray(),  mStrangeArrays[2],v0);
  addType(maker->GetV0McArray(),      mStrangeArrays[3],v0Mc);
  addType(maker->GetV0AssocArray(),   mStrangeArrays[4],assoc);

  addType(maker->GetXiClonesArray(),  mStrangeArrays[5],xi);
  addType(maker->GetXiMcArray(),      mStrangeArrays[6],xiMc);
  addType(maker->GetXiAssocArray(),   mStrangeArrays[7],assoc);

  addType(maker->GetKinkClonesArray(),mStrangeArrays[8],kink);
  addType(maker->GetKinkMcArray(),    mStrangeArrays[9],kinkMc);
  addType(maker->GetKinkAssocArray(), mStrangeArrays[10],assoc);

  addType(maker->GetCutsArray(), mStrangeArrays[11],strangeCut);

}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
template <class T>
int StMuDstMaker::addType(TClonesArray* tcaFrom, TClonesArray* &tcaTo ,T *t) {
  if (tcaFrom && tcaTo) {
    int n = tcaFrom->GetEntries();
    int counter = tcaTo->GetEntries();
    for (int i=0; i<n;i++) {
	// old     new((*tcaTo)[counter++]) T( (T&)*tcaFrom->UncheckedAt(i) );
	new((*tcaTo)[counter++]) T( *(T*)(void*)tcaFrom->UncheckedAt(i) );
    }
  }
  return 0;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// int StMuDstMaker::addType(TClonesArray* tcaTo , StMuEmcCollection *t) {
//   int counter =-1;
//   if (tcaTo) {
//     counter = tcaTo->GetEntries();

//     new((*tcaTo)[counter]) StMuEmcCollection();
//   }
//   return counter;
// }
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
template <class T>
int StMuDstMaker::addType(TClonesArray* tcaTo ,T &t) {
  int counter =-1;
  if (tcaTo) {
    counter = tcaTo->GetEntries();
    new((*tcaTo)[counter]) T( t );
  }
  return counter;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
template <class T, class U>
int StMuDstMaker::addType(TClonesArray* tcaTo ,U &u,T *t) {
  int counter =-1;
  if (tcaTo) {
    counter = tcaTo->GetEntries();
    DEBUGMESSAGE2("");
    new((*tcaTo)[counter]) T(u);
  }
  return counter;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
string StMuDstMaker::buildFileName(string dir, string fileName, string extention){
  DEBUGMESSAGE3(dir.c_str());
  DEBUGMESSAGE3(fileName.c_str());
  DEBUGMESSAGE3(extention.c_str());
  fileName = dir + fileName + extention;
  while (fileName.find("//")!=string::npos) {
    int pos = fileName.find("//");
    fileName.erase(pos,1);
  }
  return fileName;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
string StMuDstMaker::basename(string s){
  DEBUGVALUE3(s.c_str());
  string name(s);
  DEBUGVALUE3(name.c_str());
  size_t pos;
  pos = name.find_last_of("/");
  if (pos!=string::npos ) name.erase(0, pos );
  DEBUGVALUE3(name.c_str());
  pos = name.find_first_of(".");
  if (pos!=string::npos ) name.erase(pos,name.length()-pos );
  DEBUGVALUE3(name.c_str());
  return name;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
string StMuDstMaker::dirname(string s){
  string name(s);
  DEBUGVALUE3(name.c_str());
  size_t pos;
  pos = name.find_last_of("/");

  if (pos != string::npos ) name.erase(pos, name.length());
  if (name == s) name =".";

  name=name+"/";
  DEBUGVALUE3(name);
  return name;
}

void StMuDstMaker::setProbabilityPidFile(const char* file) {
  ostrstream flnm;

  if ( ! file ){
    const char *PIDtable="PIDTableP01gl.root";

    flnm << getenv("STAR") << "/StarDb/dEdxModel/" << PIDtable << ends;
    FORCEDDEBUGMESSAGE("ATTENTION: pid table hardwired to " << flnm.str() );

  } else {
    flnm << file << ends;
    FORCEDDEBUGMESSAGE("Using pid table to user value " << flnm.str() );
  }


  if (mProbabilityPidAlgorithm)
    mProbabilityPidAlgorithm->readParametersFromFile(flnm.str());
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuDstMaker::printArrays()
{
// all stuff
  TClonesArray *tcl;
  for ( int i=0; i<__NALLARRAYS__; i++) {
    if (mStatusArrays[i]==0) continue;
    tcl = mAArrays[i];
    printf(" Array %s\t = %s::%s(%d)\n",
    StMuArrays::arrayNames[i],
    tcl->ClassName(),tcl->GetName(),tcl->GetEntriesFast());
  }

}
//-----------------------------------------------------------------------
void StMuDstMaker::fillHddr()
{
  StMuEvent 		*me = mStMuDst->event();
  if (me==0)
    return;
  StEventInfo 		&ei = me->eventInfo();
  StRunInfo 		&ri = me->runInfo();
  StEvtHddr *hd = GetEvtHddr();

  hd->SetRunNumber(ei.runId())	;
  hd->SetEventType(ei.type().Data());
  hd->SetTriggerMask(ei.triggerMask())	;
//hd->SetInputTriggerMask(???);
  hd->SetBunchCrossingNumber(ei.bunchCrossingNumber(0),ei.bunchCrossingNumber(1));
  hd->SetCenterOfMassEnergy(ri.centerOfMassEnergy());
//hd->SetBImpact  (float b)  	;
//hd->SetPhiImpact(float p)  	;
//hd->SetPhImpact (float p)  	;
//hd->SetAEast(int a)		;
//hd->SetZEast(int z)		;
//hd->SetAWest(int a)		;
//hd->SetZWest(int z)		;
//hd->SetLuminosity(float lu)	;
  hd->SetGMTime((unsigned int)ei.time());
  hd->SetProdDateTime(ri.productionTime());
//hd->SetIventNumber(int iv)	;
  hd->SetEventSize(ei.eventSize());
  hd->SetEventNumber(ei.id())	;
//hd->SetGenerType(int g);
}

//-----------------------------------------------------------------------
void StMuDstMaker::connectEmcCollection() {
  mEmcCollection->setTowerData((StMuEmcTowerData*)(*(mEmcArrays[muEmcTow]))[0]);
  mEmcCollection->setPrsArray(2,mEmcArrays[muEmcPrs]);
  mEmcCollection->setSmdArray(3,mEmcArrays[muEmcSmde]);
  mEmcCollection->setSmdArray(4,mEmcArrays[muEmcSmdp]);
  mEmcCollection->setPrsArray(6,mEmcArrays[muEEmcPrs]);
  mEmcCollection->setSmdArray(7,mEmcArrays[muEEmcSmdu]);
  mEmcCollection->setSmdArray(8,mEmcArrays[muEEmcSmdv]);
}

//-----------------------------------------------------------------------
void StMuDstMaker::connectPmdCollection() {
  mPmdCollection->setPmdHitArray(mPmdArrays[muPmdHit]);
  mPmdCollection->setCpvHitArray(mPmdArrays[muCpvHit]);
  mPmdCollection->setPmdClusterArray(mPmdArrays[muPmdCluster]);
  mPmdCollection->setCpvClusterArray(mPmdArrays[muCpvCluster]);
}

/***************************************************************************
 *
 * $Log: StMuDstMaker.cxx,v $
 * Revision 1.95  2010/01/21 02:08:17  fine
 * RT #1803: Restore the broken MakeRead/MakeWrite interface to fix Skip event method
 *
 * Revision 1.94  2009/12/01 21:56:35  tone421
 * Implemented changes as per http://www.star.bnl.gov/rt2/Ticket/Display.html?id=1734
 *
 * Revision 1.93  2009/05/22 23:48:18  fine
 * Test I/O errors after filling the TTree
 *
 * Revision 1.92  2009/05/22 22:25:31  fine
 * Add the Zombue test for TFile ctors
 *
 * Revision 1.91  2009/04/27 20:50:43  perev
 * Change type of DataSet
 *
 * Revision 1.90  2009/03/10 23:43:53  jeromel
 * Set tree size to max size
 *
 * Revision 1.89  2009/02/20 16:37:44  tone421
 * *** empty log message ***
 *
 * Revision 1.87  2008/10/03 17:50:42  tone421
 * Added mVtxList(100); see http://www.star.bnl.gov/HyperNews-star/protected/get/starsoft/7529.html
 *
 * Revision 1.86  2008/04/14 21:32:12  fisyak
 * Remove stripping TObject from StDcaGeometry, because StDcaGeometry is inherit from StObject and this stripping brakes schema evolution
 *
 * Revision 1.85  2008/03/19 14:51:03  fisyak
 * Add two clone arrays for global and primary track covariance matrices, remove mSigmaDcaD and mSigmaDcaZ
 *
 * Revision 1.84  2007/08/31 01:55:05  mvl
 * Added protection against corrupted files by checking for return code -1 from TTree:GetEntry(). StMuDstMaker will silently skip these events; StMuIOMaker returns kStWarn.
 *
 * Revision 1.83  2007/08/02 20:46:47  mvl
 * Switch off Q-vector branhces in StMuDstMaker and increase version number in StMuEvent.
 * This is to avoid wranings when reading P07ib data which has Q-vector information stored with more recent libraries.
 *
 * Revision 1.82  2007/05/16 18:50:48  mvl
 * Cleanup of output. Replaced cout with LOG_INFO etc.
 *
 * Revision 1.81  2007/04/27 17:07:01  mvl
 * Added protection against StEvent::triggerData() == 0 in EZTREE.
 *
 * Revision 1.80  2007/04/20 06:26:00  mvl
 * Removed Q-vector calculation. Will implement utility class instead.
 *
 * Revision 1.78  2007/02/07 07:53:09  mvl
 * Added SetEventList function to read only pre-selected events (by J. Webb)
 *
 * Revision 1.77  2006/12/20 21:53:15  mvl
 * Added warning when file list not found (read mode)
 *
 * Revision 1.76  2006/09/10 00:58:43  mvl
 * Roll-back of previous changes for ROOT 5.12. Problem has been resolved inside ROOT.
 *
 * Revision 1.75  2006/08/18 20:09:51  fine
 * ROOT 5.12 bug workaround See: STAR Bug 741
 *
 * Revision 1.74  2006/07/28 18:25:11  mvl
 * Added call to StMuDst::setVertexIndex(0) to StMuDstMaker::read() to reset the current vertex index to 0 for every event
 *
 * Revision 1.73  2006/02/08 23:35:36  mvl
 * Added overloaded version for StIOInterface::GetFile() to return name
 * of current input or output file (depending on read or write mode)
 * StIOInterface::GetFileName() is an alias for this function.
 *
 * Revision 1.72  2005/10/18 17:55:43  mvl
 * Fixed initialisation problem of mCurrentFile, leading to potential segvio when creating MuDst
 *
 * Revision 1.71  2005/10/06 01:30:30  mvl
 * Changed some of the logic in StMuChainMaker: Now files are no longer opened
 * and checked at the start of the job, but simply added to the TChain. TChain
 * automatically skips corrupted files (this is a new feature).
 *
 * Revision 1.70  2005/08/19 19:46:05  mvl
 * Further updates for multiple vertices. The main changes are:
 * 1) StMudst::primaryTracks() now returns a list (TObjArray*) of tracks
 *    belonging to the 'current' primary vertex. The index number of the
 *    'current' vertex can be set using StMuDst::setCurrentVertex().
 *    This also affects StMuDst::primaryTracks(int i) and
 *    StMuDst::numberOfprimaryTracks().
 * 2) refMult is now stored for all vertices, in StMuPrimaryVertex. The
 *    obvious way to access these numbers is from the StMuprimaryVertex structures,
 *    but for ebakcward compatibility a function is provided in StMuEvent as well
 *    (this is the only function taht works for existing MuDst)
 *
 * As an aside, I've also changes the internals of StMuDst::createStEvent and
 * StMuDst::fixTrackIndices() to be able to deal with a larger range of index numbers for tracks as generated by Ittf.
 *
 * BIG FAT WARNING: StMudst2StEventMaker and StMuDstFilterMaker
 * do not fully support the multiple vertex functionality yet.
 *
 * Revision 1.69  2005/07/15 21:45:08  mvl
 * Added support for multiple primary vertices (StMuPrimaryVertex). Track Dcas are now calculated with repect to the first vertex in the list (highest rank), but another vertex number can be specified. Tarcks also store the index of the vertex they belong to (StMuTrack::vertexIndex())
 *
 * Revision 1.68  2005/04/12 21:56:29  mvl
 * Changes by Xin Dong for year-5 TOF data format: extra TClonesArray and routines to fill it from StEvent (StTofRawData).
 *
 * Revision 1.67  2004/11/29 15:53:22  mvl
 * Additions by Jan for Fpd ezTree
 *
 * Revision 1.66  2004/11/15 18:20:25  mvl
 * Added call to StMuDst::set() for V0-event-pointers in read()
 *
 * Revision 1.65  2004/10/31 23:43:21  mvl
 * Removed some warnings for files without EMC, PMD info.
 * Prevent filling of empty event when no stevent pointer.
 *
 * Revision 1.64  2004/10/28 00:11:33  mvl
 * Added stuff to support ezTree mode of MuDstMaker.
 * This is a special mode for fast-online processing of fast-detector data.
 *
 * Revision 1.63  2004/10/21 02:58:17  mvl
 * Removed some code from Make() (backward compatible EMc mode), to fix StMuIOMaker
 *
 * Revision 1.62  2004/10/19 01:42:29  mvl
 * Changes for splitting Emc and Pmd collections. Emc clusters and points dropped
 *
 * Revision 1.61  2004/09/18 01:28:11  jeromel
 * *** empty log message ***
 *
 * Revision 1.60  2004/05/04 13:26:23  jeromel
 * Oops .. Conflict resolution fixed.
 *
 * Revision 1.59  2004/05/04 13:17:11  jeromel
 * Changed to the documentation in doxygen format
 *
 * Revision 1.58  2004/05/04 00:09:23  perev
 *
 * //  Selecting SetBranchStatus for particular MuDst branches
 * //  Special names:
 * //  MuEventAll - all branches related to StMuEvent
 * //  StrangeAll - all branches related to StrangeMuDst
 * //  EmcAll     - all branches related to Emc
 * //  PmdAll     - all branches related to Pmd
 * //  TofAll     - all branches related to Tof
 * //  By default all branches of MuDst are read. If user wants to read only some of
 * //  them, then:
 * //  SetStatus("*",0)           // all branches off
 * //  SetStatus("MuEventAll",1)  // all standard MuEvent branches ON
 * //  SetStatus("StrangeAll",1)  // all standard Strange branches ON
 * //  SetStatus("EmcAll"    ,1)  // all standard Emc     branches ON
 * //  SetStatus("PmdAll"    ,1)  // all standard Pmd     branches ON
 * //  SetStatus("TofAll"    ,1)  // all standard Tof     branches ON
 * //
 * //  SetStatus("XiAssoc"    ,1) // Strange branch "XiAssoc" is ON
 * //  Names of branches look StMuArrays::arrayTypes[]
 * //  It allows to speed up reading MuDst significantly
 *
 * Revision 1.57  2004/04/26 00:13:28  perev
 * Cleanup+simplification
 *
 * Revision 1.56  2004/04/20 18:42:47  perev
 * remove redundant arrays
 *
 * Revision 1.55  2004/04/15 00:25:49  perev
 * fillHddr() added to fill time stamp ...
 *
 * Revision 1.54  2004/04/14 17:15:56  subhasis
 * Xin's TOF reinclusion
 *
 * Revision 1.53  2004/04/09 22:03:50  subhasis
 * after tof createevent fix by Xin
 *
 * Revision 1.52  2004/04/09 03:36:14  jeromel
 * Removed TOF support entirely for now as we need a working version ... Will
 * revisit later.
 *
 * Revision 1.51  2004/04/06 01:48:09  perev
 * Small leak + incorrect filing StMuTofHitCollection
 *
 * Revision 1.50  2004/04/02 03:24:54  jeromel
 * Changes implements PMD and TOF.  TOF is clearly incomplete.
 *
 * Revision 1.49  2004/02/17 05:05:35  jeromel
 * One more hidden one found post-commit
 *
 * Revision 1.48  2004/02/17 04:56:36  jeromel
 * Extended help, added crs support, restored __GNUC__ for PRETTY_FUNCTION(checked once
 * more and yes, it is ONLY defined in GCC and so is __FUCTION__),  use of a consistent
 * internal __PRETTYF__, return NULL if no case selected (+message) and protected against
 * NULL mChain.
 *
 * Revision 1.47  2003/11/17 22:16:55  perev
 * THack::DeleteClonesArray used for deleting, to avoid ROOT bad features
 *
 * Revision 1.46  2003/11/10 04:07:47  perev
 * again clear improved to avoid leaks
 *
 * Revision 1.45  2003/11/09 01:02:59  perev
 * more sofisticated clear() to fix leaks
 *
 * Revision 1.44  2003/11/03 22:24:45  perev
 * TClones::Clear added into StMuDstMaker::clear to avoid empty ebjects writing
 *
 * Revision 1.43  2003/10/30 20:08:13  perev
 * Check of quality added
 *
 * Revision 1.42  2003/10/28 00:03:46  perev
 * remove some debug lines
 *
 * Revision 1.41  2003/10/27 23:54:33  perev
 * weird template bug fized and templates simplified
 *
 * Revision 1.40  2003/10/23 04:08:29  perev
 * use SetBranchStatus fixed
 *
 * Revision 1.39  2003/10/20 19:50:13  perev
 * workaround added for TClonesArray::Delete + some cleanup of MuEmc
 *
 * Revision 1.38  2003/10/15 17:34:16  laue
 * StMuDstMaker:  Reading fixed. Delete() changed back to Clear()
 * StMuEmcCollection: Re-implemented the DeleteThis() function,
 *                    This hoopefully fixed the memory leak when
 *                    writing MuDst again.
 * StMuTimer: ClassDef/ClassImp
 *
 * Revision 1.37  2003/10/12 03:43:56  perev
 * LeakOff TClonesArray::Clear replaced to Delete
 *
 * Revision 1.36  2003/10/08 21:17:15  laue
 * StMuEmcUtil updates from Alex Suaide
 * StMuDst and StMuDstMaker fixes to take the double inheritance of the
 * StKinkMuDsts into account. A void* had to be introduced when casting
 * TObject* to StKinkMuDst*.
 *
 * Revision 1.35  2003/10/03 15:26:07  laue
 * some moe arrays initialized
 *
 * Revision 1.34  2003/09/28 21:10:59  jeromel
 * More data members zeroed (would cause a crash on exit)
 *
 * Revision 1.33  2003/09/19 01:45:18  jeromel
 * A few problems hopefully fixed i.e. one constructor lacked zeroing
 * emcArrays were not  zeroed, mStMuDst not zeroed.
 * For maintainability zeroArrays() added.
 *
 * Revision 1.32  2003/09/07 03:49:03  perev
 * gcc 3.2 + WarnOff
 *
 * Revision 1.31  2003/09/02 17:58:44  perev
 * gcc 3.2 updates + WarnOff
 *
 * Revision 1.30  2003/08/04 14:38:10  laue
 * Alex Suaide's updated for the EMC. Now EEMC is included.
 *
 * Revision 1.29  2003/03/25 19:08:06  laue
 * added StMuDst into TDataSet so that Valeri can pick it up for his
 * StEventDisplayMaker
 *
 * Revision 1.28  2003/03/06 01:34:18  laue
 * StAddRunInfoMaker is a make helper maker to add the StRunInfo for the
 * only year1 Au+Au 130GeV data
 *
 * Revision 1.27  2003/02/20 15:29:42  laue
 * StMuTriggerIdCollection added
 *
 * Revision 1.26  2003/02/19 15:38:10  jeromel
 * Modifications made to account for the new location of the PIDTable file.
 * The setProbabilityPidFile() method has been modified to take care of a default
 * file loading if unspecified. Messages will be displayed appropriatly.
 * Macros mdoofied to not call the method (leave it handled through the default
 * file).
 *
 * Revision 1.25  2003/02/07 23:47:53  laue
 * New EMC code. TObject arrays replaced by TClonesArrays (thanks to Alex)
 *
 * Revision 1.23  2003/02/05 22:10:00  laue
 * delete emc collection after being copied (when creating mudst)
 *
 * Revision 1.21  2003/01/29 03:04:57  laue
 * !!DIRTY FIX FOR StMuEmcCollection
 * !! Was memor leaking. Leak fixed, but slow and dirty.
 * !! Propose to change the structure as soon as possible.
 *
 * Revision 1.20  2003/01/09 18:59:45  laue
 * initial check in of new EMC classes and the changes required
 *
 * Revision 1.19  2002/11/27 15:07:31  laue
 * fix to run with standard root
 *
 * Revision 1.18  2002/11/07 17:12:22  laue
 * Comment changed.
 *
 * Revision 1.17  2002/08/27 19:05:56  laue
 * Minor updates to make the muDst from simulation work
 *
 * Revision 1.16  2002/08/20 19:55:49  laue
 * Doxygen comments added
 *
 * Revision 1.15  2002/05/20 18:57:18  laue
 * update for Christof
 *
 * Revision 1.14  2002/05/20 17:23:31  laue
 * StStrangeCuts added
 *
 * Revision 1.13  2002/05/04 23:56:30  laue
 * some documentation added
 *
 * Revision 1.12  2002/04/26 21:02:56  jeromel
 * Bug fix in dirname(). Still cannot get the arg3 bla/test.root mechanism to work
 * (but it does neither for everything else). Will come back to it.
 *
 * Revision 1.11  2002/04/23 21:35:32  laue
 * Changed name of StStraMuDstMaker to 'strangeMuDst' so that it can get picked
 * from the bfc.
 *
 * Revision 1.9  2002/04/11 14:19:30  laue
 * - update for RH 7.2
 * - decrease default arrays sizes
 * - add data base readerfor number of events in a file
 *
 * Revision 1.8  2002/04/01 22:42:30  laue
 * improved chain filter options
 *
 * Revision 1.7  2002/03/28 05:10:34  laue
 * update for running in the production
 *
 * Revision 1.6  2002/03/27 03:47:27  laue
 * better filter options
 *
 * Revision 1.5  2002/03/27 00:50:11  laue
 * bux fix from earlier check in
 *
 * Revision 1.3  2002/03/20 16:04:11  laue
 * minor changes, mostly added access functions
 *
 * Revision 1.2  2002/03/08 20:04:31  laue
 * change from two trees to 1 tree per file
 *
 * Revision 1.1  2002/03/08 17:04:17  laue
 * initial revision
 *
 *
 **************************************************************************/


















