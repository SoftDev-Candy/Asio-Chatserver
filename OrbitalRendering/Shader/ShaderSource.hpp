//
// Created by Candy on 4/27/2026.
//

#ifndef SOUL_SHADERSOURCE_HPP
#define SOUL_SHADERSOURCE_HPP

// Vertex shader for both Earth and the reused satellite mesh.
// It passes UVs, world position, and normals down to the fragment shader.
const char* VertexShader()
{
    return R"(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec3 aNormal;

        uniform mat4 uMVP;
        uniform mat4 uModel;
        uniform mat3 uNormalMatrix;

        out vec2 vUV;
        out vec3 vWorldPos;
        out vec3 vNormal;

        void main()
        {
            vUV = aUV;

            vec4 worldPos = uModel * vec4(aPos, 1.0);
            vWorldPos = worldPos.xyz;

            vNormal = normalize(uNormalMatrix * aNormal);

            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";
}

// Fragment shader for the textured planet/satellite pass.
// For Earth we use both day and night maps.
// For satellites we just bind the same texture slot to both uniforms and move on with life.
// TODO: Split this into a dedicated Earth shader and a dedicated satellite shader later.
const char* FragmentShader()
{
    return R"(
        #version 330 core

        in vec2 vUV;
        in vec3 vWorldPos;
        in vec3 vNormal;

        out vec4 FragColor;

        uniform sampler2D uDayMap;
        uniform sampler2D uNightMap;

        uniform vec3 uSunDir;
        uniform vec3 uViewPos;
        uniform float uAmbientFloor;
        uniform float uNightStrength;
        uniform float uNightBlend;
        uniform vec3 uTintColor;
        uniform float uTintStrength;

        void main()
        {
            vec3 N = normalize(vNormal);
            // The light/night split looked backwards in this scene, so use the sun direction directly here.
            // Simple day/night blend for now.
            vec3 L = normalize(uSunDir);

            float lightAmount = dot(N, L);
            float dayAmount = smoothstep(-0.01, 0.01, lightAmount);

            vec3 dayColor = texture(uDayMap, vUV).rgb;
            vec3 nightColor = texture(uNightMap, vUV).rgb;

            float diffuse = clamp(lightAmount, 0.0, 1.0);
            vec3 litDay = dayColor * (uAmbientFloor + (1.0 - uAmbientFloor) * diffuse);
            vec3 litNight = clamp(nightColor * uNightStrength, 0.0, 1.0);
            vec3 planetBlend = mix(litNight, litDay, dayAmount);
            vec3 finalColor = mix(litDay, planetBlend, uNightBlend);
            finalColor = mix(finalColor, finalColor * uTintColor + (uTintColor * 0.12), uTintStrength);

            // Keep alpha hard-set to 1 so Earth does not randomly go ghost mode again. Big nah.
            FragColor = vec4(finalColor, 1.0);
        }
    )";
}

namespace OrbitViewShaders
{
// Full-screen textured quad for the fake star wallpaper.
inline const char* StarBackdropVertexShader()
{
    return R"(
        #version 330 core

        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aUV;

        out vec2 vUV;

        void main()
        {
            vUV = aUV;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
}

inline const char* StarBackdropFragmentShader()
{
    return R"(
        #version 330 core

        in vec2 vUV;

        out vec4 FragColor;

        uniform sampler2D uStarMap;

        void main()
        {
            // Keep this fully opaque so the star wallpaper does not do weird Qt compositing nonsense.
            vec3 starColor = texture(uStarMap, vUV).rgb;
            FragColor = vec4(starColor, 1.0);
        }
    )";
}

// Tiny shader for the orbit trail lines. No textures, no drama, just positions.
inline const char* OrbitRingVertexShader()
{
    return R"(
        #version 330 core

        layout(location = 0) in vec3 aPos;

        uniform mat4 uMVP;

        out vec3 vRingPos;

        void main()
        {
            vRingPos = aPos;
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";
}

inline const char* OrbitRingFragmentShader()
{
    return R"(
        #version 330 core

        in vec3 vRingPos;

        out vec4 FragColor;

        uniform vec3 uColor;

        void main()
        {
            float angle = atan(vRingPos.z, vRingPos.x);
            float dashPattern = fract((angle + 3.14159265) / 6.28318530 * 28.0);

            if (dashPattern > 0.18)
            {
                discard;
            }

            FragColor = vec4(uColor, 1.0);
        }
    )";
}

// Super small point shader we use for the glow dot that helps track satellites.
inline const char* SatelliteGlowVertexShader()
{
    return R"(
        #version 330 core

        layout(location = 0) in vec3 aPos;

        uniform mat4 uMVP;
        uniform float uPointSize;

        void main()
        {
            gl_Position = uMVP * vec4(aPos, 1.0);
            gl_PointSize = uPointSize;
        }
    )";
}

inline const char* SatelliteGlowFragmentShader()
{
    return R"(
        #version 330 core

        out vec4 FragColor;

        uniform vec3 uColor;

        void main()
        {
            vec2 uv = gl_PointCoord * 2.0 - 1.0;
            float dist = dot(uv, uv);

            if (dist > 1.0)
            {
                discard;
            }

            float alpha = smoothstep(1.0, 0.0, dist);
            FragColor = vec4(uColor, alpha * 0.9);
        }
    )";
}

// Simple pulse ring for the solar storm warning vibe around Earth.
inline const char* SolarStormPulseVertexShader()
{
    return R"(
        #version 330 core

        layout(location = 0) in vec3 aPos;

        uniform mat4 uMVP;

        void main()
        {
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";
}

inline const char* SolarStormPulseFragmentShader()
{
    return R"(
        #version 330 core

        out vec4 FragColor;

        uniform vec3 uColor;
        uniform float uAlpha;

        void main()
        {
            FragColor = vec4(uColor, uAlpha);
        }
    )";
}

// Molten point particles for the "space weather is throwing hands" effect.
inline const char* SolarStormParticleVertexShader()
{
    return R"(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        layout(location = 2) in float aSize;

        uniform mat4 uMVP;

        out vec3 vColor;

        void main()
        {
            vColor = aColor;
            gl_Position = uMVP * vec4(aPos, 1.0);
            gl_PointSize = aSize;
        }
    )";
}

inline const char* SolarStormParticleFragmentShader()
{
    return R"(
        #version 330 core

        in vec3 vColor;

        out vec4 FragColor;

        void main()
        {
            vec2 uv = gl_PointCoord * 2.0 - 1.0;
            float dist = dot(uv, uv);

            if (dist > 1.0)
            {
                discard;
            }

            float alpha = smoothstep(1.0, 0.0, dist);
            FragColor = vec4(vColor, alpha * 0.88);
        }
    )";
}
}

#endif //SOUL_SHADERSOURCE_HPP
