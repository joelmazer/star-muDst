#include <iostream>
#include <string>
#include <typeinfo>

#ifndef StMuException_hh
#define StMuException_hh

#define PF __PRETTY_FUNCTION__

#define THROW(key,text) StMuException##key(text,__PRETTY_FUNCTION__)
#define EXE(x) class StMuException##x : public StMuException { public: StMuException##x (const char* m="", const char* in="???") : StMuException(k##x, m, in) { /* no-op */ } };
		       

enum StMuExceptionTypes {kUnknown=0, kNullPointer, kBadFlag, kBadValue, kEOF};
/** 
    @class StMuException
    Just a small helper class (and a few macros) to easily create a set of exceptions.
    Using the "THROW(...)" macro the exceptions datamember mIn will hold the name of the scope that was throwing the exception.
*/
class StMuException {
protected:
  StMuExceptionTypes mException;
  string mMessage;
  string mIn;
public:
  StMuException(StMuExceptionTypes=kUnknown, const char* m="", const char* in="???") : mMessage(m), mIn(in) { /* no-op */ }
  virtual ~StMuException() {}
  virtual string message() {return mMessage; }
  virtual void print() { cout << "*** StMuException #" <<  (unsigned long)mException << " *** " << mIn << " *** " << message() << " ***" << endl; }
  virtual StMuExceptionTypes type() { return mException; }
};

EXE(Unknown);
EXE(NullPointer);
EXE(BadValue);
EXE(BadFlag);
EXE(EOF);





#endif
