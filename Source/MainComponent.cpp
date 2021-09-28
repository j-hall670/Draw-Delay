#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (800, 600);

    addAndMakeVisible (undoButton);
    undoButton.setButtonText ("Undo");
    undoButton.onClick = [this] { undoButtonClicked(); };

    addAndMakeVisible (openButton);
    openButton.setButtonText ("Open");
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible (playButton);
    playButton.setButtonText ("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setEnabled (false);
    
    addAndMakeVisible (volumeSlider);
    volumeSlider.setSliderStyle (juce::Slider::SliderStyle::LinearBarVertical);
    volumeSlider.setRange (0, 1.0);
    volumeSlider.addListener (this);

    formatManager.registerBasicFormats(); // Register audio formats

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);

    delayBuffer.setSize (2, maximumDelayTimeS * (samplesPerBlockExpected + sampleRate), false, true);
    globalSampleRate = sampleRate;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr) // Check for valid reader source
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    
    transportSource.getNextAudioBlock (bufferToFill);
    
    // Declaring these for readability
    const int bufferLength = bufferToFill.buffer->getNumSamples();
    const int delayBufferLength = delayBuffer.getNumSamples();

    for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        const float* bufferData = bufferToFill.buffer->getReadPointer(channel);
        const float* delayBufferData = delayBuffer.getReadPointer(channel);
        float* dryBuffer = bufferToFill.buffer->getWritePointer(channel);

        fillDelayBuffer(channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
        getFromDelayBuffer(*bufferToFill.buffer, channel, bufferLength, delayBufferLength, bufferData, delayBufferData); 
        feedbackDelay(channel, bufferLength, delayBufferLength, dryBuffer);
    }

    writePosition += bufferLength; // When buffer has been processed, move write position to the next value so it becomes e.g. 513 not 0 again
    writePosition %= delayBufferLength; // Look below for explanation
    /*
    This has the effect of wrapping the value back around to 0.
    So when delayBufferLength gets to its maximum value, mWritePosition will become the same number as delayBufferLength
    So modulo divides mWritePosition by delayBufferLength which is the same as dividing it by itself.
    Dividing by itself = 1 with remainder 0.
    So mWritePosition becomes 0.
    */

    // Apply slider volume
    bufferToFill.buffer->applyGain (volumeSlider.getValue());
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()

    //transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.drawRect(delayBox);

    for (int i = 0; i < mousePosArray.size(); i++)
    {
        // Get array member of all circles
        int mouseX = mousePosArray.operator[](i).getX() - 5; // - 5 to centre circle on mouse
        int mouseY = mousePosArray.operator[](i).getY() - 5;

        // Big if statement to make sure the circles don't overlap the box's bounds
        if (mousePosArray.operator[](i).getX() < (delayBox.getX() + 5)) // If click is less across than box's x position + 5
        {
            mouseX = delayBox.getX(); // Set dot's X to the edge of the box
            // Don't need to change this box X value because the circles are drawn from the top left corner
        }
        if (mousePosArray.operator[](i).getX() > (delayBox.getRight() - 5)) // Right
        {
            mouseX = delayBox.getRight() - 10;
        }
        if (mousePosArray.operator[](i).getY() < (delayBox.getY() + 5)) // Top
        {
            mouseY = delayBox.getY(); 
        }
        if (mousePosArray.operator[](i).getY() > (delayBox.getBottom() - 5)) // Bottom
        {
            mouseY = delayBox.getBottom() - 10;
        }
        

        g.fillEllipse(mouseX, mouseY, 10, 10); // Draw circle
    }
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    undoButton.setBounds (juce::Component::getWidth() / 1.4, juce::Component::getHeight() / 8, juce::Component::getWidth() / 10, juce::Component::getWidth() / 10);
    openButton.setBounds (juce::Component::getWidth() / 1.4, juce::Component::getHeight() / 3, juce::Component::getWidth() / 10, juce::Component::getWidth() / 10);
    playButton.setBounds (juce::Component::getWidth() / 1.4, juce::Component::getHeight() / 2.05, juce::Component::getWidth() / 10, juce::Component::getWidth() / 10);
    volumeSlider.setBounds (juce::Component::getWidth() / 1.4, juce::Component::getHeight() / 1.5, juce::Component::getWidth() / 10, juce::Component::getWidth() / 10);

    delayBox.setX (juce::Component::getWidth() / 8);       // Box X position
    delayBox.setY (juce::Component::getHeight() / 8);      // Box Y position
    delayBox.setWidth (juce::Component::getWidth() / 2);   // Box width
    delayBox.setHeight (juce::Component::getHeight() / 2); // Box height
}

void MainComponent::mouseDown (const juce::MouseEvent& ev)
{
    // Use bounds of box to make width and height for readability 
    int width = delayBox.getX() + delayBox.getWidth();
    int height = delayBox.getY() + delayBox.getHeight();

    bool removing = false;

    // If click is within the box's bounds
    if (ev.position.x > delayBox.getX() && ev.position.x < width && ev.position.y > delayBox.getY() && ev.position.y < height)
    {
        // Search array 
        for (int i = 0; i < mousePosArray.size(); i++)
        {
            // If the click position is within the bounds of a drawn cirle in the array
            if ((juce::Range<float>(mousePosArray[i].getX() - 5, mousePosArray[i].getX() + 5).contains(ev.position.getX())) && (juce::Range<float>(mousePosArray[i].getY() - 5, mousePosArray[i].getY() + 5).contains(ev.position.getY())))
            {
                // Debugging 
                DBG("\nRemoving:");
                DBG("mouse = " << ev.position.getX() << ", " << ev.position.getY());
                DBG("circle = " << mousePosArray[i].getX() << ", " << mousePosArray[i].getY());

                // Remove element from array 
                mousePosArray.remove(i);
                repaint();
                removing = true;
            }
        }

        if (removing == false)
        {
            mousePosArray.add(ev.position); // Add mouse click coordinates to array of points

            // Debugging
            DBG("\nAdding:");
            DBG("mouse = " << ev.position.getX() << ", " << ev.position.getY());
            DBG("circle = " << mousePosArray.getLast().getX() << ", " << mousePosArray.getLast().getY());

            repaint();
        }
    }
}

void MainComponent::undoButtonClicked()
{
    // If circles have been drawn
    if (mousePosArray.size() > 0)
    {
        // Remove them
        int element = mousePosArray.size() - 1; // -1 because element index starts at 0
        mousePosArray.remove(element);
        repaint();
    }
}

void MainComponent::openButtonClicked()
{
    // Declare file chooser and make sure it can only open mp3s
    juce::FileChooser chooser("Select an mp3 file to play...", {}, "*.mp3");

    // Open file chooser - if succeeds if user actually selects a file rather than cancelling
    if (chooser.browseForFileToOpen())                                          
    {
        auto file = chooser.getResult(); // Get file                                      
        auto* reader = formatManager.createReaderFor (file); // Create a reader to read the file - is new because it needs to be deleted when out of scope.

        if (reader != nullptr) // If reader works - returns nullptr if file is not a format that AudioFormatManager can handle
        {
            std::unique_ptr<juce::AudioFormatReaderSource> newSource (new juce::AudioFormatReaderSource (reader, true)); // Make new source
            // - declare as a unique ptr to avoid deleting previously allocated AudioFormatReaderSource on subsequent open file commands
            transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate); // Connect reader to AudioTransportSource for use in getNextAudioBlock()                                
            playButton.setEnabled (true);                                                                                
            readerSource.reset (newSource.release()); // Safely release source resources as we have passed newSource's data on                                                                     
        }
    }
}

void MainComponent::playButtonClicked()
{
    transportSource.setPosition(0); // Reset the file position
    transportSource.start(); // Start playback
}

void MainComponent::sliderValueChanged(juce::Slider* volumeSlider) {}

void MainComponent::fillDelayBuffer(int channel, const int bufferLength, const int delayBufferLength, const float* bufferData, const float* delayBufferData)
{
    const float gain = 0.3;

    // Copy data from main buffer to delay buffer - this is a bit fiddly because the buffers are different lengths
    
    // This if alone won't fill the buffer because buffer is smaller than mDelayBuffer 
    if (delayBufferLength > bufferLength + writePosition)
    {
        delayBuffer.copyFromWithRamp(channel, writePosition, bufferData, bufferLength, gain, gain);
    }
    // So we have to catch the rest of them - look at TAP delay pt 1 tutorial for explanation of this
    else
    {
        const int bufferRemaining = delayBufferLength - writePosition; // This is the number of values left to move after the if above ^

        delayBuffer.copyFromWithRamp(channel, writePosition, bufferData + bufferRemaining, bufferRemaining, 0.8, 0.8);
        delayBuffer.copyFromWithRamp(channel, 0, bufferData + bufferRemaining, bufferLength - bufferRemaining, 0.8, 0.8); // Wrap to start of buffer
    }
}

void MainComponent::getFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, const int bufferLength, const int delayBufferLength, const float* bufferData, const float* delayBufferData)
{
    int delayTimeMS = 200;
    const int readPosition = static_cast<int> (delayBufferLength + writePosition - (globalSampleRate * delayTimeMS / 1000)) % delayBufferLength;

    if (delayBufferLength > bufferLength + readPosition)
    {
        buffer.addFrom(channel, 0, delayBufferData + readPosition, bufferLength);
    }
    else
    {
        const int bufferRemaining = delayBufferLength - readPosition;
        buffer.addFrom(channel, 0, delayBufferData + readPosition, bufferRemaining);
        buffer.addFrom(channel, bufferRemaining, delayBufferData, bufferLength - bufferRemaining);
    }
}

void MainComponent::feedbackDelay(int channel, const int bufferLength, const int delayBufferLength, float* dryBuffer)
{
    if (delayBufferLength > bufferLength + writePosition)
    {
        delayBuffer.addFromWithRamp(channel, writePosition, dryBuffer, bufferLength, 0.8, 0.8);
    }
    else
    {
        const int bufferRemaining = delayBufferLength - writePosition;

        delayBuffer.addFromWithRamp(channel, bufferRemaining, dryBuffer, bufferRemaining, 0.8, 0.8);
        delayBuffer.addFromWithRamp(channel, 0, dryBuffer, bufferLength - bufferRemaining, 0.8, 0.8);
    }
}