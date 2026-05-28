#version 330 core
in  vec2 ndcPos;
out vec4 FragColor;

uniform mat4  invView;
uniform mat4  invProj;
uniform float uTime;    // monotonic seconds — drives the High-tier twinkle
uniform int   uQuality; // 0 Off, 1 Low, 2 Medium, 3 High (see StarfieldQuality)

// ---------------------------------------------------------------------------
// Tunables — the whole look lives here. Each star layer is (tiles-per-face,
// hosting density, brightness). Density is the fraction of cells that host a
// star; brightness scales that layer's peak intensity.
// ---------------------------------------------------------------------------
const float kCoreRadius   = 0.050; // star core radius in tile units
const float kBrightPow    = 3.5;   // power-law exponent (higher = fewer bright stars)

const float kBrightScale  = 26.0;  // sparse, brilliant foreground stars
const float kBrightDens   = 0.10;
const float kBrightLevel  = 1.00;

const float kMidScale     = 50.0;  // denser mid field
const float kMidDens      = 0.16;
const float kMidLevel     = 0.55;

const float kFarScale     = 90.0;  // faint dust of distant stars
const float kFarDens      = 0.35;
const float kFarLevel     = 0.30;

const vec3  kGalacticAxis = vec3(0.30, 0.90, 0.28); // pole of the Milky Way band (tilted off-axis)
const float kBandWidth    = 0.16;  // angular half-width of the band
const float kTwinkleSpeed = 2.5;
const float kTwinkleAmt   = 0.22;

// ---- Hashes ---------------------------------------------------------------
float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

vec2 hash22(vec2 p)
{
    float n = sin(dot(p, vec2(41.0, 289.0)));
    return fract(vec2(262144.0, 32768.0) * n);
}

float hash31(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

// 3D value noise (trilinear blend of 8 hashed lattice corners).
float vnoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f      = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);
    float nxy0 = mix(nx00, nx10, f.y);
    float nxy1 = mix(nx01, nx11, f.y);
    return mix(nxy0, nxy1, f.z);
}

// Approximate stellar color by temperature t in [0,1]: cool orange-white ->
// white -> hot blue-white. Most stars are cooler, so callers bias t downward.
vec3 blackbody(float t)
{
    vec3 cool = vec3(1.00, 0.80, 0.62); // ~3500 K
    vec3 mid  = vec3(1.00, 0.97, 0.92); // ~5800 K
    vec3 hot  = vec3(0.78, 0.86, 1.00); // ~10000 K
    if (t < 0.5)
        return mix(cool, mid, t * 2.0);
    return mix(mid, hot, (t - 0.5) * 2.0);
}

// Map a (normalized) direction to a per-cube-face [0,1] UV plus a face id.
// Cube faces avoid the pole pinching that a lat/long mapping would introduce.
vec2 dirToFaceUV(vec3 dir, out float face)
{
    vec3 a = abs(dir);
    vec2 uv;
    float ma;
    if (a.x >= a.y && a.x >= a.z)
    {
        ma   = a.x;
        face = dir.x > 0.0 ? 0.0 : 1.0;
        uv   = dir.x > 0.0 ? vec2(-dir.z, dir.y) : vec2(dir.z, dir.y);
    }
    else if (a.y >= a.z)
    {
        ma   = a.y;
        face = dir.y > 0.0 ? 2.0 : 3.0;
        uv   = dir.y > 0.0 ? vec2(dir.x, -dir.z) : vec2(dir.x, dir.z);
    }
    else
    {
        ma   = a.z;
        face = dir.z > 0.0 ? 4.0 : 5.0;
        uv   = dir.z > 0.0 ? vec2(dir.x, dir.y) : vec2(-dir.x, dir.y);
    }
    return uv / ma * 0.5 + 0.5;
}

// One layer of stars: tile the face UV, place a jittered point per cell, and
// splat a soft power-law dot. Sums the 3x3 neighborhood so points never clip
// at cell borders. `seed` decorrelates layers; `level` scales peak brightness.
vec3 starLayer(vec2 uv, float scale, float density, float level, float seed)
{
    vec2 p = uv * scale + seed * 17.0;

    // Screen-space tile size for anti-aliasing. Clamped so the 1px face-seam
    // discontinuity in `p` can't blow the core radius up into a bright smear.
    float aa = clamp(max(fwidth(p.x), fwidth(p.y)), 0.0, 0.05);

    vec2 ip = floor(p);
    vec2 fp = p - ip;

    vec3 acc = vec3(0.0);
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            vec2 cell  = ip + vec2(float(i), float(j));
            float pres = hash21(cell + seed);
            float t    = (pres - (1.0 - density)) / max(density, 1e-4);
            if (t <= 0.0)
                continue;

            vec2 jit = hash22(cell * 1.7 + seed);
            vec2 sp  = vec2(float(i), float(j)) + jit;
            float d  = length(fp - sp);

            float r = max(kCoreRadius, aa);
            float v = 1.0 - smoothstep(0.0, r, d);
            v       = v * v;

            float b = pow(t, kBrightPow) * level;

            if (uQuality >= 3)
            {
                float ph = hash21(cell + 7.0) * 6.2831853;
                b *= (1.0 - kTwinkleAmt) + kTwinkleAmt * (0.5 + 0.5 * sin(uTime * kTwinkleSpeed + ph));
            }

            vec3 tint = blackbody(hash21(cell + seed + 3.3));
            acc += tint * (v * b);
        }
    }
    return acc;
}

void main()
{
    // Reconstruct the world-space view ray (project then view, both inverted).
    // Stars depend only on direction, so they sit "at infinity" — translating
    // the camera doesn't shift them.
    vec4 clip = vec4(ndcPos, 1.0, 1.0);
    vec4 eye  = invProj * clip;
    eye       = vec4(eye.xy, -1.0, 0.0);
    vec3 dir  = normalize(vec3(invView * eye));

    if (uQuality <= 0)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float face;
    vec2 uv  = dirToFaceUV(dir, face);
    vec2 luv = uv + face * 9.137; // decorrelate the six faces

    vec3 col = vec3(0.0);

    // Milky Way band (High). Computed first so it can concentrate faint stars.
    float band = 0.0;
    if (uQuality >= 3)
    {
        vec3 axis = normalize(kGalacticAxis);
        float lat = dot(dir, axis); // 0 on the band's great circle
        band      = exp(-(lat * lat) / (2.0 * kBandWidth * kBandWidth));
        float dust = band * (0.4 + 0.6 * vnoise(dir * 6.0));
        col += dust * vec3(0.10, 0.12, 0.16) * 0.6; // faint bluish dust glow
    }

    // Bright foreground (Low and up).
    col += starLayer(luv, kBrightScale, kBrightDens, kBrightLevel, 1.0);

    // Denser mid field (Medium and up).
    if (uQuality >= 2)
        col += starLayer(luv, kMidScale, kMidDens, kMidLevel, 2.0);

    // Faint dust stars (High) — denser, and concentrated along the band.
    if (uQuality >= 3)
        col += starLayer(luv, kFarScale, kFarDens, kFarLevel * (1.0 + band * 1.8), 3.0);

    FragColor = vec4(col, 1.0);
}
