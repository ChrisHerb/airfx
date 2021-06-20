#include "c74_min.h"

using namespace c74::min;

class dubcenter : public object<dubcenter>, public vector_operator<> {
public:
	MIN_DESCRIPTION {"a version of DubSub where the bass reinforcement is purely mono"};
	MIN_TAGS {"audio, effect"};
	MIN_AUTHOR {"Isabel Kaspriskie"};

	inlet<> in1 {this, "(signal) Input1"};
	inlet<> in2 {this, "(signal) Input2"};
	outlet<> out1 {this, "(signal) Output1", "signal"};
	outlet<> out2 {this, "(signal) Output2", "signal"};

	attribute<number, threadsafe::no, limit::clamp> A {this, "TGrind", 0.9, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> B {this, "Grd/Out", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> C {this, "XOver", 0.74, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> D {this, "BassDrive", 1.0, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> E {this, "BassVoice", 0.95, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> F {this, "BassOut", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> G {this, "SubDrive", 0.2, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> H {this, "SubVoice", 0.2, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> I {this, "SubOut", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> J {this, "Dry/Wet", 1.0, range {0.0, 1.0} };

	message<> dspsetup {this, "dspsetup",
		MIN_FUNCTION {
			A = 0.9;
			B = 0.5;
			C = 0.74;
			D = 1.0;
			E = 0.95;
			F = 0.5;
			G = 0.2;
			H = 0.2;
			I = 0.5;
			J = 1.0;
		
			WasNegative = false;
			SubOctave = false;
			flip = false;
			bflip = 0;
			iirDriveSampleAL = 0.0;
			iirDriveSampleBL = 0.0;
			iirDriveSampleCL = 0.0;
			iirDriveSampleDL = 0.0;
			iirDriveSampleEL = 0.0;
			iirDriveSampleFL = 0.0;
			iirDriveSampleAR = 0.0;
			iirDriveSampleBR = 0.0;
			iirDriveSampleCR = 0.0;
			iirDriveSampleDR = 0.0;
			iirDriveSampleER = 0.0;
			iirDriveSampleFR = 0.0;
			
			iirHeadBumpA = 0.0;
			iirHeadBumpB = 0.0;
			iirHeadBumpC = 0.0;
			
			iirSubBumpA = 0.0;
			iirSubBumpB = 0.0;
			iirSubBumpC = 0.0;
			
			lastHeadBump = 0.0;
			lastSubBump = 0.0;
			
			iirSampleA = 0.0;
			iirSampleB = 0.0;
			iirSampleC = 0.0;
			iirSampleD = 0.0;
			iirSampleE = 0.0;
			iirSampleF = 0.0;
			iirSampleG = 0.0;
			iirSampleH = 0.0;
			iirSampleI = 0.0;
			iirSampleJ = 0.0;
			iirSampleK = 0.0;
			iirSampleL = 0.0;
			iirSampleM = 0.0;
			iirSampleN = 0.0;
			iirSampleO = 0.0;
			iirSampleP = 0.0;
			iirSampleQ = 0.0;
			iirSampleR = 0.0;
			iirSampleS = 0.0;
			iirSampleT = 0.0;
			iirSampleU = 0.0;
			iirSampleV = 0.0;
			iirSampleW = 0.0;
			iirSampleX = 0.0;
			iirSampleY = 0.0;
			iirSampleZ = 0.0;	
			
			oscGate = 1.0;
			
			fpNShapeL = 0.0;
			fpNShapeR = 0.0;
			//this is reset: values being initialized only once. Startup values, whatever they are.
			
			return {};
		}
	};

	void operator()(audio_bundle _input, audio_bundle _output) {
		double* in1 = _input.samples(0);
		double* in2 = _input.samples(1);
		double* out1 = _output.samples(0);
		double* out2 = _output.samples(1);
		long sampleFrames = _input.frame_count();

		double overallscale = 1.0;
		overallscale /= 44100.0;
		overallscale *= samplerate();
		
		double driveone = pow(A*3.0,2);
		double driveoutput = (B*2.0)-1.0;
		double iirAmount = ((C*0.33)+0.1)/overallscale;
		double ataLowpassL;
		double ataLowpassR;
		double ataLowpass;
		double randy;
		double invrandy;
		double HeadBump = 0.0;
		double BassGain = D * 0.1;
		double HeadBumpFreq = ((E*0.1)+0.0001)/overallscale;
		double iirBmount = HeadBumpFreq/44.1;
		double altBmount = 1.0 - iirBmount;
		double BassOutGain = (F*2.0)-1.0;
		double SubBump = 0.0;
		double SubGain = G * 0.1;
		double SubBumpFreq = ((H*0.1)+0.0001)/overallscale;
		double iirCmount = SubBumpFreq/44.1;
		double altCmount = 1.0 - iirCmount;
		double SubOutGain = (I*2.0)-1.0;
		double clamp = 0.0;
		double out;
		double fuzz = 0.111;
		double wet = J;
		double dry = 1.0-wet;
		double glitch = 0.60;
		double tempSample;
		
		while (--sampleFrames >= 0)
	    {
			long double inputSampleL = *in1;
			long double inputSampleR = *in2;
	
			static int noisesourceL = 0;
			static int noisesourceR = 850010;
			int residue;
			double applyresidue;
			
			noisesourceL = noisesourceL % 1700021; noisesourceL++;
			residue = noisesourceL * noisesourceL;
			residue = residue % 170003; residue *= residue;
			residue = residue % 17011; residue *= residue;
			residue = residue % 1709; residue *= residue;
			residue = residue % 173; residue *= residue;
			residue = residue % 17;
			applyresidue = residue;
			applyresidue *= 0.00000001;
			applyresidue *= 0.00000001;
			inputSampleL += applyresidue;
			if (inputSampleL<1.2e-38 && -inputSampleL<1.2e-38) {
				inputSampleL -= applyresidue;
			}
			
			noisesourceR = noisesourceR % 1700021; noisesourceR++;
			residue = noisesourceR * noisesourceR;
			residue = residue % 170003; residue *= residue;
			residue = residue % 17011; residue *= residue;
			residue = residue % 1709; residue *= residue;
			residue = residue % 173; residue *= residue;
			residue = residue % 17;
			applyresidue = residue;
			applyresidue *= 0.00000001;
			applyresidue *= 0.00000001;
			inputSampleR += applyresidue;
			if (inputSampleR<1.2e-38 && -inputSampleR<1.2e-38) {
				inputSampleR -= applyresidue;
			}
			//for live air, we always apply the dither noise. Then, if our result is 
			//effectively digital black, we'll subtract it again. We want a 'air' hiss
			long double drySampleL = inputSampleL;
			long double drySampleR = inputSampleR;
			
			// here's the plan.
			// Grind Boost
			// Grind Output Level
			// Bass Split Freq
			// Bass Drive
			// Bass Voicing
			// Bass Output Level
			// Sub Oct Drive
			// Sub Voicing
			// Sub Output Level
			// Dry/Wet
			
			oscGate += fabs((inputSampleL + inputSampleR) * 5.0);
			oscGate -= 0.001;
			if (oscGate > 1.0) oscGate = 1.0;
			if (oscGate < 0) oscGate = 0;
			//got a value that only goes down low when there's silence or near silence on input
			clamp = 1.0-oscGate;
			clamp *= 0.00001;
			//set up the thing to choke off oscillations- belt and suspenders affair
			
			
			if (flip)
			{
				tempSample = inputSampleL;
				iirDriveSampleAL = (iirDriveSampleAL * (1 - iirAmount)) + (inputSampleL * iirAmount);
				inputSampleL -= iirDriveSampleAL;
				iirDriveSampleCL = (iirDriveSampleCL * (1 - iirAmount)) + (inputSampleL * iirAmount);
				inputSampleL -= iirDriveSampleCL;
				iirDriveSampleEL = (iirDriveSampleEL * (1 - iirAmount)) + (inputSampleL * iirAmount);
				inputSampleL -= iirDriveSampleEL;
				ataLowpassL = tempSample - inputSampleL;
				
				tempSample = inputSampleR;
				iirDriveSampleAR = (iirDriveSampleAR * (1 - iirAmount)) + (inputSampleR * iirAmount);
				inputSampleR -= iirDriveSampleAR;
				iirDriveSampleCR = (iirDriveSampleCR * (1 - iirAmount)) + (inputSampleR * iirAmount);
				inputSampleR -= iirDriveSampleCR;
				iirDriveSampleER = (iirDriveSampleER * (1 - iirAmount)) + (inputSampleR * iirAmount);
				inputSampleR -= iirDriveSampleER;
				ataLowpassR = tempSample - inputSampleR;
			}
			else
			{
				tempSample = inputSampleL;
				iirDriveSampleBL = (iirDriveSampleBL * (1 - iirAmount)) + (inputSampleL * iirAmount);
				inputSampleL -= iirDriveSampleBL;
				iirDriveSampleDL = (iirDriveSampleDL * (1 - iirAmount)) + (inputSampleL * iirAmount);
				inputSampleL -= iirDriveSampleDL;
				iirDriveSampleFL = (iirDriveSampleFL * (1 - iirAmount)) + (inputSampleL * iirAmount);
				inputSampleL -= iirDriveSampleFL;
				ataLowpassL = tempSample - inputSampleL;
				
				tempSample = inputSampleR;
				iirDriveSampleBR = (iirDriveSampleBR * (1 - iirAmount)) + (inputSampleR * iirAmount);
				inputSampleR -= iirDriveSampleBR;
				iirDriveSampleDR = (iirDriveSampleDR * (1 - iirAmount)) + (inputSampleR * iirAmount);
				inputSampleR -= iirDriveSampleDR;
				iirDriveSampleFR = (iirDriveSampleFR * (1 - iirAmount)) + (inputSampleR * iirAmount);
				inputSampleR -= iirDriveSampleFR;
				ataLowpassR = tempSample - inputSampleR;
			}
			//highpass section
			
			if (inputSampleL > 1.0) {inputSampleL = 1.0;}
			if (inputSampleL < -1.0) {inputSampleL = -1.0;}
			if (inputSampleR > 1.0) {inputSampleR = 1.0;}
			if (inputSampleR < -1.0) {inputSampleR = -1.0;}
			
			out = driveone;
			while (out > glitch)
			{
				out -= glitch;
				inputSampleL -= (inputSampleL * (fabs(inputSampleL) * glitch) * (fabs(inputSampleL) * glitch) );
				inputSampleL *= (1.0+glitch);
			}
			//that's taken care of the really high gain stuff
			inputSampleL -= (inputSampleL * (fabs(inputSampleL) * out) * (fabs(inputSampleL) * out) );
			inputSampleL *= (1.0+out);
			
			
			out = driveone;
			while (out > glitch)
			{
				out -= glitch;
				inputSampleR -= (inputSampleR * (fabs(inputSampleR) * glitch) * (fabs(inputSampleR) * glitch) );
				inputSampleR *= (1.0+glitch);
			}
			//that's taken care of the really high gain stuff
			inputSampleR -= (inputSampleR * (fabs(inputSampleR) * out) * (fabs(inputSampleR) * out) );
			inputSampleR *= (1.0+out);
			
			ataLowpass = (ataLowpassL + ataLowpassR) / 2.0;
			
			if (ataLowpass > 0)
			{if (WasNegative){SubOctave = !SubOctave;} WasNegative = false;}
			else {WasNegative = true;}
			//set up polarities for sub-bass version
			randy = (rand()/(double)RAND_MAX)*fuzz; //0 to 1 the noise, may not be needed
			invrandy = (1.0-randy);
			randy /= 2.0;
			//set up the noise
			
			iirSampleA = (iirSampleA * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleA;
			iirSampleB = (iirSampleB * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleB;
			iirSampleC = (iirSampleC * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleC;
			iirSampleD = (iirSampleD * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleD;
			iirSampleE = (iirSampleE * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleE;
			iirSampleF = (iirSampleF * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleF;
			iirSampleG = (iirSampleG * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleG;
			iirSampleH = (iirSampleH * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleH;
			iirSampleI = (iirSampleI * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleI;
			iirSampleJ = (iirSampleJ * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleJ;
			iirSampleK = (iirSampleK * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleK;
			iirSampleL = (iirSampleL * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleL;
			iirSampleM = (iirSampleM * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleM;
			iirSampleN = (iirSampleN * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleN;
			iirSampleO = (iirSampleO * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleO;
			iirSampleP = (iirSampleP * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleP;
			iirSampleQ = (iirSampleQ * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleQ;
			iirSampleR = (iirSampleR * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleR;
			iirSampleS = (iirSampleS * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleS;
			iirSampleT = (iirSampleT * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleT;
			iirSampleU = (iirSampleU * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleU;
			iirSampleV = (iirSampleV * altBmount) + (ataLowpass * iirBmount); ataLowpass -= iirSampleV;
			
			switch (bflip)
			{
				case 1:				
					iirHeadBumpA += (ataLowpass * BassGain);
					iirHeadBumpA -= (iirHeadBumpA * iirHeadBumpA * iirHeadBumpA * HeadBumpFreq);
					iirHeadBumpA = (invrandy * iirHeadBumpA) + (randy * iirHeadBumpB) + (randy * iirHeadBumpC);
					if (iirHeadBumpA > 0) iirHeadBumpA -= clamp;
					if (iirHeadBumpA < 0) iirHeadBumpA += clamp;
					HeadBump = iirHeadBumpA;
					break;
				case 2:
					iirHeadBumpB += (ataLowpass * BassGain);
					iirHeadBumpB -= (iirHeadBumpB * iirHeadBumpB * iirHeadBumpB * HeadBumpFreq);
					iirHeadBumpB = (randy * iirHeadBumpA) + (invrandy * iirHeadBumpB) + (randy * iirHeadBumpC);
					if (iirHeadBumpB > 0) iirHeadBumpB -= clamp;
					if (iirHeadBumpB < 0) iirHeadBumpB += clamp;
					HeadBump = iirHeadBumpB;
					break;
				case 3:
					iirHeadBumpC += (ataLowpass * BassGain);
					iirHeadBumpC -= (iirHeadBumpC * iirHeadBumpC * iirHeadBumpC * HeadBumpFreq);
					iirHeadBumpC = (randy * iirHeadBumpA) + (randy * iirHeadBumpB) + (invrandy * iirHeadBumpC);
					if (iirHeadBumpC > 0) iirHeadBumpC -= clamp;
					if (iirHeadBumpC < 0) iirHeadBumpC += clamp;
					HeadBump = iirHeadBumpC;
					break;
			}
			
			iirSampleW = (iirSampleW * altBmount) + (HeadBump * iirBmount); HeadBump -= iirSampleW;
			iirSampleX = (iirSampleX * altBmount) + (HeadBump * iirBmount); HeadBump -= iirSampleX;
			
			SubBump = HeadBump;
			iirSampleY = (iirSampleY * altCmount) + (SubBump * iirCmount); SubBump -= iirSampleY;
			
			SubBump = fabs(SubBump);
			if (SubOctave == false) {SubBump = -SubBump;}
			
			switch (bflip)
			{
				case 1:				
					iirSubBumpA += (SubBump * SubGain);
					iirSubBumpA -= (iirSubBumpA * iirSubBumpA * iirSubBumpA * SubBumpFreq);
					iirSubBumpA = (invrandy * iirSubBumpA) + (randy * iirSubBumpB) + (randy * iirSubBumpC);
					if (iirSubBumpA > 0) iirSubBumpA -= clamp;
					if (iirSubBumpA < 0) iirSubBumpA += clamp;
					SubBump = iirSubBumpA;
					break;
				case 2:
					iirSubBumpB += (SubBump * SubGain);
					iirSubBumpB -= (iirSubBumpB * iirSubBumpB * iirSubBumpB * SubBumpFreq);
					iirSubBumpB = (randy * iirSubBumpA) + (invrandy * iirSubBumpB) + (randy * iirSubBumpC);
					if (iirSubBumpB > 0) iirSubBumpB -= clamp;
					if (iirSubBumpB < 0) iirSubBumpB += clamp;
					SubBump = iirSubBumpB;
					break;
				case 3:
					iirSubBumpC += (SubBump * SubGain);
					iirSubBumpC -= (iirSubBumpC * iirSubBumpC * iirSubBumpC * SubBumpFreq);
					iirSubBumpC = (randy * iirSubBumpA) + (randy * iirSubBumpB) + (invrandy * iirSubBumpC);
					if (iirSubBumpC > 0) iirSubBumpC -= clamp;
					if (iirSubBumpC < 0) iirSubBumpC += clamp;
					SubBump = iirSubBumpC;
					break;
			}
			
			iirSampleZ = (iirSampleZ * altCmount) + (SubBump * iirCmount); SubBump -= iirSampleZ;
			
			inputSampleL *= driveoutput; //start with the drive section then add lows and subs
			inputSampleR *= driveoutput; //start with the drive section then add lows and subs
			
			inputSampleL += ((HeadBump + lastHeadBump) * BassOutGain);
			inputSampleL += ((SubBump + lastSubBump) * SubOutGain);
			
			inputSampleR += ((HeadBump + lastHeadBump) * BassOutGain);
			inputSampleR += ((SubBump + lastSubBump) * SubOutGain);
			
			lastHeadBump = HeadBump;
			lastSubBump = SubBump;
			
			if (wet != 1.0) {
				inputSampleL = (inputSampleL * wet) + (drySampleL * dry);
				inputSampleR = (inputSampleR * wet) + (drySampleR * dry);
			}
			//Dry/Wet control, defaults to the last slider
			
			flip = !flip;
			bflip++;
			if (bflip < 1 || bflip > 3) bflip = 1;
			
			//stereo 64 bit dither, made small and tidy.
			int expon; frexp((double)inputSampleL, &expon);
			long double dither = (rand()/(RAND_MAX*7.737125245533627e+25))*pow(2,expon+62);
			dither /= 536870912.0; //needs this to scale to 64 bit zone
			inputSampleL += (dither-fpNShapeL); fpNShapeL = dither;
			frexp((double)inputSampleR, &expon);
			dither = (rand()/(RAND_MAX*7.737125245533627e+25))*pow(2,expon+62);
			dither /= 536870912.0; //needs this to scale to 64 bit zone
			inputSampleR += (dither-fpNShapeR); fpNShapeR = dither;
			//end 64 bit dither
			
			*out1 = inputSampleL;
			*out2 = inputSampleR;
	
			*in1++;
			*in2++;
			*out1++;
			*out2++;
		}
	}
private:
    
	double iirDriveSampleAL;
	double iirDriveSampleBL;
	double iirDriveSampleCL;
	double iirDriveSampleDL;
	double iirDriveSampleEL;
	double iirDriveSampleFL;
	double iirDriveSampleAR;
	double iirDriveSampleBR;
	double iirDriveSampleCR;
	double iirDriveSampleDR;
	double iirDriveSampleER;
	double iirDriveSampleFR;
	bool flip; //drive things
	
	int bflip;
	bool WasNegative;
	bool SubOctave;
	double iirHeadBumpA;
	double iirHeadBumpB;
	double iirHeadBumpC;
	
	double iirSubBumpA;
	double iirSubBumpB;
	double iirSubBumpC;
	
	double lastHeadBump;
	double lastSubBump;
	
	double iirSampleA;
	double iirSampleB;
	double iirSampleC;
	double iirSampleD;
	double iirSampleE;
	double iirSampleF;
	double iirSampleG;
	double iirSampleH;
	double iirSampleI;
	double iirSampleJ;
	double iirSampleK;
	double iirSampleL;
	double iirSampleM;
	double iirSampleN;
	double iirSampleO;
	double iirSampleP;
	double iirSampleQ;
	double iirSampleR;
	double iirSampleS;
	double iirSampleT;
	double iirSampleU;
	double iirSampleV;
	double iirSampleW;
	double iirSampleX;
	double iirSampleY;
	double iirSampleZ;		
	double oscGate;
	
	long double fpNShapeL;
	long double fpNShapeR;
	//default stuff
	
};
MIN_EXTERNAL(dubcenter);

