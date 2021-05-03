#include <iostream>
#include <TMath.h>
using namespace std;

// constants for the fluctuations of noise power
// either with kbT or hf
const double kB=1.380649e-23; // in J*K-1
const double h = 6.626e-34; // in J*s

// signal power
const double hbarc= 197.327e-9; // in eV m
const double alpha= 1.0/137.036;
const double lambda = 78e6; // in eV
const double rhoDM = 0.45e15; // in eV/m^3
const double pi= TMath::Pi();
const double inverse_mu0pi = 0.5e7;
const double jouelToeV=1/1.6e-19;

//compute gAgammagamma
void computegAgg(const double ma,                 // in eV
		 const double gGamma=-0.97)       // KSVZ
{
  double gAgammagamma = fabs(gGamma)*alpha/pi/lambda/lambda*ma; // in eV-1

  gAgammagamma *= 1e9;

  cout << "gAgammagamma for ma= " << ma*1e6 << " micro eV and gGamma = "
       << gGamma <<  " is "
       << gAgammagamma << " GeV-1" << endl;

  cout << "KSVZ gAgammagamma for ma= " << ma*1e6 << " micro eV is "
       << 0.39*ma*1e-9 << " GeV-1" << endl;

  cout << "DFSZ gAgammagamma for ma= " << ma*1e6 << " micro eV is "
       << (0.203*8./3.-0.39)*ma*1e-9 << " GeV-1" << endl;

}


// limit on gAgammagamma
double computePs(const double f0=5e9,             // in Hz
		 const double beta=1,
		 const double B=9,                // in Tesla
		 const double V=1e-3,             // in m^3
		 const double Cmnl=0.5,            
		 const double QL=50000,
		 const double gGamma=-0.97        // KSVZ
		 )

{

  // here, compute fluctuation of noise power to be one photon energy

  double Psig =pow(gGamma*alpha/pi/lambda/lambda,2)
    *pow(hbarc,3)*rhoDM*beta/(1+beta)*f0*inverse_mu0pi
    *B*B*V*Cmnl*QL;

  cout << "signal power = " << Psig << endl;
  return Psig;
  
}


void computeLimit(const double significance=1.645,
 		  const double f0=5e9,             // in Hz
 		  const double Tsys=-1,          // in K
 		  const double beta=1,
 		  const double B=9,                // in Tesla
 		  const double V=1e-3,             // in m^3
 		  const double Cmnl=0.5,            
 		  const double QL=50000,
 		  const double bandwidth=5e4,      // in Hz
 		  const double intT=3600,
		  const int nspec=0)          // in seconds
 {

   // here, compute fluctuation of noise power to be one photon energy

   double noise_power = Tsys>0? kB*Tsys: h*f0;
   double sigmaN=noise_power*(nspec>0? bandwidth/sqrt(nspec):
			      sqrt(bandwidth/intT)); // in eV

   cout << "sigmaN = " << sigmaN << endl;

   double upperLimit_power = sigmaN*significance;

   double ma = h*f0*jouelToeV;
   cout << "mass of axion is " << ma << " eV" << endl;
   
   
   double factor_signal_power = 1/pow(ma,2)
     *pow(hbarc,3)*rhoDM*beta/(1+beta)*f0*inverse_mu0pi
     *B*B*V*Cmnl*QL;

   double gAgammagamma = sqrt(upperLimit_power/factor_signal_power);
   // in eV-1

   cout << "Upper limit on gAgammagamma = " << gAgammagamma*1e9 <<
     " GeV-1" << endl;
 }



void computeSNR(
		const double gGamma=-0.97,        // KSVZ
		const double f0=5e9,             // in Hz
		const double Tsys=-1,          // in K
		const double beta=1,
		const double B=9,                // in Tesla
		const double V=1e-3,             // in m^3
		const double Cmnl=0.5,            
		const double QL=50000,
		const double bandwidth=5e4,      // in Hz
		const double intT=3600,
		const int nspec=0)          // in seconds
 {

   // here, compute fluctuation of noise power to be one photon energy

   double noise_power = Tsys>0? kB*Tsys: h*f0;
   double sigmaN=noise_power*(nspec>0? bandwidth/sqrt(nspec):
			      sqrt(bandwidth/intT)); // in eV

   cout << "sigmaN = " << sigmaN << endl;

   double signal_power = computePs(f0,
				   beta,
				   B,
				   V,
				   Cmnl,            
				   QL,
				   gGamma        
				   );
   
   cout << "Signal-to-Noise ratio = " << signal_power/noise_power << endl;
   
 }