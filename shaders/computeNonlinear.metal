//#include <metal_stdlib>
//using namespace metal;
//
///* -- Helper functions definitions -- */
//
//// Check if specimen or air cell
//bool checkIfGrid(const uint2 position, const uint2 bounds);
//// Load current displacement based on current counter
//float loadCurrent(const uint counter, const float4 readOut);
//float loadPrevious(const uint counter, const float4 readOut);
//float loadParam(const uint2 position, const uint2 bounds, const float paramValue);
//
///* -- Helper structs -- */
//
//struct FrameData
//{
//    uint counter;
//    float extortion;
//    float readOut;
//};
//
///* -- Compute function -- */
//
//kernel void add(texture2d< float, access::read_write> tex [[texture(0)]],
//                uint2 position [[thread_position_in_grid]],
//                uint2 gridSize [[threads_per_grid]],
//                device FrameData* frame [[buffer(0)]]) {
//    
//    if(checkIfGrid(position,gridSize)) {
//        // Displacements
//        const float w_previous    = loadPrevious(frame->counter,tex.read(position));
//        const float w_previous_5  = loadPrevious(frame->counter,tex.read(uint2(position.x+1,position.y)));
//        const float w_previous_6  = loadPrevious(frame->counter,tex.read(uint2(position.x,position.y+1)));
//        const float w_current     = loadCurrent(frame->counter,tex.read(position));
//        const float w_5           = loadCurrent(frame->counter,tex.read(uint2(position.x+1,position.y)));
//        const float w_7           = loadCurrent(frame->counter,tex.read(uint2(position.x-1,position.y)));
//        const float w_6           = loadCurrent(frame->counter,tex.read(uint2(position.x,position.y+1)));
//        const float w_8           = loadCurrent(frame->counter,tex.read(uint2(position.x,position.y-1)));
//        
//        const float w_current_sq = w_current*w_current;
//        
//        // Parameters
//        constexpr float dx = 1e-4;
//        constexpr float dx_square = dx * dx;
//        constexpr float dx_cube = dx_square * dx;
//        constexpr float dy = 1e-4;
//        constexpr float dy_square = dy * dy;
//        constexpr float dy_cube = dy_square * dy;
//        constexpr float dt = 1e-8;
//        constexpr float dt_square = dt * dt;
//        
//        // Hysteresis params
//        constexpr float Alpha = 100;
//        constexpr float Em = 1e-6;
//        constexpr float EmSquared = Em * Em;
//        constexpr float Beta1 = 100;
//        constexpr float Beta2 = 100;
//        
//        // Material constants
//        constexpr float E = 70e9;   // Young module  70 [GPa]
//        constexpr float rho = 2700; // Density 2700 [kg/m^3]
//        constexpr float poisson = 0.33; // Poisson ratio
//        constexpr float mu = E/(2*(1+poisson)); // Kirchoff's shear modulus [GPa]
//        
//        // Load params
//        float mu_1          = loadParam(position, gridSize, mu);
//        float mu_2          = loadParam(uint2(position.x-1,position.y), gridSize, mu);
//        float mu_3          = loadParam(uint2(position.x-1,position.y-1), gridSize, mu);
//        float mu_4          = loadParam(uint2(position.x,position.y-1), gridSize, mu);
//        float rho_1         = loadParam(position, gridSize, rho);
//        float rho_2         = loadParam(uint2(position.x-1,position.y), gridSize, rho);
//        float rho_3         = loadParam(uint2(position.x-1,position.y-1), gridSize, rho);
//        float rho_4         = loadParam(uint2(position.x,position.y-1), gridSize, rho);
//        float beta1         = loadParam(position, gridSize, Beta1);
//        float beta2         = loadParam(position, gridSize, Beta2);
//        float alpha         = loadParam(position, gridSize, Alpha);
//        
//        // General equation
//        const float Edotx = (
//                       (w_5 - w_current)/dx
//                       -
//                       (w_previous_5 - w_previous)/dx
//                       )/dt;
//        const float Edoty = (
//                             (w_6 - w_current)/dy
//                             -
//                             (w_previous_6 - w_previous)/dy
//                             )/dt;
//        
//        float w_next = 0;
//        if(position.x == 2) {
//            w_next = frame->extortion;
//        } else {
//            if(Edotx > 0) {
//                if(Edoty > 0) {
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dx
//                     +
//                     (EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dy
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (2*beta1*(w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
//                     -
//                     (2*beta1*(w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
//                     );
//                }
//                else if (Edoty < 0) {
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dx
//                     +
//                     (-EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dy
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (2*beta1*(w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
//                     -
//                     (-2*beta2*(w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
//                     );
//                }
//                else {  //Edoty == 0
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dx
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (2*beta1*(w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
//                     );
//                }
//            } else if (Edotx < 0) {
//                if(Edoty > 0) {
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (-EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dx
//                     +
//                     (EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dy
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (-2*beta2*(w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
//                     -
//                     (2*beta1*(w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
//                     );
//                }
//                else if (Edoty < 0) {
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (-EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dx
//                     +
//                     (-EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dy
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (-2*beta2*(w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
//                     -
//                     (-2*beta2*(w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
//                     );
//                }
//                else {  //Edoty == 0
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (-EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dx
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (-2*beta2*(w_current_sq*(mu_1-mu_2-mu_3+mu_4) + w_5*w_5*(mu_1+mu_4) - w_7*w_7*(mu_2+mu_3) -2*w_current*w_5*(mu_1+mu_4) + 2*w_current*w_7*(mu_2+mu_3)))/dx_cube
//                     );
//                }
//            } else {    // Edotx == 0
//                if(Edoty > 0) {
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dy
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (2*beta1*(w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
//                     );
//                }
//                else if (Edoty < 0) {
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//                     (-EmSquared*(beta1+beta2)*(mu_1-mu_2-mu_3+mu_4))/dy
//                     +
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     -
//                     // 1/dx^3 and 1/dy^3
//                     (-2*beta2*(w_current_sq*(mu_1+mu_2-mu_3-mu_4) + w_6*w_6*(mu_1+mu_2) - w_8*w_8*(mu_3+mu_4) -2*w_current*w_6*(mu_1+mu_2) + 2*w_current*w_8*(mu_3+mu_4)))/dy_cube
//                     );
//                }
//                else {  //Edoty == 0
//                    w_next = 2*w_current - w_previous +
//                    2*dt_square/(rho_1 + rho_2 + rho_3 + rho_4) *
//                    (
//                     // 1/dx and 1/dy
//
//                     // 1/dx^2 and 1/dy^2
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_5*(mu_1+mu_4) - w_7*(mu_2+mu_3)))/dx_square
//                     +
//                     ((4*Em*alpha-2)*(w_current*(mu_1+mu_2+mu_3+mu_4) - w_6*(mu_1+mu_2) - w_8*(mu_3+mu_4)))/dy_square
//                     // 1/dx^3 and 1/dy^3
//
//                     );
//                }
//            }
//        }
//        float4 newFrame = tex.read(position);
//        switch(frame->counter) {
//            case 0:
//                newFrame.b = w_next;
//            break;
//            case 1:
//                newFrame.a = w_next;
//            break;
//            case 2:
//                newFrame.r = w_next;
//            break;
//            case 3:
//                newFrame.g = w_next;
//            break;
//        }
//        tex.write(newFrame, position, 0);
//        if(position.x == 6000 && position.y == 3) {
//            frame->readOut = w_next;
//        }
//    }
//}
//
//inline bool checkIfGrid(uint2 position, uint2 bounds) {
//    if(position.x > 0 && position.x < (bounds.x-1)) {
//        if(position.y > 0 && position.y < (bounds.y-1)) {
//            return true;
//        }
//    }
//    return false;
//}
//inline bool checkIfGrid2(uint2 position, uint2 bounds) {
//    if(position.x > 0 && position.x < (bounds.x-2)) {
//        if(position.y > 0 && position.y < (bounds.y-2)) {
//            return true;
//        }
//    }
//    return false;
//}
///*
//    --- Count = 0 ---
//    r: Previous -> READ ONLY
//    g: Current -> READ ONLY
//    b: Next -> WRITE
//    a: Unused
//    --- Count = 1 ---
//    r: Unused
//    g: Previous -> READ ONLY
//    b: Current -> READ ONLY
//    a: Next -> WRITE
//    --- Count = 2 ---
//    r: Next -> WRITE
//    g: Unused
//    b: Previous -> READ ONLY
//    a: Current - READ ONLY
//    --- Count = 3 ---
//    r: Current -> READ ONLY
//    g: Next -> WRITE
//    b: Unused
//    a: Previous -> READ ONLY
//    --- REPEAT ---
//*/
//inline float loadCurrent(const uint counter, const float4 readOut) {
//    switch(counter) {
//        case 0:
//            return readOut.g;
//        break;
//        case 1:
//            return readOut.b;
//        break;
//        case 2:
//            return readOut.a;
//        break;
//        case 3:
//            return readOut.r;
//        break;
//        default:
//            return 0.f;
//        break;
//    }
//}
//inline float loadPrevious(const uint counter, const float4 readOut) {
//    switch(counter) {
//        case 0:
//            return readOut.r;
//        break;
//        case 1:
//            return readOut.g;
//        break;
//        case 2:
//            return readOut.b;
//        break;
//        case 3:
//            return readOut.a;
//        break;
//        default:
//            return 0.f;
//        break;
//    }
//}
//inline float loadParam(const uint2 position, const uint2 bounds, const float paramValue) {
//    if(checkIfGrid2(position, bounds)) {
//        return paramValue;
//    } else {
//        return 0.f;
//    }
//}
