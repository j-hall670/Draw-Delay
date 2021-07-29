#pragma once

#include <JuceHeader.h>
#include <iostream>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
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

private:
    //==============================================================================
    // Your private member variables go here...

    juce::Array <juce::Point<float>> mousePosArray; // Array of Points in float format - Point is X,Y coordinates, needs to be float because that's what mouse event gives

    juce::TextButton undoButton;

    juce::Rectangle<float> delayBox; // Rectangle for the delay box

    juce::TextButton openButton;  // For opening audio files
    juce::TextButton playButton;  // For playing the audio file

    // Button clicks
    void undoButtonClicked();
    void openButtonClicked();
    void playButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
