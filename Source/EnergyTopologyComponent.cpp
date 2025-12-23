/*
  ==============================================================================
    EnergyTopologyComponent.cpp (SPLENTA V19.5 - 20251224.01)
    Energy Topology: Wormhole + Figure-8 Lemniscate Implementation
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

    // Initialize healing light beams (empty, spawned dynamically)
    healingBeams.clear();

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

void EnergyTopologyComponent::setBypassState(bool bypassed)
{
    isBypassed = bypassed;
}

void EnergyTopologyComponent::setSaturation(float satValue)
{
    saturation = juce::jlimit(0.0f, 100.0f, satValue);
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

        // Clear existing healing beams (will respawn during recovery)
        healingBeams.clear();
        beamSpawnTimer = 0.0f;
    }
    lastTriggerState = isTriggered;
}

juce::Colour EnergyTopologyComponent::getColorWithAlpha(float alpha)
{
    return palette.accent.withAlpha(alpha);
}

float EnergyTopologyComponent::getParticleSizeMultiplier() const
{
    float sizeMultiplier = 1.0f;

    // Bypass: reduce particle size to 70% (smaller, calmer appearance)
    if (isBypassed)
    {
        sizeMultiplier *= 0.7f;
    }

    // Saturation: increase size up to +20% at max SAT (subtle growth)
    float normalizedSat = saturation / 100.0f;
    sizeMultiplier *= (1.0f + normalizedSat * 0.2f);

    return sizeMultiplier;
}

void EnergyTopologyComponent::timerCallback()
{
    float normalizedIntensity = intensity / 100.0f;
    float normalizedSat = saturation / 100.0f;  // 0.0 to 1.0

    // Base speed multiplier from intensity
    float speedMultiplier = 1.0f + (normalizedIntensity * 3.0f);

    // Apply bypass slow-down: reduce speed to 30% when bypassed
    if (isBypassed)
    {
        speedMultiplier *= 0.3f;  // Gentle, calm motion
    }

    // Apply saturation speed boost: up to +30% speed at max SAT
    speedMultiplier *= (1.0f + normalizedSat * 0.3f);

    time += 0.01f * speedMultiplier;

    // Decay scatter effect (slower recovery for stronger impact feel)
    if (scatterAmount > 0.0f)
    {
        scatterAmount *= 0.95f; // Decay by 5% per frame (doubled recovery time at 60fps)
        if (scatterAmount < 0.01f)
            scatterAmount = 0.0f;

        // Spawn healing light beams during recovery (when scatter is decaying)
        beamSpawnTimer += 0.016f; // ~60fps
        if (beamSpawnTimer > 0.05f && healingBeams.size() < 20) // Spawn every 50ms, max 20 beams
        {
            juce::Random random;
            LightBeam beam;
            // Random position around heart base (circular distribution)
            float angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
            float radius = 40.0f + random.nextFloat() * 30.0f; // 40-70 pixels from center
            beam.x = std::cos(angle) * radius;
            beam.z = std::sin(angle) * radius;
            beam.height = 0.0f;
            beam.speed = 0.015f + random.nextFloat() * 0.010f; // Rise speed: 0.015-0.025
            beam.alpha = 0.6f + random.nextFloat() * 0.4f; // Initial alpha: 0.6-1.0
            beam.phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
            healingBeams.push_back(beam);
            beamSpawnTimer = 0.0f;
        }
    }

    // Update healing light beams
    for (auto it = healingBeams.begin(); it != healingBeams.end();)
    {
        it->height += it->speed;
        it->alpha *= 0.98f; // Fade out as it rises

        // Remove beam when it reaches top or becomes invisible
        if (it->height > 1.2f || it->alpha < 0.05f)
        {
            it = healingBeams.erase(it);
        }
        else
        {
            ++it;
        }
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
    // Bronze: FFB045, Blue: 22D3EE, Purple: D8B4FE, Green: 4ADE80, Pink: f0a5c2
    int redVal = palette.accent.getRed();
    int greenVal = palette.accent.getGreen();
    int blueVal = palette.accent.getBlue();

    // Use more robust color distance matching
    if (greenVal > 200 && blueVal > 200 && redVal < 100) {
        // Blue (22D3EE - cyan: R:34, G:211, B:238)
        drawWaves(g, width, height, cx, cy);
    }
    else if (redVal > 200 && blueVal > 240 && greenVal > 150 && greenVal < 200) {
        // Purple (D8B4FE - R:216, G:180, B:254) - high R, very high B, medium G
        drawMoon(g, width, height, cx, cy);
    }
    else if (greenVal > 200 && redVal < 100 && blueVal < 200) {
        // Green (4ADE80 - R:74, G:222, B:128)
        drawNetwork(g, width, height, cx, cy);
    }
    else if (redVal > 220 && greenVal > 150 && greenVal < 180 && blueVal > 180 && blueVal < 210) {
        // Pink (f0a5c2 - R:240, G:165, B:194) - high R, medium G, medium-high B
        drawCartesian(g, width, height, cx, cy);
    }
    else {
        // Default: Bronze (FFB045 - R:255, G:176, B:69)
        drawMobius(g, width, height, cx, cy);
    }

    // Global glow overlay (radial gradient, matches Web radial-gradient)
    float normalizedIntensity = intensity / 100.0f;
    float glowOpacity = 0.15f + normalizedIntensity * 0.15f; // Subtle glow

    // Radial gradient - use larger radius to ensure full coverage
    float radiusScale = std::sqrt(width * width + height * height) * 0.8f;  // Increased to 0.8 for full coverage
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

// 1. BRONZE: INFINITY SYMBOL / FIGURE-8 MOBIUS
void EnergyTopologyComponent::drawMobius(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float scale = juce::jmin(width, height) * 0.63f;  // Increased 1.5x (0.42 * 1.5 = 0.63)
    float normalizedIntensity = intensity / 100.0f;
    float speedMultiplier = 1.0f + (normalizedIntensity * 3.0f);

    for (int i = 0; i < particles.size(); ++i)
    {
        auto& p = particles[i];

        // Update particle angle
        p.angle += p.speed * speedMultiplier;

        float u = p.angle;

        // Figure-8 / Lemniscate curve with 3D spiral twist
        float sinU = std::sin(u);
        float cosU = std::cos(u);
        float denominator = 1.0f + sinU * sinU;

        // Lemniscate parametric equations
        float x3d = scale * cosU / denominator;
        float y3d = scale * sinU * cosU / denominator;

        // Add 3D spiral twist (Mobius effect)
        // The strip twists as it goes around, creating depth even when viewed from side
        float stripWidth = std::sin(p.angle * 2.0f + p.offset) * 16.0f;  // Strip thickness
        float twistPhase = u / 2.0f;  // Half-twist for Mobius

        // Add Z-offset that follows the curve to create 3D depth
        float z3d = stripWidth * std::sin(twistPhase);

        // Add vertical wave to prevent collapsing to a line from any angle
        y3d += stripWidth * 0.3f * std::cos(twistPhase);  // Vertical modulation

        // Rotate the whole figure-8
        float rotX = time * 0.25f;
        float rotY = time * 0.2f;

        // Y Rotation
        float cosRy = std::cos(rotY);
        float sinRy = std::sin(rotY);
        float xRot = x3d * cosRy - z3d * sinRy;
        float zRot = x3d * sinRy + z3d * cosRy;

        // X Rotation
        float cosRx = std::cos(rotX);
        float sinRx = std::sin(rotX);
        float yRot = y3d * cosRx - zRot * sinRx;
        float zFinal = y3d * sinRx + zRot * cosRx;

        float x2d = cx + xRot;
        float y2d = cy + yRot;

        // Apply scatter effect
        if (scatterAmount > 0.0f && i < numParticles)
        {
            x2d += particleOffsets[i].x * scatterAmount;
            y2d += particleOffsets[i].y * scatterAmount;
        }

        // Depth-based alpha
        float depthAlpha = (zFinal + scale) / (2.0f * scale);
        float alpha = juce::jlimit(0.1f, 1.0f, depthAlpha) * (0.5f + normalizedIntensity * 0.5f);

        // Particle size with bypass/SAT modulation
        float baseSize = 2.0f + normalizedIntensity * 2.0f;
        float size = baseSize * alpha * getParticleSizeMultiplier();

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
            float baseSize = 2.0f * persp + (heightVal * 10.0f * normalizedIntensity);
            float size = baseSize * getParticleSizeMultiplier();

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

    const int moonParticles = 400;  // Original particle count for moon
    for (int i = 0; i < moonParticles; ++i)
    {
        // Sphere Surface - Golden Spiral distribution
        float phi = std::acos(1.0f - 2.0f * (i + 0.5f) / moonParticles);
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
        if (scatterAmount > 0.0f)
        {
            int offsetIndex = i % numParticles;
            x2d += particleOffsets[offsetIndex].x * scatterAmount;
            y2d += particleOffsets[offsetIndex].y * scatterAmount;
        }

        // Draw Sphere Dot
        if (z3d < 0.0f) {
            g.setColour(getColorWithAlpha(0.1f));
        } else {
            float alpha = 0.3f + (z3d / scale) * 0.7f;
            g.setColour(getColorWithAlpha(alpha));
        }
        float baseDotSize = 1.5f + normalizedIntensity;
        float dotSize = baseDotSize * getParticleSizeMultiplier();
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

        // Draw Node (square shape)
        g.setColour(getColorWithAlpha(0.8f));
        float baseNodeSize = 1.5f;
        float nodeSize = baseNodeSize * getParticleSizeMultiplier();
        g.fillRect(finalX - nodeSize, finalY - nodeSize, nodeSize * 2.0f, nodeSize * 2.0f);
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

// 5. PINK: WORMHOLE (Pink Einstein-Rosen Bridge - Vertical)
void EnergyTopologyComponent::drawCartesian(juce::Graphics& g, float width, float height, float cx, float cy)
{
    float normalizedIntensity = intensity / 100.0f;

    // Wormhole geometry parameters (scaled down 20%)
    const float tunnelWaist = 58.0f;   // Throat radius (72 * 0.8)
    const float flareFactor = 2.2f;    // Steeper expansion
    const int longitudinal = 35;       // Vertical resolution

    // Main rotation
    float mainRotY = time * 0.4f;
    float mainRotX = 0.55f + std::sin(time * 0.15f) * 0.1f; // Restored tilt

    // Breathing pulse
    float pulse = 1.0f + normalizedIntensity * 0.14f * std::sin(time * 2.5f);

    // Generate both halves: side = -1 (black hole), side = 1 (white hole)
    for (int side : {-1, 1})
    {
        for (int j = 0; j < longitudinal; ++j)
        {
            float v = j / (float)(longitudinal - 1); // 0.0 to 1.0
            float y3d = v * 168.0f * side;           // Vertical position (210 * 0.8)

            // Core radius equation: throat + exponential expansion
            float r = tunnelWaist + std::pow(v, flareFactor) * 106.0f; // Opening expansion (132 * 0.8)

            // Dynamic particle count per ring (denser at edges)
            int particlesInRing = 18 + (int)(v * 48.0f);

            for (int i = 0; i < particlesInRing; ++i)
            {
                float u = (i / (float)particlesInRing) * juce::MathConstants<float>::twoPi;

                // Edge flow (fluid-like wave)
                float wave = std::sin(u * 6.0f + v * 8.0f + time) * 5.0f * v;
                float currentR = (r + wave) * pulse;

                // 3D coordinates
                float x3d = std::cos(u) * currentR;
                float z3d = std::sin(u) * currentR;

                // Y-axis rotation
                float xRot = x3d * std::cos(mainRotY) - z3d * std::sin(mainRotY);
                float zRot = x3d * std::sin(mainRotY) + z3d * std::cos(mainRotY);

                // X-axis tilt
                float yRot = y3d * std::cos(mainRotX) - zRot * std::sin(mainRotX);
                float zFinal = y3d * std::sin(mainRotX) + zRot * std::cos(mainRotX);

                // Perspective projection
                float fov = 800.0f;
                float perspective = fov / (fov + zFinal + 400.0f);

                float x2d = cx + xRot * perspective;
                float y2d = cy + yRot * perspective;

                // Apply scatter effect (use modulo to map to available particle offsets)
                if (scatterAmount > 0.0f)
                {
                    int offsetIndex = (j * 66 + i) % numParticles;  // Hash particles to offsets
                    x2d += particleOffsets[offsetIndex].x * scatterAmount;
                    y2d += particleOffsets[offsetIndex].y * scatterAmount;
                }

                // Brightness: brighter near throat (center)
                float brightness = 0.25f + (1.0f - v) * 0.75f;

                // Depth culling
                bool isBack = zFinal > 30.0f;
                float baseSize = (1.3f + normalizedIntensity * 1.5f) * perspective;
                float size = isBack ? baseSize * 0.5f : baseSize;

                // Opacity
                float opacity = (brightness * 0.4f + normalizedIntensity * 0.6f) * perspective * (isBack ? 0.2f : 1.0f);

                g.setColour(getColorWithAlpha(opacity));

                // Energy crystals (every 14th particle)
                int globalIndex = (j * 66 + i);  // Global particle index
                if (globalIndex % 14 == 0 && !isBack)
                {
                    // Draw diamond with glow
                    juce::Path diamond;
                    float crystalSize = size * 1.8f;
                    diamond.startNewSubPath(x2d, y2d - crystalSize);
                    diamond.lineTo(x2d + crystalSize * 0.7f, y2d);
                    diamond.lineTo(x2d, y2d + crystalSize);
                    diamond.lineTo(x2d - crystalSize * 0.7f, y2d);
                    diamond.closeSubPath();
                    g.fillPath(diamond);
                }
                else
                {
                    // Regular particle (square for performance)
                    g.fillRect(x2d - size * 0.5f, y2d - size * 0.5f, size, size);
                }
            }
        }
    }

    // Central glow removed to avoid "pillar" effect in center
}

