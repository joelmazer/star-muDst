/***************************************************************************
 *
 * $Id: StMuChainMaker.cxx,v 1.11 2002/12/19 19:44:25 laue Exp $
 * Author: Frank Laue, BNL, laue@bnl.gov
 *
 **************************************************************************/
#include <fstream>

#include "StMuException.hh"
#include "StMuDebug.h"
#include "StMuChainMaker.h"
#include "StMuDbReader.h"
#include "StMuTimer.h"

#include "StMaker.h"
#include "StChain.h"

#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TSystem.h"

extern TSystem* gSystem;

ClassImp(StMuChainMaker)
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   Constructor: The argument 'name' is the name of the TTrees be chained   
*/
StMuChainMaker::StMuChainMaker(const char* name) : mTreeName(name) {
  DEBUGMESSAGE2("");
  mChain = new TChain(mTreeName.c_str());
  mChain->SetDirectory(0);
  mDbReader = StMuDbReader::instance();
  mFileCounter=0;
  //  mSubFilters = new string[100];
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/** 
    Destructor: The TChain will not be deleted since it is passed to the 
    outside.
 */
StMuChainMaker::~StMuChainMaker() {
  DEBUGMESSAGE2("");
  //  delete []mSubFilters;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/** 
    Returns a full filename, simply concats the three arguments 'dir', 
    'fileName' and extention
*/
string StMuChainMaker::buildFileName(string dir, string fileName, string extention){
  DEBUGMESSAGE2("");
  fileName = dir + fileName + extention;
  return fileName;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   Return the input string's basename by stripping of all characters from 
   the first '.' to the end and all characters after the last '/'.
 */
string StMuChainMaker::basename(string s){
  string name(s);
  size_t pos;
  pos = name.find_last_of("/");
  if (pos!=string::npos ) name.erase(0, pos+1 );
  pos = name.find_first_of(".");
  if (pos!=string::npos ) name.erase(pos,name.length() );
  DEBUGVALUE2(name);
  return name;
} 
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   Return a inputs string's directory name by erasing the basename() and
   all charcters after the last '/'. If the only remaining character is 
   '/' and empty string "" will be returned.
 */
string StMuChainMaker::dirname(string s){
  string name(s);
  string base(basename(s));
  name.erase(name.find(base),base.length());
  size_t pos;
  pos = name.find_last_of("/");
  if (pos!=string::npos ) name.erase(pos, name.length());
  if (name=="/") name = "";
  DEBUGVALUE2(name);
  return name;
} 
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/**
   Parses the input strings. Multiple sub-filters will be built our of 
   'filter'. Here, ":" separates the individual filter strings (e.g.
   "MuDst:st_physics_2:raw_0001" will accept only files which have all of 
   the sub-strings "MuDst", "st_physics_2" and "raw_0001" in them.

   If 'file' is empty, the directory 'dir' will be scanned for files.
   If 'file' has a substring "MuDst.root" a single file will be opened.
   If 'file' has a substring ".lis" the file will be expected to be a list.
   In the case 'file' is not empty, 'dir' will be ignored, hence the filenames
   provided have to be full filenames (including path)

   A TChain will be built for files matching the sub filters (in all cases).
   The chain will be returned.
  
   
 */
TChain* StMuChainMaker::make(string dir, string file, string filter, int maxFiles) {
  DEBUGMESSAGE1("");
  subFilter(filter);

  //  for (int i=0; i<1e6; i++) double ii=pow(2,i);

  if (file!="") {
    if (file.find(".lis")!=string::npos) {
      if (file.find(".list")!=string::npos) 
	fromFileCatalog(file, maxFiles);
      else
	fromList(file, maxFiles);
    }
    if (file.find(".files")!=string::npos) fromList(file, maxFiles);
    if (file.find(".MuDst.root")!=string::npos) fromFile(file, maxFiles);
  }
  else {
    fromDir(dir, maxFiles);
  }
  DEBUGMESSAGE2("return");
  return mChain;
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuChainMaker::subFilter(string filter) {
  DEBUGMESSAGE2("");
  string tmp(filter);
  int n=0;
  size_t pos=0;
  while (tmp.find_first_of(":")!=string::npos ) {
    pos = tmp.find_first_of(":");
    mSubFilters[n] = string( tmp.substr(0,pos) );
    tmp.erase(0,pos+1);
    n++;
  }				
  mSubFilters[n] = string("endOfFilters");
  DEBUGMESSAGE2("return");
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuChainMaker::add(string file) {
  DEBUGMESSAGE3("");
  /// if no entries in db, just add file
  DEBUGVALUE2(file.c_str());

  int entries = 0;

  // read number of events in file from db
  entries = mDbReader->entries(file.c_str());

  // if I can not read the number of events from db, open file and read number.
  if (entries==0) {
    TFile f1(file.c_str());
    TTree *tree = (TTree*)dynamic_cast<TTree*>(f1.Get("MuDst"));
    if (tree) entries = (int)tree->GetEntries();
    f1.Close();
  } 

  // add to chain if #events > 0
  if (entries) {
    mChain->Add( file.c_str(), entries );
    mFileCounter++;
  }
    
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuChainMaker::fromDir(string dir, int maxFiles) {
  DEBUGMESSAGE2("");
  DEBUGVALUE(gSystem);

  void *pDir = gSystem->OpenDirectory(dir.c_str());
  // now find the files that end in the specified extention
  const char* fileName(0);
  while((fileName = gSystem->GetDirEntry(pDir))){
    bool good = true;
    string name(fileName);
    if( strcmp(fileName,".")==0 || strcmp(fileName,"..")==0) good=false;
    if( strcmp(fileName,".root")==0 || strcmp(fileName,"..")==0) good=false;
    if ( name.find(".MuDst.root")==string::npos ) good=false;
    if ( good && pass(name,mSubFilters) ) {
      char* fullFile = gSystem->ConcatFileName(dir.c_str(),fileName);
      // add it to the chain
      add( fullFile );
      delete []fullFile;
    }
    if(mFileCounter >= maxFiles) break;
  }   
  DEBUGVALUE2(mFileCounter);
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
#include "TSQLServer.h"
#include "TSQLResult.h"
#include "TSQLRow.h"
void StMuChainMaker::fromFileCatalog(string list, int maxFiles) { ///< no yet implemented
  DEBUGMESSAGE2("");
  TSQLServer* server = TSQLServer::Connect("mysql://duvall.star.bnl.gov:3306/FileCatalog","","");
  if ( !server ) DEBUGMESSAGE("could not connect to server");

  /// get machine name 
  string machine(gSystem->Getenv("HOST"));
  if (machine.find(".rcf.bnl.gov")==string::npos) machine += ".rcf.bnl.gov";

  /// read list of files
  string files("(filename='dummy')");
  ifstream in(list.c_str());
  if (!in) {
    DEBUGMESSAGE("can not open file");
    DEBUGVALUE(list);
    return;
  }
  char line[512];
  while ( in.getline(line,511) ) {
    string full(line);
    int split = full.rfind("/");
    string name = full.substr(split+1);
    string path = full.substr(0,split);
    cout << path << " " << name << endl;
    if (path.find("/star/data/")==0) // if it's a local disk, then add the machine name to the query
      files += " || (filePath='" + path + "'&&fileName='" + name +"')";
    else
      files += " || (filePath='" + path + "'&&fileName='" + name + "'&&nodeName='" + machine + "')";
  }
  in.close();
	  
  cout << files << endl;

  // now query the database
  StMuTimer timer;
  timer.stop();
  timer.reset();
  string query = "SELECT filePath,fileName,numEntries FROM FileData LEFT JOIN FileLocations USING (fileDataId) WHERE " + files;
  TSQLResult* result = server->Query(query.c_str());
  timer.stop();
  
  DEBUGVALUE(timer.elapsedTime());
  DEBUGVALUE(result->GetRowCount());
  
  // now fill chain with query results
  TSQLRow* row;
  string file;
  int entries;
  while ( (row=result->Next()) ) {
    file = row->GetField(0); 
    file += +"/";
    file += row->GetField(1);
    entries = atoi(row->GetField(2));
    mChain->Add( file.c_str(), entries );
    mFileCounter++;
  }

  server->Close();
  delete server;
  DEBUGVALUE2(mFileCounter);
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuChainMaker::fromList(string list, int maxFiles) {
  DEBUGMESSAGE2("");
  ifstream* inputStream = new ifstream;
  inputStream->open(list.c_str());
  if (!(inputStream)) {
    DEBUGMESSAGE("can not open list file");
  }
  char* temp;
  DEBUGVALUE(inputStream->good());
  for (;inputStream->good();) {
    temp = new char[200];
    inputStream->getline(temp,200);
    if ( pass(temp,mSubFilters) ) {
      add(temp);
    }
    delete []temp;
    if (mFileCounter>=maxFiles) break;
  }   
  delete inputStream;
  DEBUGVALUE2(mFileCounter);
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void StMuChainMaker::fromFile(string file, int maxFiles) {
  DEBUGMESSAGE2("");
  DEBUGMESSAGE2(mTreeName.c_str());
  add( file );
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
bool StMuChainMaker::pass(string file, string* filters) {
  bool good = true;
  int n=0;
  while (filters[n].find("endOfFilters")==string::npos  && good) {
    if ( file.find(filters[n])==string::npos ) good=false;
    n++;
  }
  return good;
}
/***************************************************************************
 *
 * $Log: StMuChainMaker.cxx,v $
 * Revision 1.11  2002/12/19 19:44:25  laue
 * update to read number of events from database, for files ending with .list
 *
 * Revision 1.10  2002/11/27 20:37:02  laue
 * output removed
 *
 * Revision 1.9  2002/11/18 14:29:31  laue
 * update for Yuri's new StProbPidTraits
 *
 * Revision 1.8  2002/10/01 23:46:13  laue
 * setting all unused subFilters explicitly to NULL
 *
 * Revision 1.7  2002/08/27 21:20:07  laue
 * Fei Du's request
 * fileCouter>maxFiles changed to fileCounter>=maxFiles
 * Now maxFiles and not maxFiles+1 are added to the list
 *
 * Revision 1.6  2002/08/20 19:55:48  laue
 * Doxygen comments added
 *
 * Revision 1.5  2002/05/04 23:56:29  laue
 * some documentation added
 *
 * Revision 1.4  2002/04/17 21:04:15  laue
 * minor updates
 *
 * Revision 1.3  2002/04/15 22:18:15  laue
 * bug fix in reading of single file
 *
 * Revision 1.2  2002/04/11 14:19:30  laue
 * - update for RH 7.2
 * - decrease default arrays sizes
 * - add data base readerfor number of events in a file
 *
 * Revision 1.1  2002/04/01 22:42:29  laue
 * improved chain filter options
 *
 *
 **************************************************************************/


















