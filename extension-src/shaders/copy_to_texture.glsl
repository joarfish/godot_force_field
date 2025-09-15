#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, std430) buffer VelocityUData {
    float velocity[];
} data_u;
layout(set = 0, binding = 1, std430) buffer VelocityVData {
    float velocity[];
} data_v;
layout(set = 0, binding = 2, std430) buffer VelocityWData {
    float velocity[];
} data_w;

layout(set = 1, binding = 0, std430) buffer PressureData {
    float pressure[];
} pressure_data;

layout(set = 2, binding = 0, rgba32f) uniform restrict writeonly image3D image;
layout(set = 3, binding = 0, std430) buffer readonly SolidData {
    float is_fluid[];
} solid_data;

layout(set = 4, binding = 0) uniform GridParameter {
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

void main() {
    ivec3 faces;
    ivec3 ijk = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 ink_out = ijk;

    ivec3 ijk_uvw0 = ijk;

    if (
            ijk_uvw0.x == 0 || ijk_uvw0.y == 0 || ijk_uvw0.z == 0 ||
            ijk_uvw0.x == faces.x - 1 || ijk_uvw0.y == faces.y - 1 || ijk_uvw0.z == faces.z - 1) {
        return;
    }

    ivec3 ijk_u1 = min(ijk + ivec3(1, 0, 0), ivec3(faces.x - 1, ijk.yz));
    ivec3 ijk_v1 = min(ijk + ivec3(0, 1, 0), ivec3(ijk.x, faces.y - 1, ijk.z));
    ivec3 ijk_w1 = min(ijk + ivec3(0, 0, 1), ivec3(ijk.xy, faces.z - 1));

    float s[6] = {
        solid_data.is_fluid[toIndex(ijk_uvw0 - ivec3(1, 0, 0))],
        solid_data.is_fluid[toIndex(ijk_uvw0 - ivec3(0, 1, 0))],
        solid_data.is_fluid[toIndex(ijk_uvw0 - ivec3(0, 0, 1))],
        solid_data.is_fluid[toIndex(ijk_uvw0 + ivec3(1, 0, 0))],
        solid_data.is_fluid[toIndex(ijk_uvw0 + ivec3(0, 1, 0))],
        solid_data.is_fluid[toIndex(ijk_uvw0 + ivec3(0, 0, 1))],
    };
   
    float s_sum = s[0] + s[1] + s[2] + s[3] + s[4] + s[5];

    if (s_sum == 0.0) {
        imageStore(image, ijk, vec4(0.0, 0.0, 0.0, 0.0));
        return;
    }

    int idx_uvw0 = toIndex(ijk_uvw0);
    int idx_u1 = toIndex(ijk_u1);
    int idx_v1 = toIndex(ijk_v1);
    int idx_w1 = toIndex(ijk_w1);

    float vel_u0 = data_u.velocity[idx_uvw0];
    float vel_v0 = data_v.velocity[idx_uvw0];
    float vel_w0 = data_w.velocity[idx_uvw0];
    float vel_u1 = data_u.velocity[idx_u1];
    float vel_v1 = data_v.velocity[idx_v1];
    float vel_w1 = data_w.velocity[idx_w1];

    vec3 velocity = vec3((vel_u0 + vel_u1) * 0.5, (vel_v0 + vel_v1) * 0.5, (vel_w0 + vel_w1) * 0.5);
    float pressure = pressure_data.pressure[idx_uvw0];

    imageStore(image, ijk, vec4(velocity.xyz, pressure));
}
