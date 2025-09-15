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

layout(set = 1, binding = 0, std430) buffer readonly SolidData {
    float is_fluid[];
} solid_data;

layout(set = 2, binding = 0, std430) buffer writeonly PressureData {
    float pressure[];
} pressure_data;

layout(set = 3, binding = 0) uniform GridParameter {
    ivec3 faces;
    float cell_size;
} grid_parameters;

layout(push_constant, std430) uniform Params {
    float delta_time;
    int iteration;
} pc;

int toIndex(ivec3 ijk) {
    ivec3 items = grid_parameters.faces;
    return (ijk.z * items.x * items.y) + (ijk.y * items.x) + ijk.x;
}

const ivec3 offsets1[2] = ivec3[2](
    ivec3(0, 0, 0),
    ivec3(1, 0, 0)
);
const ivec3 offsets2[2] = ivec3[2](
    ivec3(1, 1, 0),
    ivec3(0, 1, 0)
);
const ivec3 offsets3[2] = ivec3[2](
    ivec3(0, 1, 1),
    ivec3(0, 0, 1)
);
const ivec3 offsets4[2] = ivec3[2](
    ivec3(1, 0, 1),
    ivec3(1, 1, 1)
);

void main() {
    float overRelaxation = 1.7;
    float density = 1000.0;

	ivec3 ijk_base = 2 * ivec3(gl_GlobalInvocationID.xyz);
    ivec3 ijk_starts[4] = { 
        ijk_base + offsets1[pc.iteration],
        ijk_base + offsets2[pc.iteration],
        ijk_base + offsets3[pc.iteration],
        ijk_base + offsets4[pc.iteration]
    };

    for (int i=0; i < 4; i++) {
        ivec3 ijk_uvw0 = ijk_starts[i];

        if (ijk_uvw0.x == 0 || ijk_uvw0.y == 0 || ijk_uvw0.z == 0 ||
            ijk_uvw0.x == grid_parameters.faces.x - 1 || ijk_uvw0.y == grid_parameters.faces.y - 1 || ijk_uvw0.z == grid_parameters.faces.z - 1
        ) {
            continue;
        }

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
            continue;
        }

        ivec3 ijk_u1 = ijk_uvw0 + ivec3(1, 0, 0);
        ivec3 ijk_v1 = ijk_uvw0 + ivec3(0, 1, 0);
        ivec3 ijk_w1 = ijk_uvw0 + ivec3(0, 0, 1);

        int idx_uvw0 = toIndex(ijk_uvw0);
        int idx_u1 = toIndex(ijk_u1);
        int idx_v1 = toIndex(ijk_v1);
        int idx_w1 = toIndex(ijk_w1);

        float u0 = data_u.velocity[idx_uvw0];
        float u1 = data_u.velocity[idx_u1];
        float v0 = data_v.velocity[idx_uvw0];
        float v1 = data_v.velocity[idx_v1];
        float w0 = data_w.velocity[idx_uvw0];
        float w1 = data_w.velocity[idx_w1];

        float d = u1 - u0 + v1 - v0 + w1 - w0;
        float p = (-1.0 / s_sum) * d * overRelaxation;

        data_u.velocity[idx_uvw0] = data_u.velocity[idx_uvw0] - s[0] * p;
        data_u.velocity[idx_u1] = data_u.velocity[idx_u1] + s[3] * p;
        data_v.velocity[idx_uvw0] = data_v.velocity[idx_uvw0] - s[1] * p;
        data_v.velocity[idx_v1] = data_v.velocity[idx_v1] + s[4] * p;
        data_w.velocity[idx_uvw0] = data_w.velocity[idx_uvw0] - s[2] * p;
        data_w.velocity[idx_w1] = data_w.velocity[idx_w1] + s[5] * p;

        pressure_data.pressure[idx_uvw0] += p * density * grid_parameters.cell_size / pc.delta_time;
    }
}
