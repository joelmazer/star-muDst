//###########################################################
// EMC Micro Event
// Author: Alexandre A. P. Suaide
// initial version 08/2001
//
// See README for details
//########################################################### 
#include <assert.h>

#include "StMuEmcCollection.h"
#include "Stiostream.h"
#include "StMuEmcUtil.h"
static StMuEmcUtil util; // to ease decoding of EEMC hits
ClassImp(StMuEmcCollection)

StMuEmcCollection::StMuEmcCollection()
{    
  for(int i=0;i<7200;i++) mTowerADC[i]=0;
  for(int i=0;i<1080;i++) mEndcapTowerADC[i]=0;
  mEmcPoints=new TClonesArray("StMuEmcPoint",0);
  mPrsHits = new TClonesArray("StMuEmcHit",0);
  mEndcapEmcPoints=new TClonesArray("StMuEmcPoint",0);
  mEndcapPrsHits = new TClonesArray("StMuEmcHit",0);
  for(int i=0;i<4;i++) 
  { 
    if(i<2) 
    {
      mSmdHits[i]=new TClonesArray("StMuEmcHit",0);
      mEndcapSmdHits[i]=new TClonesArray("StMuEmcHit",0);
    }
    mEmcClusters[i]=new TClonesArray("StMuEmcCluster",0);
    mEndcapEmcClusters[i]=new TClonesArray("StMuEmcCluster",0);
  }
}

StMuEmcCollection::StMuEmcCollection(StMuEmcCollection& o) {
  memcpy( mTowerADC,  o.mTowerADC,  sizeof(mTowerADC)  );
  memcpy( mEndcapTowerADC,  o.mEndcapTowerADC,  sizeof(mEndcapTowerADC)  );

  for ( int i=0; i<2; i++) {
    mSmdHits[i] = (TClonesArray*)o.mSmdHits[i]->Clone();
    mEndcapSmdHits[i] = (TClonesArray*)o.mEndcapSmdHits[i]->Clone();
  } 
  for ( int i=0; i<4; i++) {
    mEmcClusters[i] = (TClonesArray*)o.mEmcClusters[i]->Clone();
    mEndcapEmcClusters[i] = (TClonesArray*)o.mEndcapEmcClusters[i]->Clone();
  }
  mPrsHits = (TClonesArray*)o.mPrsHits->Clone();
  mEmcPoints = (TClonesArray*)o.mEmcPoints->Clone(); 
  mEndcapPrsHits = (TClonesArray*)o.mEndcapPrsHits->Clone();
  mEndcapEmcPoints = (TClonesArray*)o.mEndcapEmcPoints->Clone(); 
}

StMuEmcCollection::~StMuEmcCollection()
{
  //clear();
  for(int i=0;i<4;i++) 
  {
    if(i<2) {if(mSmdHits[i]) {mSmdHits[i]->Clear(); delete mSmdHits[i]; mSmdHits[i]=NULL;}}
    if(i<2) {if(mEndcapSmdHits[i]) {mEndcapSmdHits[i]->Clear(); delete mEndcapSmdHits[i]; mEndcapSmdHits[i]=NULL;}}
    if(mEmcClusters[i]) {mEmcClusters[i]->Clear(); delete mEmcClusters[i]; mEmcClusters[i]=NULL;}
    if(mEndcapEmcClusters[i]) {mEndcapEmcClusters[i]->Clear(); delete mEndcapEmcClusters[i]; mEndcapEmcClusters[i]=NULL;}
  } 
  if(mEmcPoints) {mEmcPoints->Clear(); delete mEmcPoints;}
  if(mPrsHits) {mPrsHits->Clear(); delete mPrsHits;}
  if(mEndcapEmcPoints) {mEndcapEmcPoints->Clear(); delete mEndcapEmcPoints;}
  if(mEndcapPrsHits) {mEndcapPrsHits->Clear(); delete mEndcapPrsHits;}
}
void StMuEmcCollection::clear(Option_t *option)
{
/*  for(int i=0;i<7200;i++) mTowerADC[i]=0;
  for(int i=0;i<1080;i++) mEndcapTowerADC[i]=0;
  // deleting points ...
  for(int i=0;i<getNPoints();i++)
  {
    StMuEmcPoint *point = getPoint(i);
    if(point) delete point;
  }
  for(int i=0;i<getNEndcapPoints();i++)
  {
    StMuEmcPoint *point = getEndcapPoint(i);
    if(point) delete point;
  }
  // deleting clusters and hits
  for(int d=0;d<4;d++)
  {
    for(int i=0; i<getNClusters(d);i++)
    {
      StMuEmcCluster* cluster = getCluster(d,i);
      if(cluster) delete cluster;
    }
    for(int i=0; i<getNEndcapClusters(d);i++)
    {
      StMuEmcCluster* cluster = getEndcapCluster(d,i);
      if(cluster) delete cluster;
    }
  }
  for(int d=3;d<5;d++)
  {
    for(int i=0; i<getNSmdHits(d);i++)
    {
      StMuEmcHit* hit = getSmdHit(d,i);
      if(hit) delete hit;
    }
    for(int i=0; i<getNEndcapSmdHits(d+3);i++)
    {
      StMuEmcHit* hit = getEndcapSmdHit(d+3,i);
      if(hit) delete hit;
    }
  }
  for(int i=0; i<getNPrsHits();i++)
  {
    StMuEmcHit* hit = getPrsHit(i);
    if(hit) delete hit;
  }
  for(int i=0; i<getNEndcapPrsHits();i++)
  {
    StMuEmcHit* hit = getEndcapPrsHit(i);
    if(hit) delete hit;
  }
  
  for(int i=0;i<4;i++) mEndcapEmcClusters[i]->Clear();
  for(int i=0;i<2;i++) mEndcapSmdHits[i]->Clear();
  for(int i=0;i<4;i++) mEmcClusters[i]->Clear();
  for(int i=0;i<2;i++) mSmdHits[i]->Clear();
  mPrsHits->Clear();
  mEmcPoints->Clear();
  mEndcapPrsHits->Clear();
  mEndcapEmcPoints->Clear();*/
  return;
}
void StMuEmcCollection::packbits(unsigned char *data, unsigned int value, unsigned int nbits, unsigned int index)
{
  unsigned int start     = index*nbits;
  unsigned int startByte = start/8;
  unsigned int startBit  = start%8;  
  unsigned int a         = 0;
  unsigned int s         = 1;
  for(int i=0;i<4;i++)  { a+=data[startByte+i]*s; s*=256; }
  unsigned int mask = ((unsigned int)(1<<nbits)-1);
  unsigned int b = ((value&mask)<<startBit) | (a&(~(mask<<startBit)));  
  data[startByte+0] = (unsigned char)((b & 0x000000FF));
  data[startByte+1] = (unsigned char)((b & 0x0000FF00)>>8);
  data[startByte+2] = (unsigned char)((b & 0x00FF0000)>>16);
  data[startByte+3] = (unsigned char)((b & 0xFF000000)>>24);
  return;
}
unsigned int StMuEmcCollection::unpackbits(unsigned char *data, unsigned int nbits, unsigned int index)
{
  unsigned int start     = index*nbits;
  unsigned int startByte = start/8;
  unsigned int startBit  = start%8;  
  unsigned int a         = 0;
  unsigned int s         = 1;
  for(int i=0;i<4;i++)  { a+=data[startByte+i]*s; s*=256; }  
  unsigned int mask = ((unsigned int)(1<<nbits)-1);
  unsigned int b = (unsigned int)(a&(mask<<startBit))>>startBit;
  return b;
}
int StMuEmcCollection::getTowerADC(int id, int detector)
{
  if(detector == bemc)
  {
    if(id<1 || id>4800) return 0;
    return (int)unpackbits(mTowerADC,12,(unsigned int)(id-1));
  }
  if(detector == eemc)
  {
    if(id<1 || id>720) return 0;
    return (int)unpackbits(mEndcapTowerADC,12,(unsigned int)(id-1));
  }
  return 0;
}


int StMuEmcCollection::getNSmdHits(int detector)
{
  TClonesArray *tca = NULL;
  if(detector==bsmde || detector==bsmdp) tca = mSmdHits[detector-bsmde];
  if(detector==esmdu || detector==esmdv) tca = mEndcapSmdHits[detector-esmdu];
  if(tca) return tca->GetEntries();
  return 0;
}
StMuEmcHit* StMuEmcCollection::getSmdHit(int hitId,int detector)
{
  TClonesArray *tca = NULL;
  if(detector==bsmde || detector==bsmdp) tca = mSmdHits[detector-bsmde];
  if(detector==esmdu || detector==esmdv) tca = mEndcapSmdHits[detector-esmdu];
  if(tca)
  {
    int counter = tca->GetEntries();
    if(hitId<0 || hitId>counter) return NULL;
    return (StMuEmcHit*)tca->At(hitId);
  }
  return NULL;
}
int StMuEmcCollection::getNPrsHits(int detector)
{
  TClonesArray *tca = NULL;
  if(detector == bprs) tca = mPrsHits;
  if(detector == eprs) tca = mEndcapPrsHits;
  if(tca) return tca->GetEntries();
  return 0;
}
StMuEmcHit* StMuEmcCollection::getPrsHit(int hitId, int detector)
{
  TClonesArray *tca = NULL;
  if(detector == bprs) tca = mPrsHits;
  if(detector == eprs) tca = mEndcapPrsHits;
  if(tca)
  {
    int counter = tca->GetEntries();
    if(hitId<0 || hitId>counter) return NULL;
    return (StMuEmcHit*)tca->At(hitId);
  }
  return NULL;
}
int StMuEmcCollection::getNClusters(int detector)
{
  if(detector<bemc && detector>esmdv) return 0;
  TClonesArray *tca =NULL;
  if(detector>=bemc && detector <= bsmdp) tca = mEmcClusters[detector-bemc];
  else tca = mEndcapEmcClusters[detector-eemc];
  return tca->GetEntries();
}
StMuEmcCluster* StMuEmcCollection::getCluster(int clusterId,int detector)
{
  if(detector<bemc && detector>esmdv) return NULL;
  TClonesArray *tca = NULL;
  if(detector>=bemc && detector <= bsmdp) tca = mEmcClusters[detector-bemc];
  else tca = mEndcapEmcClusters[detector-eemc];
  int counter = tca->GetEntries();
  if(clusterId<0 || clusterId>counter) return NULL;
  return (StMuEmcCluster*)tca->At(clusterId);
}
int StMuEmcCollection::getNPoints()
{
  TClonesArray *tca =mEmcPoints;
  return tca->GetEntries();
}
int StMuEmcCollection::getNEndcapPoints()
{
  TClonesArray *tca =mEndcapEmcPoints;
  return tca->GetEntries();
}
StMuEmcPoint* StMuEmcCollection::getPoint(int pointId)
{
  TClonesArray *tca =mEmcPoints;
  int counter = tca->GetEntries();
  if(pointId<0 || pointId>counter) return NULL;
  return (StMuEmcPoint*)tca->At(pointId);
}
StMuEmcPoint* StMuEmcCollection::getEndcapPoint(int pointId)
{
  TClonesArray *tca =mEndcapEmcPoints;
  int counter = tca->GetEntries();
  if(pointId<0 || pointId>counter) return NULL;
  return (StMuEmcPoint*)tca->At(pointId);
}



void StMuEmcCollection::setTowerADC(int id,int adc, int detector)
{
  if(detector == bemc)
  {
    if(id<1 || id>4800) return;
    packbits(mTowerADC,adc,12,(unsigned int)(id-1));
  }
  if(detector == eemc)
  {
    if(id<1 || id>720) return;
    packbits(mEndcapTowerADC,adc,12,(unsigned int)(id-1));
  }
  return;
}
void StMuEmcCollection::addSmdHit(int detector)
{
  TClonesArray *tca = NULL;
  if(detector==bsmde || detector==bsmdp) tca = mSmdHits[detector-bsmde];
  if(detector==esmdu || detector==esmdv) tca = mEndcapSmdHits[detector-esmdu];
  if(tca) 
  {
    int counter = tca->GetEntries();
    new ((*tca)[counter]) StMuEmcHit();   
  }
  return;
}
void StMuEmcCollection::addPrsHit(int detector)
{
  TClonesArray *tca = NULL;
  if(detector == bprs) tca = mPrsHits;
  if(detector == eprs) tca = mEndcapPrsHits;
  if(tca)
  {
    int counter = tca->GetEntries();
    new ((*tca)[counter]) StMuEmcHit();
  }
  return;
}
void StMuEmcCollection::addCluster(int detector)
{
  if(detector<bemc && detector>esmdv) return;
  TClonesArray *tca =NULL;
  if(detector>=bemc && detector <= bsmdp) tca = mEmcClusters[detector-bemc];
  else tca = mEndcapEmcClusters[detector-eemc];
  int counter = tca->GetEntries();
  new ((*tca)[counter]) StMuEmcCluster();
  return;
}
void StMuEmcCollection::addPoint()
{
  TClonesArray *tca =mEmcPoints;
  int counter = tca->GetEntries();
  new ((*tca)[counter]) StMuEmcPoint();
  return;
}
void StMuEmcCollection::addEndcapPoint()
{
  TClonesArray *tca =mEndcapEmcPoints;
  int counter = tca->GetEntries();
  new ((*tca)[counter]) StMuEmcPoint();
  return;
}


void StMuEmcCollection
::getEndcapTowerADC(int ihit, int &adc, int &isec, int &isub, int & ieta)
{
  adc=getTowerADC(ihit,eemc);
  if(adc<=0) {
    isec=isub=ieta=-1;
    return;
  }
  util.getEndcapBin(eemc,ihit,isec,ieta,isub);
  return ;
}


StMuEmcHit * StMuEmcCollection
::getEndcapPrsHit(int ihit, int &isec, int &isub, int & ieta, int &ipre)
{
  StMuEmcHit * h =  getPrsHit(ihit,eprs);
  util.getEndcapBin(eprs,h->getId(),isec,ieta,isub);
  ipre=isub/5;
  isub%=5;
  return h;
}


int StMuEmcCollection::getNEndcapSmdHits(char uv)
{
  assert(uv=='U' || uv=='V');
  return getNSmdHits(esmdu+uv-'U');
}


StMuEmcHit * StMuEmcCollection
::getEndcapSmdHit(char uv, int ihit,int &isec, int &istrip)
{
  assert(uv=='U' || uv=='V');
  int det=esmdu+uv-'U';
  StMuEmcHit * h =getSmdHit(ihit,det);  
  int idum;
  util.getEndcapBin(det,h->getId(),isec,istrip,idum);
  return h;
}
