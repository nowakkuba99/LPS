#include <metal_stdlib>
using namespace metal;


namespace colormap {
namespace MATLAB {
namespace jet {

float colormap_red(float x) {
    if (x < 0.7) {
        return 4.0 * x - 1.5;
    } else {
        return -4.0 * x + 4.5;
    }
}

float colormap_green(float x) {
    if (x < 0.5) {
        return 4.0 * x - 0.5;
    } else {
        return -4.0 * x + 3.5;
    }
}

float colormap_blue(float x) {
    if (x < 0.3) {
       return 4.0 * x + 0.5;
    } else {
       return -4.0 * x + 2.5;
    }
}

float4 colormap(float x) {
    float r = clamp(colormap_red(x), 0.0, 1.0);
    float g = clamp(colormap_green(x), 0.0, 1.0);
    float b = clamp(colormap_blue(x), 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace jet
} // namespace MATLAB
} // namespace colormap

float loadCurrent(const uint counter, const float4 readOut);

struct v2f
{
    float4 position [[position]];
    float2 texcoord;
    uint counter;
};

struct VertexData
{
    float3 position;
    float2 texCoord;
};

struct FrameData
{
    uint counter;
    float extortion;
    float readOut;
};

v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                       constant FrameData* frameData [[buffer(1)]],
                       uint vertexId [[vertex_id]])
{
    v2f o;
    o.position = float4(vertexData[ vertexId ].position, 1.0);
    o.texcoord = vertexData[vertexId].texCoord.xy;
    o.counter = frameData->counter;
    return o;
}

float4 fragment fragmentMain( v2f in [[stage_in]],
                             texture2d< float, access::sample > tex [[texture(0)]])
{
    constexpr sampler s( address::repeat, filter::linear );
    float4 readOut = tex.sample( s, in.texcoord );
    float val = loadCurrent(in.counter,readOut);

    float minLim = -6e-6;
    float maxLim = 6e-6;
    val = (val - minLim) / (maxLim - minLim);
    
    
    
    float4 ret = colormap::MATLAB::jet::colormap(val);
    return ret;
    
}


inline float loadCurrent(const uint counter, const float4 readOut) {
    switch(counter) {
        case 0:
            return readOut.b;
        break;
        case 1:
            return readOut.a;
        break;
        case 2:
            return readOut.r;
        break;
        case 3:
            return readOut.g;
        break;
        default:
            return 0.f;
        break;
    }
}
