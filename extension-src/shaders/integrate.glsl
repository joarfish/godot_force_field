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

layout(set = 2, binding = 0, std430) buffer writeonly PressureInData {
    float pressure[];
} pressure_data;

layout(set = 3, binding = 0, std430) buffer readonly SolidData {
    float is_fluid[];
} solid_data;

layout(set = 4, binding = 0) uniform GridParameter {
    ivec3 faces;
    float cell_size;
} grid_parameters;

layout(set = 5, binding = 0) uniform Emitter {
    vec3 min;
    vec3 max;
    vec3 velocity;
} emitter;

layout(push_constant, std430) uniform Params {
    float delta_time;
} pc;

int toIndex(ivec3 ijk) {
    ivec3 items = grid_parameters.faces;
    return (ijk.z * items.x * items.y) + (ijk.y * items.x) + ijk.x;
}

void main() {
    vec3 g = vec3(0.0, -9.81, 0.0);
	ivec3 ijk = ivec3(gl_GlobalInvocationID.xyz);
    int i = toIndex(ijk);

    float s = ijk.y > 1 && ijk.y < grid_parameters.faces.y - 1 && ijk.x > 0 && ijk.z > 0 ? 1.0 : 0.0;
    s = s * solid_data.is_fluid[i] * solid_data.is_fluid[toIndex(ijk - ivec3(0.0, 1.0, 0.0))];
    s = 0;

    u_out.velocity[i] = u_in.velocity[i] + s * pc.delta_time * g.x;
    v_out.velocity[i] = v_in.velocity[i] + s * pc.delta_time * g.y;
    w_out.velocity[i] = w_in.velocity[i] + s * pc.delta_time * g.z;

    ivec3 emitter0 = ivec3(floor(vec3(grid_parameters.faces) * emitter.min));
    ivec3 emitter1 = ivec3(floor(vec3(grid_parameters.faces) * emitter.max));

    if (length(emitter.velocity) > 0.0 && ijk.x > emitter0.x && ijk.x < emitter1.x && ijk.y > emitter0.y && ijk.y < emitter1.y && ijk.z > emitter0.z && ijk.z < emitter1.z) {
        u_out.velocity[i] = emitter.velocity.x * pc.delta_time;
        v_out.velocity[i] = emitter.velocity.y * pc.delta_time;
        w_out.velocity[i] = emitter.velocity.z * pc.delta_time;
    }

    pressure_data.pressure[i] = 0.0;
}
