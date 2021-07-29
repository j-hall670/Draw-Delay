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
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()

    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)

    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock (bufferToFill);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()

    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!

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

    // If click is within the box's bounds
    if (ev.position.x > delayBox.getX() && ev.position.x < width && ev.position.y > delayBox.getY() && ev.position.y < height)
    {
        mousePosArray.add(ev.position); // Add mouse click coordinates to array of points
        repaint();
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
    juce::FileChooser chooser("Select an mp3 file to play...", {}, "*.mp3");

    if (chooser.browseForFileToOpen())                                          
    {
        auto file = chooser.getResult();                                       
        auto* reader = formatManager.createReaderFor (file);                    

        if (reader != nullptr)
        {
            std::unique_ptr<juce::AudioFormatReaderSource> newSource (new juce::AudioFormatReaderSource (reader, true)); 
            transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);                                 
            playButton.setEnabled (true);                                                                                
            readerSource.reset (newSource.release());                                                                    
        }
    }
}

void MainComponent::playButtonClicked()
{
    transportSource.start();
}