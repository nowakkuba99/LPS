#include <metal_stdlib>
using namespace metal;

/* -- Helper functions definitions -- */

// Check if specimen or air cell
bool checkIfGrid(const uint2 position, const uint2 bounds);
// Load current displacement based on current counter
float loadCurrent(const uint counter, const float4 readOut);
float loadPrevious(const uint counter, const float4 readOut);
float loadParam(const uint2 position, const uint2 bounds, const float paramValue);

/* -- Helper structs -- */

struct FrameData
{
    uint counter;
    float excitation;
    float readOut;
};

/* -- Compute function -- */

kernel void computeLISA(texture2d< float, access::read_write> tex [[texture(0)]],
                uint2 position [[thread_position_in_grid]],
                uint2 gridSize [[threads_per_grid]],
                device FrameData* frame [[buffer(0)]]) {
    
    if(checkIfGrid(position,gridSize)) {
        // Displacements
        const float w_previous    = loadPrevious(frame->counter,tex.read(position));
        const float w_previous_5  = loadPrevious(frame->counter,tex.read(uint2(position.x+1,position.y)));
        const float w_previous_6  = loadPrevious(frame->counter,tex.read(uint2(position.x,position.y+1)));
        const float w_current     = loadCurrent(frame->counter,tex.read(position));
        const float w_5           = loadCurrent(frame->counter,tex.read(uint2(position.x+1,position.y)));
        const float w_7           = loadCurrent(frame->counter,tex.read(uint2(position.x-1,position.y)));
        const float w_6           = loadCurrent(frame->counter,tex.read(uint2(position.x,position.y+1)));
        const float w_8           = loadCurrent(frame->counter,tex.read(uint2(position.x,position.y-1)));
        
        const float w_current_sq = w_current*w_current;
        
        // Parameters
        constexpr float dx = 1e-4;
        constexpr float dx_square = dx * dx;
        constexpr float dx_cube = dx_square * dx;
        constexpr float dy = 1e-4;
        constexpr float dy_square = dy * dy;
        constexpr float dy_cube = dy_square * dy;
        constexpr float dt = 1e-8;
        constexpr float dt_square = dt * dt;
        
        // Hysteresis params
        constexpr float Em = 1e-6;
        constexpr float EmP = Em;
        constexpr float EmN = Em;
        constexpr float G = 150;
        constexpr float G1 = G;
        constexpr float G2 = G;
        constexpr float G3 = G;
        constexpr float G4 = G;
        
        // Material constants
        constexpr float E = 70e9;   // Young module  70 [GPa]
        constexpr float rho = 2700; // Density 2700 [kg/m^3]
        constexpr float poisson = 0.33; // Poisson ratio
        constexpr float mu = E/(2*(1+poisson)); // Kirchoff's shear modulus [GPa]
        
        // Crack placement
        constexpr float crackMid = 4000;
        constexpr float crackSize = 500;
        constexpr float crackLeft = crackMid - crackSize/2;
        constexpr float crackRight = crackMid + crackSize/2;
        
        // Load params
        float mu_1          = loadParam(position, gridSize, mu);
        float mu_2          = loadParam(uint2(position.x-1,position.y), gridSize, mu);
        float mu_3          = loadParam(uint2(position.x-1,position.y-1), gridSize, mu);
        float mu_4          = loadParam(uint2(position.x,position.y-1), gridSize, mu);
        float rho_1         = loadParam(position, gridSize, rho);
        float rho_2         = loadParam(uint2(position.x-1,position.y), gridSize, rho);
        float rho_3         = loadParam(uint2(position.x-1,position.y-1), gridSize, rho);
        float rho_4         = loadParam(uint2(position.x,position.y-1), gridSize, rho);
        float g1            = loadParam(position, gridSize, G1);
        float g2            = loadParam(position, gridSize, G2);
        float g3            = loadParam(position, gridSize, G3);
        float g4            = loadParam(position, gridSize, G4);
        
        // General equation
        const float Ex = (w_5 - w_current)/dx;
        const float Ey = (w_6 - w_current)/dy;
        
        const float Edotx = (
                       (w_5 - w_current)/dx
                       -
                       (w_previous_5 - w_previous)/dx
                       )/dt;
        const float Edoty = (
                             (w_6 - w_current)/dy
                             -
                             (w_previous_6 - w_previous)/dy
                             )/dt;
        
        float w_next = 0;
        if(position.x == 2) {
            w_next = frame->excitation;
        } else if(position.x >= crackLeft && position.x <= crackRight){
            
            int A = 0, B = 0, C = 0, D = 0, E = 0, F = 0, G = 0, H = 0;
            
            // Ex
            if(Ex > 0)
                A = 1;
            else if(Ex < 0)
                B = 1;
            // Ey
            if(Ey > 0)
                C = 1;
            else if(Ey < 0)
                D = 1;
            // Edotx
            if(Edotx > 0)
                E = 1;
            else if(Edotx < 0)
                F = 1;
            // Edoty
            if(Edoty > 0)
                G = 1;
            else if(Edotx < 0)
                H = 1;
            
            
            w_next = 2*w_current - w_previous +
            dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
            (
             // 1/dx^2 and 1/dy^2
             (2*(A*F*EmP*(g1+g2) + B*E*EmN*(g3+g4) - 1) * (w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
             +
             (2*(C*H*EmP*(g1+g2) + D*G*EmN*(g3+g4) - 1) *(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
             -
             // 1/dx^3 and 1/dy^3
             (2*(A*E*g1 - A*F*g2 - B*F*g3 + B*E*g4) * (w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
             -
             (2*(C*G*g1 - C*H*g2 - D*H*g3 + D*G*g4)* (w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
             );
        } else {
            w_next =
                2*w_current - w_previous +
                2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
                (
                    (
                        w_5 * (mu_1 + mu_4) + w_7 * (mu_2 + mu_3)
                        - w_current * (mu_1 + mu_2 + mu_3 + mu_4)
                    )/dx_square
                    +
                    (
                        w_6 * (mu_1 + mu_2) + w_8 * (mu_3 + mu_4)
                        - w_current * (mu_1 + mu_2 + mu_3 + mu_4)
                    )/dy_square
                );
        }
        float4 newFrame = tex.read(position);
        switch(frame->counter) {
            case 0:
                newFrame.b = w_next;
            break;
            case 1:
                newFrame.a = w_next;
            break;
            case 2:
                newFrame.r = w_next;
            break;
            case 3:
                newFrame.g = w_next;
            break;
        }
        tex.write(newFrame, position, 0);
        if(position.x == 4000 && position.y == 3) {
            frame->readOut = w_next;
        }
    }
}

inline bool checkIfGrid(uint2 position, uint2 bounds) {
    if(position.x > 0 && position.x < (bounds.x-1)) {
        if(position.y > 0 && position.y < (bounds.y-1)) {
            return true;
        }
    }
    return false;
}
inline bool checkIfGrid2(uint2 position, uint2 bounds) {
    if(position.x > 0 && position.x < (bounds.x-2)) {
        if(position.y > 0 && position.y < (bounds.y-2)) {
            return true;
        }
    }
    return false;
}
/*
    --- Count = 0 ---
    r: Previous -> READ ONLY
    g: Current -> READ ONLY
    b: Next -> WRITE
    a: Unused
    --- Count = 1 ---
    r: Unused
    g: Previous -> READ ONLY
    b: Current -> READ ONLY
    a: Next -> WRITE
    --- Count = 2 ---
    r: Next -> WRITE
    g: Unused
    b: Previous -> READ ONLY
    a: Current - READ ONLY
    --- Count = 3 ---
    r: Current -> READ ONLY
    g: Next -> WRITE
    b: Unused
    a: Previous -> READ ONLY
    --- REPEAT ---
*/
inline float loadCurrent(const uint counter, const float4 readOut) {
    switch(counter) {
        case 0:
            return readOut.g;
        break;
        case 1:
            return readOut.b;
        break;
        case 2:
            return readOut.a;
        break;
        case 3:
            return readOut.r;
        break;
        default:
            return 0.f;
        break;
    }
}
inline float loadPrevious(const uint counter, const float4 readOut) {
    switch(counter) {
        case 0:
            return readOut.r;
        break;
        case 1:
            return readOut.g;
        break;
        case 2:
            return readOut.b;
        break;
        case 3:
            return readOut.a;
        break;
        default:
            return 0.f;
        break;
    }
}
inline float loadParam(const uint2 position, const uint2 bounds, const float paramValue) {
    if(checkIfGrid2(position, bounds)) {
        return paramValue;
    } else {
        return 0.f;
    }
}
