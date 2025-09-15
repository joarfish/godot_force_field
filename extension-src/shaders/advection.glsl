#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, std430) buffer readonly VelocityUInData {
    float velocity[];
} u_in;
layout(set = 0, binding = 1, std430) buffer readonly VelocityVInData {
    float velocity[];
} v_in;
layout(set = 0, binding = 2, std430) buffer readonly VelocityWInData {
    float velocity[];
} w_in;

layout(set = 1, binding = 0, std430) buffer writeonly VelocityUOutData {
    float velocity[];
} u_out;
layout(set = 1, binding = 1, std430) buffer writeonly VelocityVOutData {
    float velocity[];
} v_out;
layout(set = 1, binding = 2, std430) buffer writeonly VelocityWOutData {
    float velocity[];
} w_out;

layout(set = 2, binding = 0, std430) buffer readonly SolidData {
    float is_fluid[];
} solid_data;

layout(set = 3, binding = 0) uniform GridParameter {
    ivec3 faces;
    float cell_size;
} grid_parameters;

layout(push_constant, std430) uniform Params {
    float delta_time;
} pc;

int toIndex(ivec3 uvw) {
    ivec3 items = grid_parameters.faces;
    return (uvw.z * items.x * items.y) + (uvw.y * items.x) + uvw.x;
}

#define MAKE_SAMPLE_FN(DIM, DIM_IDX, SOURCE) float sample_field_##DIM(vec3 pos) { \
    pos = clamp(pos, vec3(0.0, 0.0, 0.0), grid_parameters.cell_size * vec3(grid_parameters.faces - ivec3(1)));\
\
    vec3 d_xyz = 0.5 * vec3(grid_parameters.cell_size);\
    d_xyz[DIM_IDX] = 0.0;\
\
    vec3 ijk_max = grid_parameters.faces - ivec3(1);\
\
    vec3 ijk1 = min(floor((pos - d_xyz) / grid_parameters.cell_size), ijk_max);\
\
    int idx_1 = toIndex(ivec3(ijk1));\
    int idx_2 = toIndex(ivec3(min(ijk1 + vec3(0.0, 1.0, 0.0), ijk_max)));\
    int idx_3 = toIndex(ivec3(min(ijk1 + vec3(0.0, 0.0, 1.0), ijk_max)));\
    int idx_4 = toIndex(ivec3(min(ijk1 + vec3(0.0, 1.0, 1.0), ijk_max)));\
    int idx_5 = toIndex(ivec3(min(ijk1 + vec3(1.0, 0.0, 0.0), ijk_max)));\
    int idx_6 = toIndex(ivec3(min(ijk1 + vec3(1.0, 1.0, 0.0), ijk_max)));\
    int idx_7 = toIndex(ivec3(min(ijk1 + vec3(1.0, 0.0, 1.0), ijk_max)));\
    int idx_8 = toIndex(ivec3(min(ijk1 + vec3(1.0, 1.0, 1.0), ijk_max)));\
\
    float w1 = ((pos - d_xyz - ijk1 * grid_parameters.cell_size) / grid_parameters.cell_size)[DIM_IDX];\
    float w2 = 1.0 - w1;\
\
    float vel1 = SOURCE.velocity[idx_1];\
    float vel2 = SOURCE.velocity[idx_2];\
    float vel3 = SOURCE.velocity[idx_3];\
    float vel4 = SOURCE.velocity[idx_4];\
    float vel5 = SOURCE.velocity[idx_5];\
    float vel6 = SOURCE.velocity[idx_6];\
    float vel7 = SOURCE.velocity[idx_7];\
    float vel8 = SOURCE.velocity[idx_8];\
\
    return w1 * w1 * w1 * vel1 +\
             w1 * w2 * w1 * vel2 +\
             w1 * w1 * w2 * vel3 +\
             w1 * w2 * w2 * vel4 +\
             w2 * w1 * w1 * vel5 +\
             w2 * w2 * w1 * vel6 +\
             w2 * w1 * w2 * vel7 +\
             w2 * w2 * w2 * vel8;\
}

MAKE_SAMPLE_FN(u, 0, u_in)
MAKE_SAMPLE_FN(v, 1, v_in)
MAKE_SAMPLE_FN(w, 2, w_in)

void main() {
	ivec3 ijk = ivec3(gl_GlobalInvocationID.xyz);
    int i = toIndex(ijk);

    if (
            ijk.x == 0 || ijk.y == 0 || ijk.z == 0
        ) {
        return;
    }

    float u0 = u_in.velocity[i];
    float v0 = v_in.velocity[i];
    float w0 = w_in.velocity[i];

    /*
    u_out.velocity[i] = u_in.velocity[i];
    v_out.velocity[i] = v_in.velocity[i];
    w_out.velocity[i] = w_in.velocity[i];

    return;
    */

    bool is_fluid = solid_data.is_fluid[i] > 0.0;
    float cell_size = grid_parameters.cell_size;

    // Advect U

    if (is_fluid && solid_data.is_fluid[toIndex(ijk - ivec3(1, 0, 0))] > 0.0) {
        float vel_u_v1 = v0;
        float vel_u_v2 = v_in.velocity[toIndex(ijk + ivec3( 0, 1, 0))];
        float vel_u_v3 = v_in.velocity[toIndex(ijk + ivec3(-1, 0, 0))];
        float vel_u_v4 = v_in.velocity[toIndex(ijk + ivec3(-1, 1, 0))];

        float vel_u_w1 = w0;
        float vel_u_w2 = w_in.velocity[toIndex(ijk + ivec3( 0, 0, 1))];
        float vel_u_w3 = w_in.velocity[toIndex(ijk + ivec3(-1, 0, 0))];
        float vel_u_w4 = w_in.velocity[toIndex(ijk + ivec3(-1, 0, 1))];

        vec3 vel_u = vec3(
                u0,
                (vel_u_v1 + vel_u_v2 + vel_u_v3 + vel_u_v4) * 0.25,
                (vel_u_w1 + vel_u_w2 + vel_u_w3 + vel_u_w4) * 0.25
                );
        vec3 p_u = vec3(ijk * cell_size) + 0.5*vec3(0.0, cell_size, cell_size) - vel_u * pc.delta_time;

        u_out.velocity[i] = sample_field_u(p_u);
    }

    // Advect V

    if (is_fluid && solid_data.is_fluid[toIndex(ijk - ivec3(0, 1, 0))] > 0.0) {
        float vel_v_u1 = u0;
        float vel_v_u2 = u_in.velocity[toIndex(ijk + ivec3(1,  0, 0))];
        float vel_v_u3 = u_in.velocity[toIndex(ijk + ivec3(0, -1, 0))];
        float vel_v_u4 = u_in.velocity[toIndex(ijk + ivec3(1, -1, 0))];

        float vel_v_w1 = w0;
        float vel_v_w2 = w_in.velocity[toIndex(ijk + ivec3(0,  0, 1))];
        float vel_v_w3 = w_in.velocity[toIndex(ijk + ivec3(0, -1, 0))];
        float vel_v_w4 = w_in.velocity[toIndex(ijk + ivec3(0, -1, 1))];

        vec3 vel_v = vec3(
                (vel_v_u1 + vel_v_u2 + vel_v_u3 + vel_v_u4) * 0.25,
                v0,
                (vel_v_w1 + vel_v_w2 + vel_v_w3 + vel_v_w4) * 0.25
                );
        vec3 p_v = vec3(ijk * cell_size) + 0.5*vec3(cell_size, 0.0, cell_size) - vel_v * pc.delta_time;

        v_out.velocity[i] = sample_field_v(p_v);
    }

    // Advect W

    if (is_fluid && solid_data.is_fluid[toIndex(ijk - ivec3(0, 0, 1))] > 0.0) {
        float vel_w_u1 = u0;
        float vel_w_u2 = u_in.velocity[toIndex(ijk + ivec3(1, 0,  0))];
        float vel_w_u3 = u_in.velocity[toIndex(ijk + ivec3(0, 0, -1))];
        float vel_w_u4 = u_in.velocity[toIndex(ijk + ivec3(1, 0, -1))];

        float vel_w_v1 = v0;
        float vel_w_v2 = v_in.velocity[toIndex(ijk + ivec3(0, 1, 0))];
        float vel_w_v3 = v_in.velocity[toIndex(ijk + ivec3(0, 0, -1))];
        float vel_w_v4 = v_in.velocity[toIndex(ijk + ivec3(0, 1, -1))];

        vec3 vel_w = vec3(
                (vel_w_u1 + vel_w_u2 + vel_w_u3 + vel_w_u4) * 0.25,
                (vel_w_v1 + vel_w_v2 + vel_w_v3 + vel_w_v4) * 0.25,
                w0
                );

        vec3 p_w = vec3(ijk * cell_size) + 0.5*vec3(cell_size, cell_size, 0.0) - vel_w * pc.delta_time;

        w_out.velocity[i] = sample_field_w(p_w);
    }
}
