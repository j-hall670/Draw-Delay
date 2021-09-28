#pragma once

#include <JuceHeader.h>
#include <iostream>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                       public juce::Slider::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& ev) override;
    
    void sliderValueChanged (juce::Slider* volumeSlider) override;

    void fillDelayBuffer(int channel, const int bufferLength, const int delayBufferLength, const float* bufferData, const float* delayBufferData);
    void getFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, const int bufferLength, const int delayBufferLength, const float* bufferData, const float* delayBufferData);

    void feedbackDelay(int channel, const int bufferLength, const int delayBufferLength, float* dryBuffer);

private:
    //==============================================================================
    // Your private member variables go here...

    juce::Array <juce::Point<float>> mousePosArray; // Array of Points in float format - Point is X,Y coordinates, needs to be float because that's what mouse event gives

    juce::TextButton undoButton;

    juce::Rectangle<float> delayBox; // Rectangle for the delay box

    juce::TextButton openButton;  // For opening audio files
    juce::TextButton playButton;  // For playing the audio file
    
    juce::Slider volumeSlider; // For controlling output level

    // Button clicks
    void undoButtonClicked();
    void openButtonClicked();
    void playButtonClicked();

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource; // To read from AudioFormatReader
    juce::AudioTransportSource transportSource; // Basically a positionable audio source with extra features for usability 

    // Circular buffer
    juce::AudioBuffer<float> delayBuffer;
    int writePosition{ 0 };
    int globalSampleRate{ 44100 }; 
    const float maximumDelayTimeS = 5.0f;

    juce::Array<int> delayTimesMS;
    juce::Array<float> delayGains;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
