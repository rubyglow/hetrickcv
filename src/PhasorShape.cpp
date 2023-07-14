#include "HetrickCV.hpp"
#include "DSP/HCVPhasorEffects.h"

struct PhasorShape : HCVModule
{
	enum ParamIds
	{
        SHAPE_PARAM,
        SHAPECV_PARAM,
        MODE_PARAM,
        MODECV_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        SHAPECV_INPUT,
        MODECV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS = 8
	};

    static constexpr float MODE_CV_SCALE = 7.0f/5.0f;

	PhasorShape()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(SHAPE_PARAM, -5.0, 5.0, 0.0, "Phasor Shape");
        configParam(SHAPECV_PARAM, -1.0, 1.0, 1.0, "Phasor Shape CV Depth");

        configSwitch(MODE_PARAM, 0.0, 7.0, 0.0, "Shape Mode", 
        {"Curve", "S-Curve", "Kink", "Split", "Shift", "Speed - Clip", "Speed - Wrap", "Speed - Fold"});
        configParam(MODECV_PARAM, -1.0, 1.0, 1.0, "Shape Mode CV Depth");
        paramQuantities[MODE_PARAM]->snapEnabled = true;

        configInput(PHASOR_INPUT, "Phasor");
        configInput(SHAPECV_INPUT, "Phasor Shape CV");
        configInput(MODECV_INPUT, "Shape Mode CV");

        configOutput(PHASOR_OUTPUT, "Shaped Phasor");

		onReset();
	}

	void process(const ProcessArgs &args) override;
    float phasorShape(float _phasor, float _parameter, int _mode);

    void onReset() override
    {

    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorShape::process(const ProcessArgs &args)
{
    const float shapeKnob = params[SHAPE_PARAM].getValue();
    const float shapeDepth = params[SHAPECV_PARAM].getValue();

    const float modeKnob = params[MODE_PARAM].getValue();
    const float modeDepth = params[MODECV_PARAM].getValue() * MODE_CV_SCALE;

    int numChannels = getMaxInputPolyphony();
    outputs[PHASOR_OUTPUT].setChannels(numChannels);

    for (int i = 0; i < numChannels; i++)
    {
        float shape = shapeKnob + (shapeDepth * inputs[SHAPECV_INPUT].getPolyVoltage(i));
        shape = clamp(shape, -5.0f, 5.0f) * 0.2f;

        float modeMode = modeKnob + (modeDepth * inputs[MODECV_INPUT].getPolyVoltage(i));
        int mode = (int)clamp(modeKnob, 0.0f, 7.0f);

        const float phasorInput = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float scaledPhasor = scaleAndWrapPhasor(phasorInput);

        const float shapedOutput = phasorShape(scaledPhasor, shape, mode);

        outputs[PHASOR_OUTPUT].setVoltage(shapedOutput * 5.0f, i);
    }

    int lightMode = (int)modeKnob;
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[i].setBrightness(lightMode == i ? 5.0f : 0.0f);
    }
    
}

float PhasorShape::phasorShape(float _phasor, float _parameter, int _mode)
{
    switch (_mode)
    {
    case 0: return HCVPhasorEffects::phasorCurve(_phasor, _parameter); //curve
    case 1: return HCVPhasorEffects::phasorPinch(_phasor, _parameter); //s-curve
    case 2: return HCVPhasorEffects::phasorKink(_phasor, _parameter); //kink
    case 3: return HCVPhasorEffects::phasorSplit(_phasor, _parameter); //split
    case 4: return HCVPhasorEffects::phasorShift(_phasor, _parameter); //phase shift
    case 5: return HCVPhasorEffects::speedClip(_phasor, _parameter); //speed clip
    case 6: return HCVPhasorEffects::speedWrap(_phasor, _parameter); //speed wrap
    case 7: return HCVPhasorEffects::speedFold(_phasor, _parameter); //speed fold
    
    default: return _phasor;
    }
}

struct PhasorShapeWidget : HCVModuleWidget { PhasorShapeWidget(PhasorShape *module); };

PhasorShapeWidget::PhasorShapeWidget(PhasorShape *module)
{
    setSkinPath("res/PhasorShape.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    int jackY = 310;
    createInputPort(21, jackY, PhasorShape::PHASOR_INPUT);
    int knobY = 90;
    createParamComboVertical(15, knobY, PhasorShape::SHAPE_PARAM, PhasorShape::SHAPECV_PARAM, PhasorShape::SHAPECV_INPUT);
    createParamComboVertical(70, knobY, PhasorShape::MODE_PARAM, PhasorShape::MODECV_PARAM, PhasorShape::MODECV_INPUT);
    
    createOutputPort(70, jackY, PhasorShape::PHASOR_OUTPUT);

    for (int i = 0; i < 4; i++)
    {
        float lightY = 250 + i*10.0f;
        createHCVRedLight(35, lightY, i*2);
        createHCVRedLight(65, lightY, i*2 + 1);
    }
    
    
}

Model *modelPhasorShape = createModel<PhasorShape, PhasorShapeWidget>("PhasorShape");