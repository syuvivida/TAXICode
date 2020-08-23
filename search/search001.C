#include <TRandom3.h>
#include <TSystem.h>
#include <TProfile.h>
#include <TFile.h>
#include <TMath.h>
#include <TH1.h>
#include <TF1.h>
#include <iostream>
#include <string>
#include <fstream>

// a program to search for axion using numbers of Ed Daw
// assuming a maxwell-boltzmann distribution for signal
// assume the frequency bin width is narrower than the signal

using namespace std;

// Pout_original = 1.52e-21 W [ V/220L * B/7.6T * B/7.6T* Cnlm * g_gamma/0.97*g_gamma/0.97 * rhoa /0.45 * f0/750MHz * Q/70000]
// corr Pout =
// Pout_original * (1-2S11)/(1-S11)/(1+4Q^2(f/f0-1)^2)] *eta

const double c = 3e5; // in km/s
const double eta = 1;
const double baseline = 3.0; // (*1e-22)
const double QL = 70000;
const double S11 = 0;
const double Tsys = 5.6; // in Kelvin
const double bandwidth = 125e-6; //  in MHz, 125 Hz
const double kB = 1.38e-1; // ( *1e-22)
//const double integration_time = 80; // in seconds
const int N_integral = 10000;
const double sigma_noise = kB*Tsys*bandwidth*1e6/sqrt(N_integral); // for each bin
const double rangeSpec = 50e-3; // 50 kHz
const int nBins = rangeSpec/bandwidth;
const double step_size = 2e-3; // in MHz, 2 kHz

#ifndef FATAL
#define FATAL(msg) do { fprintf(stderr, "FATAL: %s\n", msg); gSystem->Exit(1); } while (0)
#endif


// frequency in MHz
double P_exp(double* x, double* par)
{
  double f_vary = x[0];
  double f0 = par[0];
  double Power =  baseline * (f0/750.) * (1-2*S11)/(1-S11) *
    (1/(1+4*QL*QL*pow(f_vary/f0-1,2))) * eta;
  return Power;
}



double maxwell_boltzmann(double x)
{
  double v = 226;
  double func = 4*TMath::Pi()*x*x/pow(v*v*TMath::Pi(),1.5)*TMath::Exp(-x*x/v/v);
  return func;
}


// frequency discussed here is in GHz, and the power is expressed
// in terms of noise power kB*T*bandwidth
// 
// 1. Generate fake data that contains an axion signal with some
// random frequency between 4.5 and 5.5 GHz
// 2. Overlay the signal with background from thermal noise
// 3. Sum of signal and background is the measured spectrum
// 4. Weight each spectrum
// 5. Add to get signal/noise ratio and see if we could find a signal

void search001(const double lo = 749, const double hi = 751,
	       bool narrow=false,
	       const unsigned int nTrials=1, bool debug=false)
{

  if(S11 > 1 || fabs(S11-1)<1e-6 || S11 < 0) FATAL("S11 should be between 0 and 1");

  double freq_lo = lo - 0.5*rangeSpec;
  double freq_hi = hi + 0.5*rangeSpec;
  const unsigned int nSteps = (hi-lo)/step_size;

  cout << "Preparing a study with " << nTrials << " trials and ";
  cout << nSteps << " steps of frequency changes" << endl;
  cout << "sigma of noise is " << sigma_noise << endl;
  cout << "Grand spectrum frequency range is " <<
    freq_lo << " -- " << freq_hi << " MHz" << endl;
  
  TF1* fsig = new TF1("fsig", P_exp, freq_lo, freq_hi, 1);
  fsig->SetNpx(2500);

  TF1* fspeed = new TF1("fspeed","maxwell_boltzmann(x)",0,10000);

  TRandom3* gRandom= new TRandom3();
  const int nTotalBins = (freq_hi-freq_lo)/bandwidth;
  TH1F* hGrand = new TH1F("hGrand","Grand spectrum",
			  nTotalBins, freq_lo, freq_hi);

  TH1F* hsig[nTrials][nSteps];
  TH1F* hbkg[nTrials][nSteps];
  TH1F* hmea[nTrials][nSteps];

  // writing example output file
  TFile* outFile = new TFile("sim_search001.root","recreate");
  
  for(unsigned int itrial=0; itrial < nTrials; itrial++){

    // first random peak a signal frequency
    double f_axion = lo + (hi-lo)*gRandom->Rndm();
    if(debug)
      cout << "search for a signal with frequency = " << f_axion
		<< " MHz" << endl;
    // now start stepping frequency in size of step_size

    double start_freq;
    double end_freq;
    unsigned int nSpectra=0;
    int startStepBin=-1;
    
    for(unsigned int istep=0; istep<nSteps; istep++){
    //    for(unsigned int istep=0; nSpectra<2; istep++){

      start_freq = freq_lo + istep*step_size;
      end_freq   = start_freq + rangeSpec;

      if(debug)
	cout << "start:end frequencies = " << start_freq << "\t"
	     << end_freq << endl;

      TH1F* htemp = new TH1F("htemp","template of frequency",
			     nBins, start_freq, end_freq);
      htemp->SetXTitle("Frequency [MHz]");
      htemp->SetYTitle("Power [10^{-22} W]");

      hsig[itrial][istep] = (TH1F*)htemp->Clone(Form("hsig%02d%04d",itrial,istep));
      hsig[itrial][istep] -> SetTitle(Form("Signal for trial %02d and frequency step %04d",itrial,istep));

      hbkg[itrial][istep] = (TH1F*)htemp->Clone(Form("hbkg%02d%04d",itrial,istep));
      hbkg[itrial][istep] -> SetTitle(Form("Background for trial %02d and frequency step %04d",itrial,istep));

      hmea[itrial][istep] = (TH1F*)htemp->Clone(Form("hmea%02d%04d",itrial,istep));
      hmea[itrial][istep] -> SetTitle(Form("Measured for trial %02d and frequency step %04d",itrial,istep));

      
      // if axion signal frequency is higher than the highest frequency of
      // this spectrum, skip to the next resonance frequency
      if(f_axion > end_freq)continue;
      if(startStepBin < 0)startStepBin = istep;
      nSpectra++;
      double resFreq = start_freq + 0.5*rangeSpec;
      if(debug)
	cout << "resonance frequency = " <<  resFreq << endl;      
      fsig->SetParameter(0,resFreq);
      // signal power for a given axion frequency and cavity resonance frequency
      double power_sig = fsig->Eval(f_axion); 

      if(debug)
	cout << "power signal " <<  power_sig << endl;      

      // first generate signal
      // find the first bin of axion signal in this spectrum
      // binStart could be equal to zero
      const unsigned int binStart = hsig[itrial][istep]->FindBin(f_axion);
      if(debug) cout << "binStart = " << binStart << endl;
      double binlo = f_axion;
      double binhi = hsig[itrial][istep] ->GetBinLowEdge(binStart+1);
      if(debug)cout << "binlo = " << binlo << "\t binhi = " << binhi << endl;

      // if assuming narrow distribution
      if(narrow)hsig[itrial][istep]->SetBinContent(binStart,power_sig);
      // if assuming a maxwell distribution
      else{
      
	// set signal power for every bin, assuming maxwell distribution
	double vlo=0;
	double vhi=c*sqrt(binhi/f_axion-1);
	if(debug)cout << "vlo = " << vlo << "\t vhi = " << vhi << endl;
	double prob = fspeed->Integral(vlo,vhi);
	double sum_prob = prob;
	double sum_signal_power = power_sig*prob;
	for(int ib=binStart+1; ib <= nBins; ib++){

	  binlo = hsig[itrial][istep] ->GetBinLowEdge(ib);
	  binhi = hsig[itrial][istep] ->GetBinLowEdge(ib+1);
	  if(debug)cout << "binlo = " << binlo << "\t binhi = " << binhi << endl;
	
	  vlo = c*sqrt(binlo/f_axion-1);
	  vhi = c*sqrt(binhi/f_axion-1);
	  if(debug)cout << "vlo = " << vlo << "\t vhi = " << vhi << endl;
	  double prob = fspeed->Integral(vlo,vhi);
	  sum_prob += prob; 
	  double power_for_this_bin = power_sig * prob;
	  sum_signal_power += power_for_this_bin;
	
	  hsig[itrial][istep]->SetBinContent(ib, power_for_this_bin);
	

	} // end of loop over nBins

	if(debug)cout << "sum of probability is " << sum_prob << endl;
	if(debug)cout << "sum of signal power is " << sum_signal_power << endl;
      } // if use a wider distribution
      
      // now generate background spectrum

      for(int ib=1; ib <= nBins; ib++){
	double noise = gRandom->Gaus(0, sigma_noise);
	hbkg[itrial][istep]->SetBinContent(ib, noise);
      }

      // add signal and background generation to get measured spectrum
      hmea[itrial][istep]->Add(hsig[itrial][istep],
			       hbkg[itrial][istep]);

      hsig[itrial][istep]->Write();
      hbkg[itrial][istep]->Write();
      hmea[itrial][istep]->Write();
	
    } // end of loop over steps of frequency

  } // end of loop over trials


  
   outFile->Close();


  
} // end of program








