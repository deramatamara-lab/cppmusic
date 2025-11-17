#include "InspectorPanel.h"
#include "../lookandfeel/DesignSystem.h"

using namespace daw::ui::lookandfeel::DesignSystem;

namespace daw::ui::views
{

InspectorPanel::InspectorPanel(std::shared_ptr<daw::project::ProjectModel> projectModel,
                               std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : projectModel(projectModel)
    , engineContext(engineContext)
    , titleLabel("Title", "Inspector")
    , nameLabel("Name", "Name:")
    , colorLabel("Color", "Color:")
    , gainLabel("Gain", "Gain:")
    , gainSlider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight)
    , panLabel("Pan", "Pan:")
    , panSlider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight)
    , startLabel("Start", "Start:")
    , lengthLabel("Length", "Length:")
    , currentTrack(nullptr)
    , currentClip(nullptr)
{
    setupUI();
}

InspectorPanel::~InspectorPanel()
{
}

void InspectorPanel::setupUI()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Enhanced labels with better typography
    addAndMakeVisible(titleLabel);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(getHeadingFont(Typography::heading3));
    
    addAndMakeVisible(nameLabel);
    nameLabel.setJustificationType(juce::Justification::centredRight);
    nameLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(nameEditor);
    nameEditor.onTextChange = [this] { nameChanged(); };
    nameEditor.setFont(getBodyFont(Typography::body));
    
    addAndMakeVisible(colorLabel);
    colorLabel.setJustificationType(juce::Justification::centredRight);
    colorLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(colorButton);
    colorButton.onClick = [this] { colorChanged(); };
    // Note: TextButton doesn't have setFont() - font is controlled via LookAndFeel
    
    // Listen to selection changes
    if (projectModel != nullptr)
    {
        projectModel->getSelectionModel().addSelectionListener([this] { refresh(); });
    }
    
    // Enhanced labels with typography
    addAndMakeVisible(gainLabel);
    gainLabel.setJustificationType(juce::Justification::centredRight);
    gainLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(gainSlider);
    gainSlider.setRange(-60.0, 12.0, 0.1);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.onValueChange = [this] { gainChanged(); };
    
    addAndMakeVisible(panLabel);
    panLabel.setJustificationType(juce::Justification::centredRight);
    panLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.onValueChange = [this] { panChanged(); };
    
    addAndMakeVisible(startLabel);
    startLabel.setJustificationType(juce::Justification::centredRight);
    startLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(startEditor);
    startEditor.onTextChange = [this] { startChanged(); };
    startEditor.setFont(getMonoFont(Typography::body));
    
    addAndMakeVisible(lengthLabel);
    lengthLabel.setJustificationType(juce::Justification::centredRight);
    lengthLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(lengthEditor);
    lengthEditor.onTextChange = [this] { lengthChanged(); };
    lengthEditor.setFont(getMonoFont(Typography::body));
}

void InspectorPanel::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Enhanced glassmorphism background
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::none, false);
    
    // Enhanced divider line with gradient
    juce::ColourGradient dividerGradient(juce::Colour(Colors::divider).withAlpha(0.0f),
                                        0.0f,
                                        bounds.getY(),
                                        juce::Colour(Colors::divider),
                                        0.0f,
                                        bounds.getCentreY(),
                                        false);
    g.setGradientFill(dividerGradient);
    g.drawLine(0.0f, 0.0f, 0.0f, bounds.getHeight(), 1.5f);
}

void InspectorPanel::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    auto bounds = getLocalBounds().reduced(Spacing::small);
    
    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(Spacing::medium);
    
    auto nameLabelBounds = bounds.removeFromTop(20);
    nameLabel.setBounds(nameLabelBounds.removeFromLeft(60));
    nameEditor.setBounds(nameLabelBounds);
    bounds.removeFromTop(Spacing::small);
    
    auto colorLabelBounds = bounds.removeFromTop(30);
    colorLabel.setBounds(colorLabelBounds.removeFromLeft(60));
    colorButton.setBounds(colorLabelBounds.removeFromLeft(80));
    bounds.removeFromTop(Spacing::medium);
    
    if (currentTrack != nullptr)
    {
        auto gainLabelBounds = bounds.removeFromTop(20);
        gainLabel.setBounds(gainLabelBounds.removeFromLeft(60));
        gainSlider.setBounds(gainLabelBounds);
        bounds.removeFromTop(Spacing::small);
        
        auto panLabelBounds = bounds.removeFromTop(20);
        panLabel.setBounds(panLabelBounds.removeFromLeft(60));
        panSlider.setBounds(panLabelBounds);
    }
    
    if (currentClip != nullptr)
    {
        auto startLabelBounds = bounds.removeFromTop(20);
        startLabel.setBounds(startLabelBounds.removeFromLeft(60));
        startEditor.setBounds(startLabelBounds);
        bounds.removeFromTop(Spacing::small);
        
        auto lengthLabelBounds = bounds.removeFromTop(20);
        lengthLabel.setBounds(lengthLabelBounds.removeFromLeft(60));
        lengthEditor.setBounds(lengthLabelBounds);
    }
}

void InspectorPanel::refresh()
{
    updateFromSelection();
    resized();
}

void InspectorPanel::updateFromSelection()
{
    if (projectModel == nullptr)
        return;
    
    const auto& selection = projectModel->getSelectionModel();
    const auto selectedTracks = selection.getSelectedTracks();
    const auto selectedClips = selection.getSelectedClips();
    
    currentTrack = nullptr;
    currentClip = nullptr;
    
    if (!selectedTracks.empty())
    {
        currentTrack = projectModel->getTrack(selectedTracks[0]);
    }
    
    if (!selectedClips.empty())
    {
        currentClip = projectModel->getClip(selectedClips[0]);
    }
    
    // Update UI from current selection
    if (currentTrack != nullptr)
    {
        nameEditor.setText(currentTrack->getName(), juce::dontSendNotification);
        colorButton.setColour(juce::TextButton::buttonColourId, currentTrack->getColor());
        gainSlider.setValue(currentTrack->getGainDb(), juce::dontSendNotification);
        panSlider.setValue(currentTrack->getPan(), juce::dontSendNotification);
    }
    else if (currentClip != nullptr)
    {
        nameEditor.setText(currentClip->getLabel(), juce::dontSendNotification);
        startEditor.setText(juce::String(currentClip->getStartBeats(), 2), juce::dontSendNotification);
        lengthEditor.setText(juce::String(currentClip->getLengthBeats(), 2), juce::dontSendNotification);
    }
    
    repaint();
}

void InspectorPanel::nameChanged()
{
    if (currentTrack != nullptr)
    {
        currentTrack->setName(nameEditor.getText().toStdString());
    }
    else if (currentClip != nullptr)
    {
        currentClip->setLabel(nameEditor.getText().toStdString());
    }
}

void InspectorPanel::colorChanged()
{
    if (currentTrack != nullptr)
    {
        // Simple color cycling for now
        const auto colors = std::vector<juce::Colour>{
            juce::Colours::red, juce::Colours::blue, juce::Colours::green,
            juce::Colours::yellow, juce::Colours::cyan, juce::Colours::magenta
        };
        const auto currentColor = currentTrack->getColor();
        auto it = std::find(colors.begin(), colors.end(), currentColor);
        if (it != colors.end() && std::next(it) != colors.end())
            currentTrack->setColor(*std::next(it));
        else
            currentTrack->setColor(colors[0]);
        
        colorButton.setColour(juce::TextButton::buttonColourId, currentTrack->getColor());
    }
}

void InspectorPanel::gainChanged()
{
    if (currentTrack != nullptr)
    {
        const auto gainDb = static_cast<float>(gainSlider.getValue());
        currentTrack->setGainDb(gainDb);
        
        // Update engine
        if (engineContext)
        {
            // Find track index in engine
            const auto tracks = projectModel->getTracks();
            for (size_t i = 0; i < tracks.size(); ++i)
            {
                if (tracks[i]->getId() == currentTrack->getId())
                {
                    engineContext->setTrackGain(static_cast<int>(i), gainDb);
                    break;
                }
            }
        }
    }
}

void InspectorPanel::panChanged()
{
    if (currentTrack != nullptr)
    {
        const auto pan = static_cast<float>(panSlider.getValue());
        currentTrack->setPan(pan);
        
        // Update engine
        if (engineContext)
        {
            // Find track index in engine
            const auto tracks = projectModel->getTracks();
            for (size_t i = 0; i < tracks.size(); ++i)
            {
                if (tracks[i]->getId() == currentTrack->getId())
                {
                    engineContext->setTrackPan(static_cast<int>(i), pan);
                    break;
                }
            }
        }
    }
}

void InspectorPanel::startChanged()
{
    if (currentClip != nullptr)
    {
        const auto start = startEditor.getText().getDoubleValue();
        currentClip->setStartBeats(start);
    }
}

void InspectorPanel::lengthChanged()
{
    if (currentClip != nullptr)
    {
        const auto length = lengthEditor.getText().getDoubleValue();
        currentClip->setLengthBeats(length);
    }
}


} // namespace daw::ui::views

