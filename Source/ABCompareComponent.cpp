/*
  ==============================================================================
    ABCompareComponent.cpp (SPLENTA V19.5 - 20251223.02)
    A/B Compare - Rainbow Color Cycling Pyramid Animation
  ==============================================================================
*/

#include "ABCompareComponent.h"
#include "PluginProcessor.h"

ABCompareComponent::ABCompareComponent(NewProjectAudioProcessor& processor)
    : audioProcessor(processor)
{
    // Initialize with default Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // Enable mouse interaction
    setInterceptsMouseClicks(true, false);
}

ABCompareComponent::~ABCompareComponent()
{
    stopTimer();
}

void ABCompareComponent::paint(juce::Graphics& g)
{
    updateButtonBounds();  // Ensure bounds are current

    // Draw A button
    drawButton(g, buttonABounds, "A", isStateA, isHoveringA);

    // Draw pyramid button (3D wireframe tetrahedron)
    juce::Colour pyramidColor = isAnimating ? animationColor :
                                (isHoveringPyramid ? juce::Colours::white.withAlpha(0.9f) :
                                                     juce::Colours::white.withAlpha(0.7f));  // Brighter default color
    drawPyramid(g, pyramidBounds, rotationAngle, pyramidColor, isHoveringPyramid);

    // Draw B button
    drawButton(g, buttonBBounds, "B", !isStateA, isHoveringB);
}

void ABCompareComponent::updateButtonBounds()
{
    auto bounds = getLocalBounds().toFloat();
    float height = bounds.getHeight();

    // Layout: [A button] [pyramid] [B button]
    // A and B buttons: 20px each, pyramid: 24px, gaps: 4px each
    const float gap = 4.0f;
    const float buttonWidth = 20.0f;
    const float pyramidSize = 24.0f;

    float xPos = 0.0f;

    // A button (left)
    buttonABounds = juce::Rectangle<float>(xPos, 0, buttonWidth, height);
    xPos += buttonWidth + gap;

    // Pyramid button (center)
    pyramidBounds = juce::Rectangle<float>(xPos, 0, pyramidSize, height);
    xPos += pyramidSize + gap;

    // B button (right)
    buttonBBounds = juce::Rectangle<float>(xPos, 0, buttonWidth, height);
}

void ABCompareComponent::drawButton(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text, bool isActive, bool isHovered)
{
    // Determine text color based on state (no background/border)
    juce::Colour textColor;

    if (isActive)
    {
        // Active state: Bright accent color
        textColor = palette.accent.brighter(0.4f);
    }
    else if (isHovered)
    {
        // Hovered but not active: White highlight
        textColor = juce::Colours::white.withAlpha(0.8f);
    }
    else
    {
        // Inactive: Dim white
        textColor = juce::Colours::white.withAlpha(0.5f);
    }

    // Draw text only (no background/border)
    g.setColour(textColor);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));  // Larger font size
    g.drawText(text, bounds, juce::Justification::centred);
}

void ABCompareComponent::drawPyramid(juce::Graphics& g, juce::Rectangle<float> bounds, float rotation, juce::Colour color, bool isHovered)
{
    // 3D wireframe regular tetrahedron (pyramid with equilateral triangle base)
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float size = bounds.getHeight() * 0.85f;  // Larger pyramid size (was 0.6f)

    // Define 4 vertices of regular tetrahedron in 3D space
    // Add base angle (45 degrees) for better 3D view + animation rotation
    float baseAngle = 45.0f;  // Initial tilt angle (changed from 30 to 45 for better visibility)
    float totalRotation = baseAngle + rotation;
    float rotRad = juce::degreesToRadians(totalRotation);
    float cosR = std::cos(rotRad);
    float sinR = std::sin(rotRad);

    // Regular tetrahedron vertices (centered at origin, normalized)
    struct Vertex3D { float x, y, z; };
    Vertex3D vertices[4] = {
        {0.0f, -0.5f, 0.0f},           // Top vertex
        {-0.433f, 0.25f, -0.25f},      // Base vertex 1
        {0.433f, 0.25f, -0.25f},       // Base vertex 2
        {0.0f, 0.25f, 0.5f}            // Base vertex 3
    };

    // Apply rotation around Y-axis and project to 2D
    juce::Point<float> projected[4];
    for (int i = 0; i < 4; ++i)
    {
        // Rotate around Y-axis
        float x = vertices[i].x * cosR + vertices[i].z * sinR;
        float y = vertices[i].y;
        // float z = -vertices[i].x * sinR + vertices[i].z * cosR;  // Not used in simple orthographic projection

        // Simple orthographic projection
        projected[i].x = cx + x * size;
        projected[i].y = cy + y * size;
    }

    // Draw edges (6 edges for tetrahedron)
    g.setColour(color);
    float lineThickness = isHovered ? 2.5f : 2.0f;  // Thicker lines for better visibility

    // Base triangle (vertices 1, 2, 3)
    g.drawLine(projected[1].x, projected[1].y, projected[2].x, projected[2].y, lineThickness);
    g.drawLine(projected[2].x, projected[2].y, projected[3].x, projected[3].y, lineThickness);
    g.drawLine(projected[3].x, projected[3].y, projected[1].x, projected[1].y, lineThickness);

    // Edges from top vertex to base (0 to 1, 2, 3)
    g.drawLine(projected[0].x, projected[0].y, projected[1].x, projected[1].y, lineThickness);
    g.drawLine(projected[0].x, projected[0].y, projected[2].x, projected[2].y, lineThickness);
    g.drawLine(projected[0].x, projected[0].y, projected[3].x, projected[3].y, lineThickness);

    // Draw glow effect during animation
    if (isAnimating)
    {
        juce::DropShadow glow(color.withAlpha(0.6f), 6, juce::Point<int>(0, 0));
        for (int i = 0; i < 4; ++i)
        {
            juce::Path vertexPath;
            vertexPath.addEllipse(projected[i].x - 2, projected[i].y - 2, 4, 4);
            glow.drawForPath(g, vertexPath);
        }
    }
}

void ABCompareComponent::timerCallback()
{
    if (!isAnimating)
    {
        stopTimer();
        return;
    }

    // Update rotation angle
    const float rotationSpeed = 24.0f;  // Degrees per frame (60 fps = 1440°/sec = 2 rotations/sec)

    if (isClockwise)
    {
        rotationAngle += rotationSpeed;
        if (rotationAngle >= targetRotation)
        {
            rotationAngle = 0.0f;
            isAnimating = false;
            stopTimer();
        }
    }
    else
    {
        rotationAngle -= rotationSpeed;
        if (rotationAngle <= targetRotation)
        {
            rotationAngle = 0.0f;
            isAnimating = false;
            stopTimer();
        }
    }

    // Update animation color based on rotation progress (cycle through 5 theme colors)
    // 720° total rotation, divide into 5 segments (144° each)
    float progress = std::abs(rotationAngle) / 720.0f;  // 0.0 to 1.0
    int colorIndex;

    if (isClockwise)
    {
        // Left-click: Bronze → Blue → Purple → Green → Pink
        colorIndex = static_cast<int>(progress * 5.0f) % 5;
    }
    else
    {
        // Right-click: Pink → Green → Purple → Blue → Bronze (reverse)
        colorIndex = 4 - (static_cast<int>(progress * 5.0f) % 5);
    }

    // Get color from theme palette (using accent color)
    animationColor = ThemePalette::getPaletteByIndex(colorIndex).accent;

    repaint();
}

void ABCompareComponent::resized()
{
    // Bounds calculated in updateButtonBounds()
}

void ABCompareComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void ABCompareComponent::mouseMove(const juce::MouseEvent& event)
{
    updateButtonBounds();
    auto pos = event.position;

    bool wasHoveringA = isHoveringA;
    bool wasHoveringPyramid = isHoveringPyramid;
    bool wasHoveringB = isHoveringB;

    isHoveringA = buttonABounds.contains(pos);
    isHoveringPyramid = pyramidBounds.contains(pos);
    isHoveringB = buttonBBounds.contains(pos);

    if (wasHoveringA != isHoveringA || wasHoveringPyramid != isHoveringPyramid || wasHoveringB != isHoveringB)
    {
        repaint();
    }
}

void ABCompareComponent::mouseExit(const juce::MouseEvent& event)
{
    if (isHoveringA || isHoveringPyramid || isHoveringB)
    {
        isHoveringA = false;
        isHoveringPyramid = false;
        isHoveringB = false;
        repaint();
    }
}

void ABCompareComponent::mouseDown(const juce::MouseEvent& event)
{
    updateButtonBounds();
    auto pos = event.position;

    bool isRightClick = event.mods.isRightButtonDown();

    // Handle A button click
    if (buttonABounds.contains(pos))
    {
        if (!isStateA)
        {
            isStateA = true;
            audioProcessor.switchToStateA();
            repaint();
        }
    }
    // Handle B button click
    else if (buttonBBounds.contains(pos))
    {
        if (isStateA)
        {
            isStateA = false;
            audioProcessor.switchToStateB();
            repaint();
        }
    }
    // Handle pyramid button click
    else if (pyramidBounds.contains(pos))
    {
        if (isRightClick)
        {
            // Right-click: B → A (counter-clockwise, color cycle: Pink → Green → Purple → Blue → Bronze)
            audioProcessor.copyBtoA();
            isClockwise = false;
            targetRotation = -720.0f;
            animationColor = ThemePalette::getPaletteByIndex(4).accent;  // Start with Pink
        }
        else
        {
            // Left-click: A → B (clockwise, color cycle: Bronze → Blue → Purple → Green → Pink)
            audioProcessor.copyAtoB();
            isClockwise = true;
            targetRotation = 720.0f;
            animationColor = ThemePalette::getPaletteByIndex(0).accent;  // Start with Bronze
        }

        // Start animation
        rotationAngle = 0.0f;
        isAnimating = true;
        startTimerHz(60);  // 60 FPS for smooth animation
        repaint();
    }
}
