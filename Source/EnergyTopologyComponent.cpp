/*
  ==============================================================================
    EnergyTopologyComponent.cpp (SPLENTA V18.6 - 20251216.08)
    Energy Topology: Radial Gradient Glow Optimization
  ==============================================================================
*/

#include "EnergyTopologyComponent.h"

EnergyTopologyComponent::EnergyTopologyComponent()
{
    // Initialize particles
    juce::Random random;
    particles.reserve(numParticles);
    particleOffsets.reserve(numParticles);

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

        // Initialize scatter offsets
        particleOffsets.push_back(juce::Point<float>(0.0f, 0.0f));
    }

    // Initialize sakura trail
    sakuraTrail.resize(trailLength, juce::Point<float>(0.0f, 0.0f));
    sakuraTrailRotations.resize(trailLength, 0.0f);

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

void EnergyTopologyComponent::setTriggerState(bool isTriggered)
{
    // Detect rising edge (false -> true)
    if (isTriggered && !lastTriggerState)
    {
        // Trigger scatter effect
        scatterAmount = 1.0f;

        // Generate random scatter offsets for each particle
        juce::Random random;
        for (int i = 0; i < numParticles; ++i)
        {
            // Random direction and distance (burst outward)
            float angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
            float distance = 30.0f + random.nextFloat() * 50.0f; // 30-80 pixels
            particleOffsets[i].x = std::cos(angle) * distance;
            particleOffsets[i].y = std::sin(angle) * distance;
        }
    }
    lastTriggerState = isTriggered;
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

    // Decay scatter effect (fast recovery - exponential decay)
    if (scatterAmount > 0.0f)
    {
        scatterAmount *= 0.90f; // Decay by 10% per frame (very fast at 60fps)
        if (scatterAmount < 0.01f)
            scatterAmount = 0.0f;
    }

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
        drawCartesian(g, width, height, cx, cy);
    }
    else {
        // Default: Bronze
        drawMobius(g, width, height, cx, cy);
    }

    // Global glow overlay (radial gradient, matches Web radial-gradient)
    float normalizedIntensity = intensity / 100.0f;
    float glowOpacity = 0.15f + normalizedIntensity * 0.15f; // Subtle glow

    // Radial gradient from center (20% opacity) to 70% radius (transparent)
    float radiusScale = width * 0.7f;
    juce::ColourGradient gradient(
        palette.accent.withAlpha(glowOpacity), cx, cy,
        juce::Colours::transparentBlack, cx + radiusScale, cy,
        true  // radial
    );
    g.setGradientFill(gradient);
    g.fillRect(getLocalBounds());  // Fill entire component (no hard edges)
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

    for (int i = 0; i < particles.size(); ++i)
    {
        auto& p = particles[i];

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

        // Apply scatter effect - radial burst from center
        if (scatterAmount > 0.0f && i < numParticles)
        {
            x2d += particleOffsets[i].x * scatterAmount;
            y2d += particleOffsets[i].y * scatterAmount;
        }

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

            // Apply scatter effect (if active)
            if (scatterAmount > 0.0f)
            {
                int particleIndex = r * cols + c;
                if (particleIndex < numParticles)
                {
                    x2d += particleOffsets[particleIndex].x * scatterAmount;
                    y2d += particleOffsets[particleIndex].y * scatterAmount;
                }
            }

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

        // Apply scatter effect - radial explosion from moon center
        if (scatterAmount > 0.0f && i < numParticles)
        {
            x2d += particleOffsets[i].x * scatterAmount;
            y2d += particleOffsets[i].y * scatterAmount;
        }

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

        float finalX = cx + gx;
        float finalY = cy + gy;

        // Apply scatter effect - network nodes scatter randomly
        if (scatterAmount > 0.0f && i < numParticles)
        {
            finalX += particleOffsets[i].x * scatterAmount;
            finalY += particleOffsets[i].y * scatterAmount;
        }

        nodes.push_back(juce::Point<float>(finalX, finalY));

        // Draw Node
        g.setColour(getColorWithAlpha(0.8f));
        g.fillRect(finalX - 1.5f, finalY - 1.5f, 3.0f, 3.0f);
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

// --- 3D PROJECTION HELPER ---
EnergyTopologyComponent::Projection3D EnergyTopologyComponent::project3D(float x, float y, float z, float cx, float cy)
{
    float rotY = time * 0.3f;
    float rotX = time * 0.2f;

    // Rotate Y
    float tx = x * std::cos(rotY) - z * std::sin(rotY);
    float tz = x * std::sin(rotY) + z * std::cos(rotY);

    // Rotate X
    float ty = y * std::cos(rotX) - tz * std::sin(rotX);
    tz = y * std::sin(rotX) + tz * std::cos(rotX);

    // Perspective
    const float fov = 1000.0f;
    float scale2d = fov / (fov - tz + 0.001f); // Avoid divide by zero

    Projection3D result;
    result.x = cx + tx * scale2d;
    result.y = cy + ty * scale2d;
    result.scale = scale2d;
    result.z = tz;
    return result;
}

// 5. PINK: SAKURA BLOSSOM FLIGHT (Redesigned)
void EnergyTopologyComponent::drawCartesian(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.45f; // Increased from 0.35f for larger stage
    float normalizedIntensity = intensity / 100.0f;

    // 1. Draw Lissajous Curve with horizontal stretch and distortion on trigger
    juce::Path curvePath;
    bool firstPoint = true;
    const int samples = 150;

    for (int i = 0; i <= samples; ++i)
    {
        float t = (i / (float)samples) * juce::MathConstants<float>::twoPi;

        // Lissajous Knot equations with horizontal stretch
        float x3d = scale * (std::sin(t) + 2.0f * std::sin(2.0f * t)) / 2.5f * 1.4f; // 1.4x horizontal stretch
        float y3d = scale * (std::cos(t) - 2.0f * std::cos(2.0f * t)) / 2.5f;
        float z3d = scale * (-std::sin(3.0f * t)) / 2.0f;

        // Apply distortion when scatterAmount > 0 (curve inflation/twist)
        if (scatterAmount > 0.0f)
        {
            float distortion = scatterAmount * 40.0f; // Increased from 30.0f
            float noiseX = std::sin(t * 5.0f + time) * distortion;
            float noiseY = std::cos(t * 7.0f + time * 1.2f) * distortion;
            float noiseZ = std::sin(t * 3.0f + time * 0.8f) * distortion;

            x3d += noiseX;
            y3d += noiseY;
            z3d += noiseZ;
        }

        auto proj = project3D(x3d, y3d, z3d, cx, cy);

        if (firstPoint) {
            curvePath.startNewSubPath(proj.x, proj.y);
            firstPoint = false;
        } else {
            curvePath.lineTo(proj.x, proj.y);
        }
    }

    // Draw curve with glow
    g.setColour(getColorWithAlpha(0.3f + normalizedIntensity * 0.3f));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // 2. Calculate sakura position on curve
    float travelerT = std::fmod(time * 0.5f, juce::MathConstants<float>::twoPi);
    float tx3d = scale * (std::sin(travelerT) + 2.0f * std::sin(2.0f * travelerT)) / 2.5f * 1.4f; // Horizontal stretch
    float ty3d = scale * (std::cos(travelerT) - 2.0f * std::cos(2.0f * travelerT)) / 2.5f;
    float tz3d = scale * (-std::sin(3.0f * travelerT)) / 2.0f;

    auto travelerProj = project3D(tx3d, ty3d, tz3d, cx, cy);

    // Current rotation angle
    float currentRotation = time * 2.0f;

    // Update trail buffer (circular buffer)
    sakuraTrail[trailWriteIndex] = juce::Point<float>(travelerProj.x, travelerProj.y);
    sakuraTrailRotations[trailWriteIndex] = currentRotation;
    trailWriteIndex = (trailWriteIndex + 1) % trailLength;

    // 3. Draw petal trail with Brownian motion and spiral effect
    juce::Random random;
    random.setSeedRandomly();

    for (int i = 0; i < trailLength; ++i)
    {
        int readIndex = (trailWriteIndex + i) % trailLength; // Oldest first
        auto& trailPos = sakuraTrail[readIndex];
        float trailRotation = sakuraTrailRotations[readIndex];

        if (trailPos.x == 0.0f && trailPos.y == 0.0f) continue; // Skip uninitialized

        float trailAge = (float)i / (float)trailLength; // 0.0 = oldest, 1.0 = newest

        // Multiple petals per trail position (3-5 petals for dramatic effect)
        int petalCount = 3 + (int)(trailAge * 2); // More petals for newer positions

        for (int p = 0; p < petalCount; ++p)
        {
            // Brownian motion offset (larger for older petals)
            float brownianScale = (1.0f - trailAge) * 15.0f; // Older petals drift more
            float brownianX = (random.nextFloat() - 0.5f) * brownianScale;
            float brownianY = (random.nextFloat() - 0.5f) * brownianScale;

            // Spiral offset (petals spiral outward as they age)
            float spiralAngle = trailRotation + (p / (float)petalCount) * juce::MathConstants<float>::twoPi;
            float spiralRadius = (1.0f - trailAge) * 8.0f; // Spiral outward
            float spiralX = std::cos(spiralAngle) * spiralRadius;
            float spiralY = std::sin(spiralAngle) * spiralRadius;

            float petalX = trailPos.x + brownianX + spiralX;
            float petalY = trailPos.y + brownianY + spiralY;

            // Fade and size based on age
            float petalAlpha = (1.0f - trailAge) * 0.5f; // Fade more aggressively
            float petalSize = (4.0f + random.nextFloat() * 2.0f) * (1.0f - trailAge * 0.6f);

            // Individual petal rotation
            float petalRotation = spiralAngle + time * 0.5f;

            // Draw petal as ellipse with rotation
            g.setColour(getColorWithAlpha(petalAlpha));
            juce::Path petalPath;
            petalPath.addEllipse(petalX - petalSize * 0.7f, petalY - petalSize,
                                 petalSize * 1.4f, petalSize * 2.0f);

            juce::AffineTransform transform = juce::AffineTransform::rotation(petalRotation, petalX, petalY);
            petalPath.applyTransform(transform);
            g.fillPath(petalPath);
        }
    }

    // 4. Draw main sakura blossom with flash effect (LARGER)
    float beatCycle = std::fmod(time * 1.5f, 2.0f);
    bool isFlash = beatCycle < 0.2f;
    float flashAlpha = isFlash ? 1.0f : 0.7f;

    float sakuraSize = 18.0f + normalizedIntensity * 8.0f; // Increased from 8+4 to 18+8
    float rotation = time * 2.0f; // Gentle rotation

    drawSakura(g, travelerProj.x, travelerProj.y, sakuraSize, rotation, scatterAmount);
}

// Helper: Draw sakura blossom (8 petals for fuller look)
void EnergyTopologyComponent::drawSakura(juce::Graphics& g, float x, float y, float size, float rotation, float scatterAmt)
{
    const int petalCount = 8; // Increased from 5 to 8 for fuller blossom
    const float angleStep = juce::MathConstants<float>::twoPi / petalCount;

    // Petal scatter offsets (when scatterAmt > 0, petals fly apart)
    juce::Random random;

    for (int i = 0; i < petalCount; ++i)
    {
        float angle = rotation + i * angleStep;

        // Petal position offset from center
        float baseOffsetDist = size * 0.5f; // Increased from 0.4f
        float scatterDist = scatterAmt * (30.0f + random.nextFloat() * 25.0f); // Increased scatter range

        float offsetX = std::cos(angle) * (baseOffsetDist + scatterDist);
        float offsetY = std::sin(angle) * (baseOffsetDist + scatterDist);

        float petalX = x + offsetX;
        float petalY = y + offsetY;

        // Draw heart-shaped petal (larger)
        juce::Path petalPath;

        // Heart petal using ellipse approximation
        float petalW = size * 0.6f; // Increased from 0.5f
        float petalH = size * 0.75f; // Increased from 0.6f

        // Create petal as rounded shape pointing outward
        petalPath.addEllipse(petalX - petalW/2, petalY - petalH/2, petalW, petalH);

        // Rotate petal to point outward from center
        juce::AffineTransform rotation = juce::AffineTransform::rotation(angle, petalX, petalY);
        petalPath.applyTransform(rotation);

        // Draw petal with gradient (lighter at tip, darker at base)
        float petalAlpha = 0.85f * (1.0f - scatterAmt * 0.3f); // Slight fade when scattered
        g.setColour(getColorWithAlpha(petalAlpha));
        g.fillPath(petalPath);
    }

    // Draw center of sakura (larger)
    float centerSize = size * 0.35f * (1.0f - scatterAmt * 0.5f); // Increased from 0.25f
    g.setColour(getColorWithAlpha(0.95f));
    g.fillEllipse(x - centerSize, y - centerSize, centerSize * 2.0f, centerSize * 2.0f);
}
