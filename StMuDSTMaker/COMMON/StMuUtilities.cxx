/***********************************************************************
 *
 * $Id: StMuUtilities.h,v 1.0 1999/09/07
 * Authors: Frank Laue, BNL, laue@bnl.gov
 *
 ***********************************************************************/

#include "StMuDSTMaker/COMMON/StMuUtilities.h"

#include "TCernLib.h"


namespace StMuUtilities {


/**
 * Creates a new vector of track parameters M with error matrix C by
 * constraining the track with the vertex Y +/- B. The original track
 * parameters are assumed to be given at the DCA to the vertex.
 */
double joinVtx(const double Y[3], const double B[6],
               const double X[6], const double A[21],
                     double M[6],       double C[21])
{
   // Y : x,y,z of vertex.      B: error matrix of Y
   // X : track parameters.     A: error matrix of X
   // M : track parameters of X+Y.      C: error matrix of M


   enum {nP1 = 3, nE1 = 6, nP2 = 6, nE2 = 21};

   double Ai[21];
   memcpy(Ai, A, 21*sizeof(double));

   Ai[0] = 1;
   TCL::trsinv(Ai, Ai, nP2);
   Ai[0] = 0;

   double Ai11i[6], Ai10[3][3], T10[3][3], dif[6], m[6];

   // Ai.get11(Ai11i);
   memcpy(Ai11i+0, Ai+ 9, 1*sizeof(double));
   memcpy(Ai11i+1, Ai+13, 2*sizeof(double));
   memcpy(Ai11i+3, Ai+18, 3*sizeof(double));

   // Ai.get10(Ai10[0]);
   memcpy(Ai10[0]+0, Ai+ 6, 3*sizeof(double));
   memcpy(Ai10[0]+3, Ai+10, 3*sizeof(double));
   memcpy(Ai10[0]+6, Ai+15, 3*sizeof(double));

   TCL::trsinv(Ai11i , Ai11i, 3);
   TCL::trsa  (Ai11i, Ai10[0], T10[0], 3, 3);   //Ai11*Ai10
   TCL::ucopy(Y, m, 3);
   TCL::vsub (X, Y, dif, 3);
   TCL::mxmpy(T10[0], dif, m + 3, 3, 3, 1);
   TCL::vadd(X + 3, m + 3, m + 3, 3);                   //m:   resulting params

   if (M) {TCL::ucopy(m, M, nP2); }     //fill resulting params

   TCL::vsub(X, m, dif, nP2);                   //dif = X - M
   double chi2;
   TCL::trasat(dif, Ai, &chi2, 1, nP2);         //calc chi2

   if (!C) return chi2;

   // Error matrix calculation
   memset(C, 0, 21*sizeof(double));

   double TX[nP1][nP2]; memset(TX[0], 0, sizeof(TX));

   for (int i = 0; i < 3; i++) {TCL::ucopy(T10[i], TX[i], 3); TX[i][i + 3] = 1;}

   double C11[nE1];
   TCL::trasat(TX[0], A, C11, nP1, nP2);

   //C->set11(C11);
   memcpy(C+ 9, C11+0, 1*sizeof(double));
   memcpy(C+13, C11+1, 2*sizeof(double));
   memcpy(C+18, C11+3, 3*sizeof(double));

   double TY[nP2][nP1]; memset(TY[0], 0, sizeof(TX));

   for (int i = 0; i < 3; i++) {TCL::vcopyn(T10[i], TY[i + 3], 3); TY[i][i] = 1;}

   TY[0][0] = 0;
   double CYY[nE2];
   TCL::trasat(TY[0], B, CYY, nP2, nP1);
   // C = CYY + C
   TCL::vadd(CYY, C, C, nE2);
   return chi2;
}


}

/***********************************************************************
 *
 * $Log: StMuUtilities.cxx,v $
 * Revision 1.1  2002/03/08 17:04:18  laue
 * initial revision
 *
 *
 ***********************************************************************/
