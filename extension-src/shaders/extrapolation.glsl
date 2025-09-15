#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0, std430) buffer VelocityUData {
    float velocity[];
} data_u;
layout(set = 0, binding = 1, std430) buffer VelocityVData {
    float velocity[];
} data_v;
layout(set = 0, binding = 2, std430) buffer VelocityWData {
    float velocity[];
} data_w;

layout(set = 1, binding = 0) uniform GridParameter {
    ivec3 faces;
    float cell_size;
} grid_parameters;

layout(push_constant, std430) uniform Params {
    float delta_time;
} pc;

uint toIndex(uint i, uint j, uint k) {
    return (k * grid_parameters.faces.x * grid_parameters.faces.y) + (j * grid_parameters.faces.x) + i;
}

void main() {
    // Todo: Refactor to allow for non equal cell counts per dimension

    uint a = gl_GlobalInvocationID.x;
    uint b = gl_GlobalInvocationID.y;

    uint max_i = grid_parameters.faces.x - 1;
    uint max_j = grid_parameters.faces.y - 1;
    uint max_k = grid_parameters.faces.z - 1;

    /*
    data_u.velocity[toIndex(0, a, b)] = data_u.velocity[toIndex(1, a, b)];
    data_u.velocity[toIndex(max_i, a, b)] = data_u.velocity[toIndex(max_i - 1, a, b)];

    data_v.velocity[toIndex(a, 0, b)] = data_v.velocity[toIndex(a, 1, b)];
    data_v.velocity[toIndex(a, max_j, b)] = data_v.velocity[toIndex(a, max_j - 1, b)];

    data_w.velocity[toIndex(a, b, 0)] = data_w.velocity[toIndex(a, b, 1)];
    data_w.velocity[toIndex(a, b, max_k)] = data_w.velocity[toIndex(a, b, max_k - 1)];
    */
    data_u.velocity[toIndex(0, a, b)] = 0.0;
    data_u.velocity[toIndex(max_i, a, b)] = 0.0;

    data_v.velocity[toIndex(a, 0, b)] = 0.0;
    data_v.velocity[toIndex(a, max_j, b)] = 0.0;

    data_w.velocity[toIndex(a, b, 0)] = 0.0;
    data_w.velocity[toIndex(a, b, max_k)] = 0.0;
}
