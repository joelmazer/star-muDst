/***************************************************************************
 *
 * $Id: StMuIOMaker.h,v 1.2 2003/09/09 18:16:53 laue Exp $
 * Author: Frank Laue, BNL, laue@bnl.gov
 ***************************************************************************/
#ifndef StMuIOMaker_hh
#define StMuIOMaker_hh

#include <string>

#ifndef ST_NO_NAMESPACES
using namespace std;
#endif

#include "StIOInterFace.h"
#include "St_DataSetIter.h"

#include "StMuArrays.h"

class StMuEvent;
class StMuDst;
class StMuCut;

class StEvent;
class StTrackNode;
class StTrack;
class StRichSpectra;
class StDetectorState;
class StL3AlgorithmInfo;

class StuProbabilityPidAlgorithm;

class StIOMaker;
class StTreeMaker;

/// strangeness group stuff
class StStrangeEvMuDst;
class StStrangeMuDstMaker;
class StV0MuDst;
class StV0Mc; 
class StXiMuDst;
class StXiMc;
class StKinkMuDst;
class StKinkMc;
class StStrangeAssoc;
class StStrangeCuts;

/// emc stuff
#include "StMuEmcCollection.h"
class StMuEmcUtil;

class TFile;
class TTree;
class TChain;
class TClonesArray;

/**
   @class StMuIOMaker
   Class to create and read STAR's common micro dst (StMuDst)
   
   This class is a true maker in the STAR sense. It inherits from "StMaker" and implements the functions "int Init()", "void Clear()",
   int Make()", and "int Finish()" in order to run as part of an "StChain". Please refer to the STAR Computing Web pages in case you do not 
   know what "StMaker" and "StChain" mean.
   
*/
class StMuIOMaker : public StIOInterFace {
 public:
    /// Default constructor
    StMuIOMaker(const char* name="MuDst");
    ~StMuIOMaker();
    
  int Init();
  void Clear();
  int Make();
  int Make(int index);
  int Make(int major, int minor);
  int Make(const StUKey &RunEvent);

  int MakeRead() {return Make(); }
  int MakeRead(const StUKey &RunEvent) { return Make(RunEvent); }
  int Skip(int nskip) { return Make(mCurrentIndex+nskip); }

  int Finish();
  void  SetFileName(const char *fileName);
  void  SetFile(const char *fileName);

  StMuDst* muDst(); ///< return pointer the  current (last read) StMuDst
  StMuEmcUtil* muEmcUtil() { return mEmcUtil; } ///< return pointer to StMuEmcUtil;
  int currentIndex() { return mCurrentIndex; }
  int eventCounter() { return mEventCounter; }
  int numberOfEvents() { return mNumberOfEvents; }
  virtual const char *GetCVS() const {  ///< Returns version tag.

    static const char cvs[]="Tag $Name:  $ $Id: StMuIOMaker.h,v 1.2 2003/09/09 18:16:53 laue Exp $ built "__DATE__" "__TIME__ ; 
    return cvs;
  }


private:
  StMuDst* mStMuDst;
  StMuEmcUtil* mEmcUtil;

  TChain* mChain;
int mNumberOfEvents; ///< holds the number of events in the current chain (file)
int mCurrentIndex;  ///< holds the index of the last event read
int mEventCounter; ///< a running counter holding the total number of events read
  
  template<class T> 
  void saveDelete(T* t) { if (t!=0) delete t; t=0;}    

  void openRead();
  void setBranchAddresses();
  void closeRead();

  void setBranchAddresses(TChain*);

  void clear(TClonesArray* t, int& counter);
  void del(TClonesArray* t, int& counter);
  void clear();

  void createArrays();
  TClonesArray* clonesArray(TClonesArray*& p, const char* type, int size, int& counter);

  string basename(string);
  string dirname(string);
  string buildFileName(string dir, string fileName, string extention);
 
  friend class StMuDst;
  friend class StMuDstFilterMaker;
    
  TClonesArray* arrays[__NARRAYS__];//->
  TClonesArray* mArrays[__NARRAYS__];//->

  TClonesArray* strangeArrays[__NSTRANGEARRAYS__];//->
  TClonesArray* mStrangeArrays[__NSTRANGEARRAYS__];//->

  TClonesArray* emcArrays[__NEMCARRAYS__];//->
  TClonesArray* mEmcArrays[__NEMCARRAYS__];//->


  ClassDef(StMuIOMaker, 1)
}; 

inline StMuDst* StMuIOMaker::muDst() { return mStMuDst;}

#endif

/***************************************************************************
 *
 * $Log: StMuIOMaker.h,v $
 * Revision 1.2  2003/09/09 18:16:53  laue
 * StMuIOMaker: embedded documentation added
 * StMuTimer: name of define changed (was same as StTimer)
 *
 **************************************************************************/