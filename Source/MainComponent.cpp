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

    if (readerSource.get() == nullptr) // Check for valid reader source
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock (bufferToFill);

    /* Just seeing if start() stops the file playing automatically when the file runs out - it does
    if (transportSource.isPlaying())
    {
        DBG("Playing");
    }
    else if (!transportSource.isPlaying())
    {
        DBG("Not Playing");
    }
    */
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