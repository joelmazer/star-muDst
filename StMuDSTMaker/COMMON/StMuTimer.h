/***************************************************************************
 *
 * $Id: StMuTimer.h,v 1.2 2003/09/09 18:16:54 laue Exp $
 *
 * Author: Thomas Ullrich, April 1999
 ***************************************************************************
 *
 * Description:  CPU timer
 *
 ***************************************************************************
 *
 * $Log: StMuTimer.h,v $
 * Revision 1.2  2003/09/09 18:16:54  laue
 * StMuIOMaker: embedded documentation added
 * StMuTimer: name of define changed (was same as StTimer)
 *
 * Revision 1.1  2002/03/08 17:04:18  laue
 * initial revision
 *
 * Revision 1.1  1999/04/27 19:23:46  ullrich
 * Initial Revision
 *
 **************************************************************************/
#ifndef StMuTimer_h
#define StMuTimer_h

#include "TObject.h"

class StMuTimer : public TObject {
public:
    StMuTimer();
    
    double        elapsedTime() const;
    double        resolution() const;
    void          reset();
    void          start();
    void          stop();
    
private:
    double        mStartTime;
    double        mStopTime;
    int           mIsStopped;
    
    static double absoluteTime();

//    ClassDef(StMuTimer,0)
};

#endif
