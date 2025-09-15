#include "force_field.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rendering_device.hpp>

#include "godot_cpp/classes/rd_shader_file.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/variant/typed_array.hpp"

#include "godot_cpp/classes/rd_shader_spirv.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/input_event_key.hpp"

using namespace godot;

void ForceField::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_field_size"), &ForceField::get_field_size);
    ClassDB::bind_method(D_METHOD("set_field_size", "field_size"), &ForceField::set_field_size);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "field_size"), "set_field_size", "get_field_size");

    ClassDB::bind_method(D_METHOD("get_cell_size"), &ForceField::get_cell_size);
    ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &ForceField::set_cell_size);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size"), "set_cell_size", "get_cell_size");

    ClassDB::bind_method(D_METHOD("get_texture"), &ForceField::get_texture);
    ClassDB::bind_method(D_METHOD("set_texture"), &ForceField::set_texture);

    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture3DRD", PROPERTY_USAGE_DEFAULT |
            PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_texture", "get_texture");

    ClassDB::bind_method(D_METHOD("get_emitter_min"), &ForceField::get_emitter_position_min);
    ClassDB::bind_method(D_METHOD("set_emitter_min", "emitter_min"), &ForceField::set_emitter_position_min);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "emitter_pos_min"), "set_emitter_min", "get_emitter_min");

    ClassDB::bind_method(D_METHOD("get_emitter_max"), &ForceField::get_emitter_position_max);
    ClassDB::bind_method(D_METHOD("set_emitter_max", "emitter_max"), &ForceField::set_emitter_position_max);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "emitter_pos_max"), "set_emitter_max", "get_emitter_max");

    ClassDB::bind_method(D_METHOD("get_emitter_velocity"), &ForceField::get_emitter_velocity);
    ClassDB::bind_method(D_METHOD("set_emitter_velocity", "emitter_max"), &ForceField::set_emitter_velocity);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "emitter_velocity"), "set_emitter_velocity", "get_emitter_velocity");
}

ForceField::ForceField() {
    m_device = nullptr;

    m_field_size = Vector3i(32, 32, 32);
    m_cell_size = 0.2;
}

void ForceField::_ready() {
    UtilityFunctions::print("ForceField Ready");

    RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this, &ForceField::init_compute));
}

void ForceField::_process(double delta) {
    if (!m_compute_ready) {
        return;
    }

    RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this, &ForceField::run_compute));
}

void ForceField::_input(const Ref<InputEvent>& event) {
    if (event->is_class("InputEventKey")) {
        Ref<InputEventKey> event_key = event;

        if (event_key->is_pressed() && event_key->get_keycode() == KEY_D) {
            UtilityFunctions::print("Debug Key pressed.");
            m_print_debug_info = true;
        }
    }
}

Vector3i ForceField::get_field_size() const {
    return m_field_size;
}

void ForceField::set_field_size(Vector3i size) {
    m_field_size = size;
}

float ForceField::get_cell_size() const {
    return m_cell_size;
}

void ForceField::set_cell_size(float size) {
    m_cell_size = size;
}

Ref<Texture3DRD> ForceField::get_texture() const {
    return m_texture;
}

void ForceField::set_texture(const Ref<Texture3DRD> &texture) {
    m_texture = texture;

    texture->set_texture_rd_rid(m_rd_texture);
    UtilityFunctions::print("Setting texture rendering device resource on given texture.");
}

Vector3 ForceField::get_emitter_position_min() const {
    return m_emitter_min;
}

void ForceField::set_emitter_position_min(const Vector3 pos) {
    m_emitter_min = pos;
    update_emitter_buffer();
}

Vector3 ForceField::get_emitter_position_max() const {
    return m_emitter_max;
}

void ForceField::set_emitter_position_max(Vector3 pos) {
    m_emitter_max = pos;
    update_emitter_buffer();
}

Vector3 ForceField::get_emitter_velocity() const {
    return m_emitter_velocity;
}

void ForceField::set_emitter_velocity(Vector3 velocity) {
    m_emitter_velocity = velocity;
    update_emitter_buffer();
}

void ForceField::init_compute() {
    UtilityFunctions::print("Initializing compute shaders ...");

    m_device = RenderingServer::get_singleton()->get_rendering_device();

    m_velocity_buffers1.u = create_velocity_storage_buffer();
    m_velocity_buffers1.v = create_velocity_storage_buffer();
    m_velocity_buffers1.w = create_velocity_storage_buffer();

    m_velocity_buffers2.u = create_velocity_storage_buffer();
    m_velocity_buffers2.v = create_velocity_storage_buffer();
    m_velocity_buffers2.w = create_velocity_storage_buffer();

    m_solid_buffer = create_solid_storage_buffer(true);
    m_pressure_buffer = create_pressure_buffer();
    m_grid_params_buffer = create_grid_params_buffer();
    m_rd_texture = create_texture();
    m_emitter_buffer = create_emitter_buffer();

    if (m_texture.is_valid()) {
        m_texture->set_texture_rd_rid(m_rd_texture);
    }

    init_integrate_pass(m_velocity_buffers2, m_velocity_buffers1, m_solid_buffer, m_pressure_buffer, m_grid_params_buffer, m_emitter_buffer);
    init_incompressibility_pass(m_velocity_buffers1, m_solid_buffer, m_pressure_buffer, m_grid_params_buffer);
    init_extrapolation_pass(m_velocity_buffers1, m_grid_params_buffer);
    init_advect_pass(m_velocity_buffers1, m_velocity_buffers2, m_solid_buffer, m_grid_params_buffer);
    init_copy_to_texture_pass(m_velocity_buffers2, m_rd_texture, m_pressure_buffer, m_solid_buffer, m_grid_params_buffer);

    UtilityFunctions::print("Done.");

    m_compute_ready = true;
}

void ForceField::init_integrate_pass(const VelocityBuffers &velocity_in, const VelocityBuffers &velocity_out,
                                     const RID &solids, const RID& pressure, const RID &grid_parameters, const RID& emitter_buffer) {
    ResourceLoader *const loader = ResourceLoader::get_singleton();
    const Ref<RDShaderFile> shader_file = loader->load("res://extensions/force-field/shaders/integrate.glsl");
    const auto shader = m_device->shader_create_from_spirv(shader_file->get_spirv());

    m_integrate_pass.velocity_in_set = create_velocity_set(velocity_in, shader, 0);
    m_integrate_pass.velocity_out_set = create_velocity_set(velocity_out, shader, 1);
    m_integrate_pass.pressure_set = create_pressure_set(pressure, shader, 2);
    m_integrate_pass.solid_set = create_solid_set(solids, shader, 3);
    m_integrate_pass.grid_parameters_set = create_grid_parameters_set(grid_parameters, shader, 4);
    m_integrate_pass.emitter_set = create_emitter_set(emitter_buffer, shader, 5);
    m_integrate_pass.pipeline = m_device->compute_pipeline_create(shader);
    m_integrate_pass.shader = shader;
}

void ForceField::init_incompressibility_pass(const VelocityBuffers &velocity, const RID &solid,
                                             const RID& pressure, const RID &grid_parameters) {
    ResourceLoader *const loader = ResourceLoader::get_singleton();
    const Ref<RDShaderFile> shader_file = loader->load(
        "res://extensions/force-field/shaders/solve_incompressibility.glsl");
    const auto shader = m_device->shader_create_from_spirv(shader_file->get_spirv());

    m_incompressibility_pass.velocity_set = create_velocity_set(velocity, shader, 0);
    m_incompressibility_pass.solid_set = create_solid_set(solid, shader, 1);
    m_incompressibility_pass.pressure_set = create_pressure_set(pressure, shader, 2);
    m_incompressibility_pass.grid_parameters_set = create_grid_parameters_set(grid_parameters, shader, 3);
    m_incompressibility_pass.pipeline = m_device->compute_pipeline_create(shader);
    m_incompressibility_pass.shader = shader;
}

void ForceField::init_extrapolation_pass(const VelocityBuffers &velocity, const RID &grid_parameters) {
    ResourceLoader *const loader = ResourceLoader::get_singleton();
    const Ref<RDShaderFile> shader_file = loader->load(
        "res://extensions/force-field/shaders/extrapolation.glsl");
    const auto shader = m_device->shader_create_from_spirv(shader_file->get_spirv());

    m_extrapolation_pass.velocity_set = create_velocity_set(velocity, shader, 0);
    m_extrapolation_pass.grid_parameters_set = create_grid_parameters_set(grid_parameters, shader, 1);
    m_extrapolation_pass.pipeline = m_device->compute_pipeline_create(shader);
    m_extrapolation_pass.shader = shader;
}

void ForceField::init_advect_pass(const VelocityBuffers &velocity_in, const VelocityBuffers &velocity_out,
                                  const RID &solid, const RID &grid_parameters) {
    ResourceLoader *const loader = ResourceLoader::get_singleton();
    const Ref<RDShaderFile> shader_file = loader->load(
        "res://extensions/force-field/shaders/advection.glsl");
    const auto shader = m_device->shader_create_from_spirv(shader_file->get_spirv());

    m_advection_pass.velocity_in_set = create_velocity_set(velocity_in, shader, 0);
    m_advection_pass.velocity_out_set = create_velocity_set(velocity_out, shader, 1);
    m_advection_pass.solid_set = create_solid_set(solid, shader, 2);
    m_advection_pass.grid_parameters_set = create_grid_parameters_set(grid_parameters, shader, 3);
    m_advection_pass.pipeline = m_device->compute_pipeline_create(shader);
    m_advection_pass.shader = shader;
}

void ForceField::init_copy_to_texture_pass(const VelocityBuffers &velocity, const RID &texture, const RID& pressure, const RID &solid,
                                           const RID &grid_parameters) {
    ResourceLoader *const loader = ResourceLoader::get_singleton();
    const Ref<RDShaderFile> shader_file = loader->load("res://extensions/force-field/shaders/copy_to_texture.glsl");
    const auto shader = m_device->shader_create_from_spirv(shader_file->get_spirv());

    TypedArray<RDUniform> texture_uniforms;
    Ref<RDUniform> texture_uniform;
    texture_uniform.instantiate();

    texture_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    texture_uniform->set_binding(0);
    texture_uniform->add_id(texture);
    texture_uniforms.push_back(texture_uniform);

    m_transfer_to_texture_pass.velocity_set = create_velocity_set(velocity, shader, 0);
    m_transfer_to_texture_pass.pressure_set = create_pressure_set(pressure, shader, 1);
    m_transfer_to_texture_pass.texture_set = m_device->uniform_set_create(texture_uniforms, shader, 2);
    m_transfer_to_texture_pass.solid_set = create_solid_set(solid, shader, 3);
    m_transfer_to_texture_pass.grid_parameters_set = create_grid_parameters_set(grid_parameters, shader, 4);
    m_transfer_to_texture_pass.pipeline = m_device->compute_pipeline_create(shader);
    m_transfer_to_texture_pass.shader = shader;
}

void ForceField::run_compute() {
    constexpr float delta_time = 0.016;
    const int groups_x = m_field_size.x / 8;
    const int groups_y = m_field_size.y / 8;
    const int groups_z = m_field_size.z / 8;

    const PackedFloat32Array push_values{
        delta_time, 0.0, 0.0, 0.0
    };
    const PackedByteArray push_constants{push_values.to_byte_array()};

    {
        const auto cl = m_device->compute_list_begin();

        m_device->compute_list_bind_compute_pipeline(cl, m_integrate_pass.pipeline);
        m_device->compute_list_bind_uniform_set(cl, m_integrate_pass.velocity_in_set, 0);
        m_device->compute_list_bind_uniform_set(cl, m_integrate_pass.velocity_out_set, 1);
        m_device->compute_list_bind_uniform_set(cl, m_integrate_pass.pressure_set, 2);
        m_device->compute_list_bind_uniform_set(cl, m_integrate_pass.solid_set, 3);
        m_device->compute_list_bind_uniform_set(cl, m_integrate_pass.grid_parameters_set, 4);
        m_device->compute_list_bind_uniform_set(cl, m_integrate_pass.emitter_set, 5);
        m_device->compute_list_set_push_constant(cl, push_constants, push_constants.size());
        m_device->compute_list_dispatch(cl, groups_x, groups_y, groups_z);
        m_device->compute_list_end();
    }

    for (int i = 0; i < 100; ++i) {
        const auto cl = m_device->compute_list_begin();

        m_device->compute_list_bind_compute_pipeline(cl, m_incompressibility_pass.pipeline);
        m_device->compute_list_bind_uniform_set(cl, m_incompressibility_pass.velocity_set, 0);
        m_device->compute_list_bind_uniform_set(cl, m_incompressibility_pass.solid_set, 1);
        m_device->compute_list_bind_uniform_set(cl, m_incompressibility_pass.pressure_set, 2);
        m_device->compute_list_bind_uniform_set(cl, m_incompressibility_pass.grid_parameters_set, 3);

        const auto incomp_push_constants = get_incompressibility_push_constants(delta_time, i);
        m_device->compute_list_set_push_constant(cl, incomp_push_constants, incomp_push_constants.size());

        m_device->compute_list_dispatch(cl, groups_x / 2, groups_y / 2, groups_z / 2);
        m_device->compute_list_end();
    }

    {
        const auto cl = m_device->compute_list_begin();

        m_device->compute_list_bind_compute_pipeline(cl, m_extrapolation_pass.pipeline);
        m_device->compute_list_bind_uniform_set(cl, m_extrapolation_pass.velocity_set, 0);
        m_device->compute_list_bind_uniform_set(cl, m_extrapolation_pass.grid_parameters_set, 1);
        m_device->compute_list_set_push_constant(cl, push_constants, push_constants.size());
        m_device->compute_list_dispatch(cl, groups_x, groups_y, groups_z);
        m_device->compute_list_end();
    }

    {
        const auto cl = m_device->compute_list_begin();

        m_device->compute_list_bind_compute_pipeline(cl, m_advection_pass.pipeline);
        m_device->compute_list_bind_uniform_set(cl, m_advection_pass.velocity_in_set, 0);
        m_device->compute_list_bind_uniform_set(cl, m_advection_pass.velocity_out_set, 1);
        m_device->compute_list_bind_uniform_set(cl, m_advection_pass.solid_set, 2);
        m_device->compute_list_bind_uniform_set(cl, m_advection_pass.grid_parameters_set, 3);
        m_device->compute_list_set_push_constant(cl, push_constants, push_constants.size());
        m_device->compute_list_dispatch(cl, groups_x, groups_y, groups_z);
        m_device->compute_list_end();
    }

    {
        const auto cl = m_device->compute_list_begin();

        m_device->compute_list_bind_compute_pipeline(cl, m_transfer_to_texture_pass.pipeline);
        m_device->compute_list_bind_uniform_set(cl, m_transfer_to_texture_pass.velocity_set, 0);
        m_device->compute_list_bind_uniform_set(cl, m_transfer_to_texture_pass.pressure_set, 1);
        m_device->compute_list_bind_uniform_set(cl, m_transfer_to_texture_pass.texture_set, 2);
        m_device->compute_list_bind_uniform_set(cl, m_transfer_to_texture_pass.solid_set, 3);
        m_device->compute_list_bind_uniform_set(cl, m_transfer_to_texture_pass.grid_parameters_set, 4);
        m_device->compute_list_set_push_constant(cl, push_constants, push_constants.size());
        m_device->compute_list_dispatch(cl, groups_x, groups_y, groups_z);
        m_device->compute_list_end();
    }

    if (m_print_debug_info) {
        m_print_debug_info = false;
        m_device->buffer_get_data_async(m_velocity_buffers1.u, callable_mp(this, &ForceField::read_velocity_buffer));
        m_device->buffer_get_data_async(m_velocity_buffers1.v, callable_mp(this, &ForceField::read_velocity_buffer));
        m_device->buffer_get_data_async(m_velocity_buffers1.w, callable_mp(this, &ForceField::read_velocity_buffer));
        m_device->buffer_get_data_async(m_pressure_buffer, callable_mp(this, &ForceField::read_velocity_buffer));
    }
}

RID ForceField::create_velocity_storage_buffer() const {
    PackedFloat32Array data;
    data.resize(
        m_field_size.x * m_field_size.y * m_field_size.z
    );
    data.fill(0.0);

    const PackedByteArray bytes = data.to_byte_array();

    return m_device->storage_buffer_create(bytes.size(), bytes, 0, RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
}

RID ForceField::create_grid_params_buffer() const {
    const PackedInt32Array buffer_part1{
        m_field_size.x,
        m_field_size.y,
        m_field_size.z,
    };
    const PackedFloat32Array buffer_part2{m_cell_size};

    PackedByteArray bytes{buffer_part1.to_byte_array()};
    bytes.append_array(buffer_part2.to_byte_array());

    return m_device->uniform_buffer_create(bytes.size(), bytes, RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
}

RID ForceField::create_solid_storage_buffer(bool walls) const {
    PackedFloat32Array buffer;
    buffer.resize(
        m_field_size.x * m_field_size.y * m_field_size.z
    );
    buffer.fill(1.0);

    if (walls) {
        for (int i = 0; i < m_field_size.x; ++i) {
            buffer[to_index(i, 0, 0)] = 0.0;
            buffer[to_index(i, m_field_size.y - 1, m_field_size.z - 1)] = 0.0;
        }
        for (int j = 0; j < m_field_size.y; ++j) {
            buffer[to_index(0, j, 0)] = 0.0;
            buffer[to_index(m_field_size.x - 1, j, m_field_size.z - 1)] = 0.0;
        }
        for (int k = 0; k < m_field_size.y; ++k) {
            buffer[to_index(0, 0, k)] = 0.0;
            buffer[to_index(m_field_size.x - 1, m_field_size.y - 1, k)] = 0.0;
        }
    }

    const PackedByteArray bytes = buffer.to_byte_array();
    return m_device->storage_buffer_create(bytes.size(), bytes, 0, RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
}

RID ForceField::create_texture() const {
    Ref<RDTextureFormat> texture_format;
    texture_format.instantiate();

    texture_format->set_format(RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
    texture_format->set_texture_type(RenderingDevice::TEXTURE_TYPE_3D);
    texture_format->set_width(m_field_size.x);
    texture_format->set_height(m_field_size.y);
    texture_format->set_depth(m_field_size.z);
    texture_format->set_array_layers(1);
    texture_format->set_mipmaps(1);
    texture_format->set_usage_bits(
        RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT |
        RenderingDevice::TEXTURE_USAGE_STORAGE_BIT |
        RenderingDevice::TEXTURE_USAGE_CAN_UPDATE_BIT |
        RenderingDevice::TEXTURE_USAGE_CAN_COPY_TO_BIT |
        RenderingDevice::TEXTURE_USAGE_CAN_COPY_FROM_BIT
    );

    Ref<RDTextureView> texture_view;
    texture_view.instantiate();

    auto texture_rid = m_device->texture_create(texture_format, texture_view);

    m_device->texture_clear(texture_rid, Color(1.0, 0.0, 0.0, 1.0), 0, 1, 0, 1);

    return texture_rid;
}

RID ForceField::create_emitter_buffer() const {
    const auto bytes = create_emitter_bytes(m_emitter_min, m_emitter_max, m_emitter_velocity);
    return m_device->uniform_buffer_create(bytes.size(), bytes, RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
}

void ForceField::update_emitter_buffer() const {
    if (!m_emitter_buffer.is_valid()) {
        return;
    }
    const auto bytes = create_emitter_bytes(m_emitter_min, m_emitter_max, m_emitter_velocity);
    m_device->buffer_update(m_emitter_buffer, 0.0, bytes.size(), bytes);
}

RID ForceField::create_pressure_buffer() const {
    PackedFloat32Array data;
    data.resize(m_field_size.x * m_field_size.y * m_field_size.z);
    data.fill(0.0);

    const PackedByteArray bytes = data.to_byte_array();

    return m_device->storage_buffer_create(bytes.size(), bytes, 0, RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
}

RID ForceField::create_velocity_set(const VelocityBuffers &storage_buffers, const RID &shader, int set) const {
    TypedArray<RDUniform> uniforms;
    Ref<RDUniform> u_uniform, v_uniform, w_uniform;

    u_uniform.instantiate();
    v_uniform.instantiate();
    w_uniform.instantiate();

    u_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    u_uniform->set_binding(0);
    u_uniform->add_id(storage_buffers.u);

    v_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    v_uniform->set_binding(1);
    v_uniform->add_id(storage_buffers.v);

    w_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    w_uniform->set_binding(2);
    w_uniform->add_id(storage_buffers.w);

    uniforms.push_back(u_uniform);
    uniforms.push_back(v_uniform);
    uniforms.push_back(w_uniform);

    return m_device->uniform_set_create(uniforms, shader, set);
}

RID ForceField::create_grid_parameters_set(const RID &parameter_buffer, const RID &shader, int set) const {
    TypedArray<RDUniform> uniforms;
    Ref<RDUniform> uniform;
    uniform.instantiate();

    uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER);
    uniform->set_binding(0);
    uniform->add_id(parameter_buffer);

    uniforms.push_back(uniform);

    return m_device->uniform_set_create(uniforms, shader, set);
}

RID ForceField::create_solid_set(const RID &solid_buffer, const RID &shader, int set) const {
    TypedArray<RDUniform> uniforms;
    Ref<RDUniform> uniform;
    uniform.instantiate();

    uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    uniform->set_binding(0);
    uniform->add_id(solid_buffer);

    uniforms.push_back(uniform);

    return m_device->uniform_set_create(uniforms, shader, set);
}

RID ForceField::create_emitter_set(const RID &emitter_buffer, const RID &shader, int set) const {
    TypedArray<RDUniform> uniforms;
    Ref<RDUniform> uniform;
    uniform.instantiate();

    uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER);
    uniform->set_binding(0);
    uniform->add_id(emitter_buffer);

    uniforms.push_back(uniform);

    return m_device->uniform_set_create(uniforms, shader, set);
}

RID ForceField::create_pressure_set(const RID& pressure_buffer, const RID &shader, int set) const {
    TypedArray<RDUniform> uniforms;
    Ref<RDUniform> uniform;
    uniform.instantiate();

    uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    uniform->set_binding(0);
    uniform->add_id(pressure_buffer);

    uniforms.push_back(uniform);

    return m_device->uniform_set_create(uniforms, shader, set);
}

int ForceField::to_index(int i, int j, int k) const {
    return k * m_field_size.x * m_field_size.y + j * m_field_size.x + i;
}

PackedByteArray ForceField::get_incompressibility_push_constants(float delta_time, int iteration) {
    const PackedFloat32Array float_values {delta_time};
    const PackedInt32Array int_values{iteration % 2, 0, 0 };

    PackedByteArray bytes { float_values.to_byte_array() };
    bytes.append_array(int_values.to_byte_array());

    return std::move(bytes);
}

void ForceField::read_velocity_buffer(const PackedByteArray &buffer) {
    const auto& vel = buffer.to_float32_array();
    double min_v = vel.get(0);
    double max_v = vel.get(0);

    for (int i=0; i < vel.size(); ++i) {
        double value = vel.get(i);

        if (value > max_v) {
            max_v = value;
        }

        if (value < min_v) {
            min_v = value;
        }
    }

    UtilityFunctions::print("Min: ", min_v, " Max: ", max_v, " Count: ", vel.size());
}

PackedByteArray ForceField::create_emitter_bytes(Vector3 min, Vector3 max, Vector3 velocity) {
    const PackedFloat32Array buffer{
        min.x, min.y, min.z, 0.0,
        max.x, max.y, max.z, 0.0,
        velocity.x, velocity.y, velocity.z, 0.0,
    };

    return PackedByteArray { buffer.to_byte_array() };
}
