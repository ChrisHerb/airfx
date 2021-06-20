#include "c74_min.h"

using namespace c74::min;

class holt : public object<holt>, public vector_operator<> {
public:
	MIN_DESCRIPTION {"a synth-like resonant lowpass filter focussed on bass frequencies"};
	MIN_TAGS {"audio, effect"};
	MIN_AUTHOR {"Isabel Kaspriskie"};

	inlet<> in1 {this, "(signal) Input1"};
	inlet<> in2 {this, "(signal) Input2"};
	outlet<> out1 {this, "(signal) Output1", "signal"};
	outlet<> out2 {this, "(signal) Output2", "signal"};

	attribute<number, threadsafe::no, limit::clamp> A {this, "Freq", 1.0, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> B {this, "Reso", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> C {this, "Poles", 1.0, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> D {this, "Output", 1.0, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> E {this, "Dry/Wet", 1.0, range {0.0, 1.0} };

	message<> dspsetup {this, "dspsetup",
		MIN_FUNCTION {
			A = 1.0;
			B = 0.5;
			C = 1.0;
			D = 1.0;
			E = 1.0;
			
			previousSampleAL = 0.0;
			previousTrendAL = 0.0;
			previousSampleBL = 0.0;
			previousTrendBL = 0.0;
			previousSampleCL = 0.0;
			previousTrendCL = 0.0;
			previousSampleDL = 0.0;
			previousTrendDL = 0.0;
		
			previousSampleAR = 0.0;
			previousTrendAR = 0.0;
			previousSampleBR = 0.0;
			previousTrendBR = 0.0;
			previousSampleCR = 0.0;
			previousTrendCR = 0.0;
			previousSampleDR = 0.0;
			previousTrendDR = 0.0;
			
				fpd = 17;
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

		double alpha = pow(A,4)+0.00001;
		if (alpha > 1.0) alpha = 1.0;
		double beta = (alpha * pow(B,2))+0.00001;
		alpha += ((1.0-beta)*pow(A,3)); //correct for droop in frequency
		if (alpha > 1.0) alpha = 1.0;
		
		long double trend;
		long double forecast; //defining these here because we're copying the routine four times
		
		double aWet = 1.0;
		double bWet = 1.0;
		double cWet = 1.0;
		double dWet = C*4.0;
		//four-stage wet/dry control using progressive stages that bypass when not engaged
		if (dWet < 1.0) {aWet = dWet; bWet = 0.0; cWet = 0.0; dWet = 0.0;}
		else if (dWet < 2.0) {bWet = dWet - 1.0; cWet = 0.0; dWet = 0.0;}
		else if (dWet < 3.0) {cWet = dWet - 2.0; dWet = 0.0;}
		else {dWet -= 3.0;}
		//this is one way to make a little set of dry/wet stages that are successively added to the
		//output as the control is turned up. Each one independently goes from 0-1 and stays at 1
		//beyond that point: this is a way to progressively add a 'black box' sound processing
		//which lets you fall through to simpler processing at lower settings.
		
		double gain = D;	
		double wet = E;	
		
	    while (--sampleFrames >= 0)
	    {
			long double inputSampleL = *in1;
			long double inputSampleR = *in2;
	
			if (fabs(inputSampleL)<1.18e-43) inputSampleL = fpd * 1.18e-43;
			if (fabs(inputSampleR)<1.18e-43) inputSampleR = fpd * 1.18e-43;
	
			long double drySampleL = inputSampleL;
			long double drySampleR = inputSampleR;
	
			if (aWet > 0.0) {
				trend = (beta * (inputSampleL - previousSampleAL) + ((0.999-beta) * previousTrendAL));
				forecast = previousSampleAL + previousTrendAL;
				inputSampleL = (alpha * inputSampleL) + ((0.999-alpha) * forecast);
				previousSampleAL = inputSampleL; previousTrendAL = trend;
				inputSampleL = (inputSampleL * aWet) + (drySampleL * (1.0-aWet));
				
				trend = (beta * (inputSampleR - previousSampleAR) + ((0.999-beta) * previousTrendAR));
				forecast = previousSampleAR + previousTrendAR;
				inputSampleR = (alpha * inputSampleR) + ((0.999-alpha) * forecast);
				previousSampleAR = inputSampleR; previousTrendAR = trend;
				inputSampleR = (inputSampleR * aWet) + (drySampleR * (1.0-aWet));
			}
			
			if (bWet > 0.0) {
				trend = (beta * (inputSampleL - previousSampleBL) + ((0.999-beta) * previousTrendBL));
				forecast = previousSampleBL + previousTrendBL;
				inputSampleL = (alpha * inputSampleL) + ((0.999-alpha) * forecast);
				previousSampleBL = inputSampleL; previousTrendBL = trend;
				inputSampleL = (inputSampleL * bWet) + (previousSampleAL * (1.0-bWet));
				
				trend = (beta * (inputSampleR - previousSampleBR) + ((0.999-beta) * previousTrendBR));
				forecast = previousSampleBR + previousTrendBR;
				inputSampleR = (alpha * inputSampleR) + ((0.999-alpha) * forecast);
				previousSampleBR = inputSampleR; previousTrendBR = trend;
				inputSampleR = (inputSampleR * bWet) + (previousSampleAR * (1.0-bWet));
			}
			
			if (cWet > 0.0) {
				trend = (beta * (inputSampleL - previousSampleCL) + ((0.999-beta) * previousTrendCL));
				forecast = previousSampleCL + previousTrendCL;
				inputSampleL = (alpha * inputSampleL) + ((0.999-alpha) * forecast);
				previousSampleCL = inputSampleL; previousTrendCL = trend;
				inputSampleL = (inputSampleL * cWet) + (previousSampleBL * (1.0-cWet));
				
				trend = (beta * (inputSampleR - previousSampleCR) + ((0.999-beta) * previousTrendCR));
				forecast = previousSampleCR + previousTrendCR;
				inputSampleR = (alpha * inputSampleR) + ((0.999-alpha) * forecast);
				previousSampleCR = inputSampleR; previousTrendCR = trend;
				inputSampleR = (inputSampleR * cWet) + (previousSampleBR * (1.0-cWet));
			}
			
			if (dWet > 0.0) {
				trend = (beta * (inputSampleL - previousSampleDL) + ((0.999-beta) * previousTrendDL));
				forecast = previousSampleDL + previousTrendDL;
				inputSampleL = (alpha * inputSampleL) + ((0.999-alpha) * forecast);
				previousSampleDL = inputSampleL; previousTrendDL = trend;
				inputSampleL = (inputSampleL * dWet) + (previousSampleCL * (1.0-dWet));
				
				trend = (beta * (inputSampleR - previousSampleDR) + ((0.999-beta) * previousTrendDR));
				forecast = previousSampleDR + previousTrendDR;
				inputSampleR = (alpha * inputSampleR) + ((0.999-alpha) * forecast);
				previousSampleDR = inputSampleR; previousTrendDR = trend;
				inputSampleR = (inputSampleR * dWet) + (previousSampleCR * (1.0-dWet));
			}
			
			if (gain < 1.0) {
				inputSampleL *= gain;
				inputSampleR *= gain;
			}
			
			//clip to 1.2533141373155 to reach maximum output
			if (inputSampleL > 1.2533141373155) inputSampleL = 1.2533141373155;
			if (inputSampleL < -1.2533141373155) inputSampleL = -1.2533141373155;
			if (inputSampleR > 1.2533141373155) inputSampleR = 1.2533141373155;
			if (inputSampleR < -1.2533141373155) inputSampleR = -1.2533141373155;
			inputSampleL = sin(inputSampleL * fabs(inputSampleL)) / ((fabs(inputSampleL) == 0.0) ?1:fabs(inputSampleL));
			inputSampleR = sin(inputSampleR * fabs(inputSampleR)) / ((fabs(inputSampleR) == 0.0) ?1:fabs(inputSampleR));
			
			if (wet < 1.0) {
				inputSampleL = (inputSampleL*wet)+(drySampleL*(1.0-wet));
				inputSampleR = (inputSampleR*wet)+(drySampleR*(1.0-wet));
			}
			
			//begin 64 bit stereo floating point dither
			int expon; frexp((double)inputSampleL, &expon);
			fpd ^= fpd << 13; fpd ^= fpd >> 17; fpd ^= fpd << 5;
			inputSampleL += static_cast<int32_t>(fpd) * 1.110223024625156e-44L * pow(2,expon+62);
			frexp((double)inputSampleR, &expon);
			fpd ^= fpd << 13; fpd ^= fpd >> 17; fpd ^= fpd << 5;
			inputSampleR += static_cast<int32_t>(fpd) * 1.110223024625156e-44L * pow(2,expon+62);
			//end 64 bit stereo floating point dither
			
			*out1 = inputSampleL;
			*out2 = inputSampleR;
	
			*in1++;
			*in2++;
			*out1++;
			*out2++;
		}
	}
private:
    
		uint32_t fpd;
	//default stuff
	
	long double previousSampleAL;
	long double previousTrendAL;
	long double previousSampleBL;
	long double previousTrendBL;
	long double previousSampleCL;
	long double previousTrendCL;
	long double previousSampleDL;
	long double previousTrendDL;

	long double previousSampleAR;
	long double previousTrendAR;
	long double previousSampleBR;
	long double previousTrendBR;
	long double previousSampleCR;
	long double previousTrendCR;
	long double previousSampleDR;
	long double previousTrendDR;
	

};
MIN_EXTERNAL(holt);
