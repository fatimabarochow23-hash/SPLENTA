/*
  ==============================================================================
    EnergyTopologyComponent.cpp (SPLENTA V18.6 - 20251216.06)
    Energy Topology: Mobius Visualizer (Canvas to JUCE Graphics Port)
  ==============================================================================
*/

#include "EnergyTopologyComponent.h"

EnergyTopologyComponent::EnergyTopologyComponent()
{
    // Initialize particles
    juce::Random random;
    particles.reserve(numParticles);

    for (int i = 0; i < numParticles; ++i)
    {
        Particle p;
        p.x = random.nextFloat() * 100.0f;
        p.y = random.nextFloat() * 100.0f;
        p.z = random.nextFloat() * 100.0f;
        p.vx = (random.nextFloat() - 0.5f) * 0.5f;
        p.vy = (random.nextFloat() - 0.5f) * 0.5f;
        p.angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
        p.radius = random.nextFloat() * 20.0f + 10.0f;
        p.offset = random.nextFloat() * juce::MathConstants<float>::twoPi;
        p.speed = 0.005f + random.nextFloat() * 0.01f;
        p.life = random.nextFloat();
        particles.push_back(p);
    }

    // Initialize with default Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // Start animation timer at 60fps
    startTimerHz(60);
}

EnergyTopologyComponent::~EnergyTopologyComponent()
{
    stopTimer();
}

void EnergyTopologyComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void EnergyTopologyComponent::setIntensity(float intensityValue)
{
    intensity = juce::jlimit(0.0f, 100.0f, intensityValue);
}

juce::Colour EnergyTopologyComponent::getColorWithAlpha(float alpha)
{
    return palette.accent.withAlpha(alpha);
}

void EnergyTopologyComponent::timerCallback()
{
    float normalizedIntensity = intensity / 100.0f;
    float speedMultiplier = 1.0f + (normalizedIntensity * 3.0f);
    time += 0.01f * speedMultiplier;
    repaint();
}

void EnergyTopologyComponent::paint(juce::Graphics& g)
{
    float width = (float)getWidth();
    float height = (float)getHeight();
    float cx = width * 0.5f;
    float cy = height * 0.5f;

    // Clear with trails effect (matches Web: rgba(15, 12, 10, 0.25))
    g.setColour(juce::Colour(15, 12, 10).withAlpha(0.25f));
    g.fillRect(getLocalBounds());

    // Get current theme type from palette
    // We need to determine theme by comparing accent color (simple approach)
    // Bronze: FFB045, Blue: 22D3EE, Purple: D8B4FE, Green: 4ADE80, Pink: FF3399
    auto accentRGB = palette.accent.getRed() | (palette.accent.getGreen() << 8) | (palette.accent.getBlue() << 16);

    if (palette.accent.isOpaque() && palette.accent.getGreen() > 200 && palette.accent.getBlue() > 200) {
        // Blue (22D3EE - cyan-ish, high G and B)
        drawWaves(g, width, height, cx, cy);
    }
    else if (palette.accent.getRed() > 200 && palette.accent.getBlue() > 200) {
        // Purple (D8B4FE - high R and B)
        drawMoon(g, width, height, cx, cy);
    }
    else if (palette.accent.getGreen() > 200 && palette.accent.getRed() < 100) {
        // Green (4ADE80 - high G, low R)
        drawNetwork(g, width, height, cx, cy);
    }
    else if (palette.accent.getRed() > 200 && palette.accent.getGreen() < 100) {
        // Pink (FF3399 - high R, low G)
        drawSakura(g, width, height, cx, cy);
    }
    else {
        // Default: Bronze
        drawMobius(g, width, height, cx, cy);
    }

    // Global glow overlay (radial gradient)
    float normalizedIntensity = intensity / 100.0f;
    float glowOpacity = 0.5f + normalizedIntensity / 2.0f;

    juce::ColourGradient gradient(
        palette.glow.withAlpha(glowOpacity * 0.2f), cx, cy,
        juce::Colours::transparentBlack, cx + width * 0.7f, cy,
        true
    );
    g.setGradientFill(gradient);
    g.fillEllipse(cx - width * 0.35f, cy - height * 0.35f, width * 0.7f, height * 0.7f);
}

void EnergyTopologyComponent::resized()
{
    // Component resized - nothing special needed
}

// --- THEME RENDERERS ---

// 1. BRONZE: MOBIUS STRIP
void EnergyTopologyComponent::drawMobius(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.35f;
    float normalizedIntensity = intensity / 100.0f;
    float speedMultiplier = 1.0f + (normalizedIntensity * 3.0f);

    for (auto& p : particles)
    {
        // Update particle angle (Mobius parameter 'u')
        p.angle += p.speed * speedMultiplier;

        float u = p.angle;
        float v = std::sin(p.angle * 2.0f + p.offset) * 0.5f; // Strip width variation

        float radius = scale * (1.0f + v * std::cos(u / 2.0f));
        float x3d = radius * std::cos(u);
        float y3d = radius * std::sin(u);
        float z3d = scale * v * std::sin(u / 2.0f);

        // Rotate the whole strip
        float rotX = time * 0.2f;
        float rotY = time * 0.3f;

        float cosRy = std::cos(rotY);
        float sinRy = std::sin(rotY);
        float xRot = x3d * cosRy - z3d * sinRy;
        float zRot = x3d * sinRy + z3d * cosRy;

        float cosRx = std::cos(rotX);
        float sinRx = std::sin(rotX);
        float yRot = y3d * cosRx - zRot * sinRx;

        float x2d = cx + xRot;
        float y2d = cy + yRot;

        // Depth-based alpha
        float depthAlpha = (zRot + scale) / (2.0f * scale);
        float alpha = juce::jlimit(0.1f, 1.0f, depthAlpha) * (0.5f + normalizedIntensity * 0.5f);

        float size = (2.0f + normalizedIntensity * 2.0f) * alpha;

        g.setColour(getColorWithAlpha(alpha));
        g.fillEllipse(x2d - size, y2d - size, size * 2.0f, size * 2.0f);
    }
}

// 2. BLUE: OCEAN WAVES
void EnergyTopologyComponent::drawWaves(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.4f;
    float normalizedIntensity = intensity / 100.0f;

    const int rows = 15; // Reduced from 20 for performance
    const int cols = 15;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            // Normalized coords -1 to 1
            float u = (c / (float)cols - 0.5f) * 2.0f;
            float v = (r / (float)rows - 0.5f) * 2.0f;

            // Circular Mask
            if (u*u + v*v > 1.0f) continue;

            // Wave height function
            float dist = std::sqrt(u*u + v*v);
            float heightVal = std::sin(dist * 5.0f - time * 2.0f) * std::cos(u * 5.0f + time) * 0.5f * (0.2f + normalizedIntensity);

            // 3D Projection
            float x3d = u * scale;
            float z3d = v * scale;
            float y3d = heightVal * scale;

            // Rotate View
            float rotAngle = time * 0.1f;
            float xRot = x3d * std::cos(rotAngle) - z3d * std::sin(rotAngle);
            float zRot = x3d * std::sin(rotAngle) + z3d * std::cos(rotAngle);

            // Tilt camera
            float tilt = 0.5f;
            float yFinal = y3d * std::cos(tilt) - zRot * std::sin(tilt);
            float zFinal = y3d * std::sin(tilt) + zRot * std::cos(tilt);

            float persp = 1000.0f / (1000.0f - zFinal);
            float x2d = cx + xRot * persp;
            float y2d = cy + yFinal * persp;

            float alpha = ((zFinal + scale) / (scale * 2.0f)) * 0.8f + 0.2f;
            float size = 2.0f * persp + (heightVal * 10.0f * normalizedIntensity);

            g.setColour(getColorWithAlpha(alpha));
            g.fillEllipse(x2d - size, y2d - size, juce::jmax(1.0f, size) * 2.0f, juce::jmax(1.0f, size) * 2.0f);
        }
    }
}

// 3. PURPLE: CYBER MOON
void EnergyTopologyComponent::drawMoon(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.3f;
    float normalizedIntensity = intensity / 100.0f;

    for (int i = 0; i < numParticles; ++i)
    {
        // Sphere Surface - Golden Spiral distribution
        float phi = std::acos(1.0f - 2.0f * (i + 0.5f) / numParticles);
        float theta = juce::MathConstants<float>::pi * (1.0f + std::sqrt(5.0f)) * (i + 0.5f);

        float r = scale;
        float x3d = r * std::sin(phi) * std::cos(theta);
        float y3d = r * std::sin(phi) * std::sin(theta);
        float z3d = r * std::cos(phi);

        // Rotate Sphere
        float rotY = time * 0.5f;
        float rotZ = 0.2f;

        // Y Rotation
        float tx = x3d * std::cos(rotY) - z3d * std::sin(rotY);
        float tz = x3d * std::sin(rotY) + z3d * std::cos(rotY);
        x3d = tx; z3d = tz;

        // Z Tilt
        float ty = y3d * std::cos(rotZ) - x3d * std::sin(rotZ);
        tx = y3d * std::sin(rotZ) + x3d * std::cos(rotZ);
        x3d = tx; y3d = ty;

        float x2d = cx + x3d;
        float y2d = cy + y3d;

        // Draw Sphere Dot
        if (z3d < 0.0f) {
            g.setColour(getColorWithAlpha(0.1f));
        } else {
            float alpha = 0.3f + (z3d / scale) * 0.7f;
            g.setColour(getColorWithAlpha(alpha));
        }
        float dotSize = 1.5f + normalizedIntensity;
        g.fillEllipse(x2d - dotSize, y2d - dotSize, dotSize * 2.0f, dotSize * 2.0f);
    }

    // Orbital Ring
    g.setColour(getColorWithAlpha(0.4f + normalizedIntensity * 0.4f));
    juce::Path ringPath;
    bool firstPoint = true;

    for (float a = 0; a <= juce::MathConstants<float>::twoPi; a += 0.1f)
    {
        float ringR = scale * 1.6f;
        float rx = ringR * std::cos(a);
        float ry = 0.0f;
        float rz = ringR * std::sin(a);

        // Wobble ring with audio
        ry += std::sin(a * 5.0f + time * 5.0f) * (10.0f * normalizedIntensity);

        // Same rotation as sphere but slower
        float rotY = time * 0.2f;
        float rotZ = 0.4f;

        float tx = rx * std::cos(rotY) - rz * std::sin(rotY);
        float tz = rx * std::sin(rotY) + rz * std::cos(rotY);
        rx = tx; rz = tz;

        float ty = ry * std::cos(rotZ) - rx * std::sin(rotZ);
        tx = ry * std::sin(rotZ) + rx * std::cos(rotZ);
        rx = tx; ry = ty;

        if (firstPoint) {
            ringPath.startNewSubPath(cx + rx, cy + ry);
            firstPoint = false;
        } else {
            ringPath.lineTo(cx + rx, cy + ry);
        }
    }
    ringPath.closeSubPath();
    g.strokePath(ringPath, juce::PathStrokeType(2.0f));
}

// 4. GREEN: HACKER NETWORK
void EnergyTopologyComponent::drawNetwork(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.45f;
    float normalizedIntensity = intensity / 100.0f;

    const int nodeCount = 60;
    std::vector<juce::Point<float>> nodes;
    nodes.reserve(nodeCount);

    juce::Random random;

    for (int i = 0; i < nodeCount; ++i)
    {
        auto& p = particles[i];

        // Move particles in pseudo-random box
        float x = std::sin(time * p.speed * 20.0f + i) * scale * 1.5f;
        float y = std::cos(time * p.speed * 15.0f + i * 2.0f) * scale * 0.8f;

        // Glitch jump on beat
        float gx = x;
        float gy = y;
        if (normalizedIntensity > 0.5f && random.nextFloat() > 0.9f) {
            gx += (random.nextFloat() - 0.5f) * 50.0f;
        }

        nodes.push_back(juce::Point<float>(cx + gx, cy + gy));

        // Draw Node
        g.setColour(getColorWithAlpha(0.8f));
        g.fillRect(cx + gx - 1.5f, cy + gy - 1.5f, 3.0f, 3.0f);
    }

    // Connect Neighbors
    g.setColour(getColorWithAlpha(0.3f + normalizedIntensity * 0.5f));
    for (int i = 0; i < (int)nodes.size(); ++i)
    {
        for (int j = i + 1; j < (int)nodes.size(); ++j)
        {
            float dx = nodes[i].getX() - nodes[j].getX();
            float dy = nodes[i].getY() - nodes[j].getY();
            float dist = std::sqrt(dx*dx + dy*dy);

            // Connection threshold increases with intensity
            float thresh = 60.0f + normalizedIntensity * 60.0f;

            if (dist < thresh) {
                g.drawLine(nodes[i].getX(), nodes[i].getY(), nodes[j].getX(), nodes[j].getY(), 1.0f);
            }
        }
    }
}

// 5. PINK: SAKURA VORTEX
void EnergyTopologyComponent::drawSakura(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.4f;
    float normalizedIntensity = intensity / 100.0f;

    juce::Random random;

    for (int i = 0; i < numParticles; ++i)
    {
        auto& p = particles[i];

        // Spiral Motion
        p.life -= 0.005f;
        if (p.life < 0.0f) {
            p.life = 1.0f;
            p.radius = random.nextFloat() * scale * 1.5f;
        }

        float r = p.radius * p.life; // Spiral in
        float angle = p.angle + time * (1.0f + normalizedIntensity);

        float x = cx + r * std::cos(angle);
        float y = cy + r * std::sin(angle);

        // Petal orientation (tumbling)
        float tumble = time * 5.0f + i;

        // Draw Petal (Simple Ellipse)
        float alpha = std::sin(p.life * juce::MathConstants<float>::pi); // Fade in/out
        g.setColour(getColorWithAlpha(alpha * 0.8f));

        juce::AffineTransform transform = juce::AffineTransform::rotation(tumble, x, y);
        juce::Path petalPath;
        petalPath.addEllipse(-( 4.0f + normalizedIntensity * 2.0f), -2.0f,
                            (4.0f + normalizedIntensity * 2.0f) * 2.0f, 4.0f);
        petalPath.applyTransform(transform.translated(x, y));
        g.fillPath(petalPath);
    }
}
