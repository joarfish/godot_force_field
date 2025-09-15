#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>

#include "godot_cpp/classes/texture3drd.hpp"
#include "godot_cpp/classes/input_event.hpp"

namespace godot {

class ForceField : public Node3D {
    GDCLASS(ForceField, Node3D)

    RenderingDevice* m_device;

    struct VelocityBuffers {
        RID u;
        RID v;
        RID w;
    };

    VelocityBuffers m_velocity_buffers1;
    VelocityBuffers m_velocity_buffers2;
    RID m_solid_buffer;
    RID m_grid_params_buffer;
    RID m_emitter_buffer;
    RID m_pressure_buffer;

    struct IntegratePass {
        RID pipeline;
        RID velocity_in_set;
        RID velocity_out_set;
        RID solid_set;
        RID grid_parameters_set;
        RID emitter_set;
        RID pressure_set;
        RID shader;
    };

    struct IncompressibilityPass {
        RID pipeline;
        RID shader;
        RID velocity_set;
        RID solid_set;
        RID pressure_set;
        RID grid_parameters_set;
    };

    struct ExtrapolationPass {
        RID pipeline;
        RID shader;
        RID velocity_set;
        RID grid_parameters_set;
    };

    struct AdvectionPass {
        RID pipeline;
        RID velocity_in_set;
        RID velocity_out_set;
        RID solid_set;
        RID grid_parameters_set;
        RID shader;
    };

    struct TransferToTexturePass {
        RID pipeline;
        RID shader;
        RID velocity_set;
        RID pressure_set;
        RID texture_set;
        RID solid_set;
        RID grid_parameters_set;
    };

    IntegratePass m_integrate_pass;
    IncompressibilityPass m_incompressibility_pass;
    ExtrapolationPass m_extrapolation_pass;
    AdvectionPass m_advection_pass;
    TransferToTexturePass m_transfer_to_texture_pass;

    RID m_rd_texture;

    bool m_compute_ready { false };
    bool m_print_debug_info { false };

    void init_integrate_pass(const VelocityBuffers& velocity_in, const VelocityBuffers& velocity_out, const RID& solids, const RID& pressure, const RID& grid_parameters, const RID& emitter_buffer);
    void init_incompressibility_pass(const VelocityBuffers& velocity, const RID& solid, const RID& pressure, const RID& grid_parameters);
    void init_extrapolation_pass(const VelocityBuffers& velocity, const RID& grid_parameters);
    void init_advect_pass(const VelocityBuffers& velocity_in, const VelocityBuffers& velocity_out, const RID& solid, const RID& grid_parameters);
    void init_copy_to_texture_pass(const VelocityBuffers& velocity, const RID& texture, const RID& pressure, const RID& solid, const RID& grid_parameters);

    void init_compute();

    void run_compute();

    [[nodiscard]] RID create_velocity_storage_buffer() const;
    [[nodiscard]] RID create_grid_params_buffer() const;
    [[nodiscard]] RID create_solid_storage_buffer(bool walls) const;
    [[nodiscard]] RID create_texture() const;
    [[nodiscard]] RID create_emitter_buffer() const;
    void update_emitter_buffer() const;
    [[nodiscard]] static PackedByteArray create_emitter_bytes(Vector3 min, Vector3 max, Vector3 velocity);
    [[nodiscard]] RID create_pressure_buffer() const;

    [[nodiscard]] RID create_velocity_set(const VelocityBuffers &storage_buffers, const RID &shader, int set) const;
    [[nodiscard]] RID create_grid_parameters_set(const RID& parameter_buffer, const RID& shader, int set) const;
    [[nodiscard]] RID create_solid_set(const RID& solid_buffer, const RID& shader, int set) const;
    [[nodiscard]] RID create_emitter_set(const RID& emitter_buffer, const RID& shader, int set) const;
    [[nodiscard]] RID create_pressure_set(const RID& pressure_buffer, const RID& shader, int set) const;

    int to_index(int i, int j, int k) const;
    [[nodiscard]] static PackedByteArray get_incompressibility_push_constants(float delta_time, int iteration);

    void read_velocity_buffer(const PackedByteArray& buffer);

protected:
    static void _bind_methods();

    Vector3i m_field_size;
    float m_cell_size;
    Ref<Texture3DRD> m_texture;
    Vector3 m_emitter_min { 0.44, 0.44, 0.1 };
    Vector3 m_emitter_max { 0.54, 0.54, 0.1 };
    Vector3 m_emitter_velocity { 0.0, 0.0, 15.82 };

public:
    ForceField();

    void _ready() override;
    void _process(double delta) override;
    void _input(const Ref<InputEvent>& event) override;

    Vector3i get_field_size() const;
    void set_field_size(Vector3i size);

    float get_cell_size() const;
    void set_cell_size(float size);

    Ref<Texture3DRD> get_texture() const;
    void set_texture(const Ref<Texture3DRD>& texture);

    Vector3 get_emitter_position_min() const;
    void set_emitter_position_min(Vector3 pos);

    Vector3 get_emitter_position_max() const;
    void set_emitter_position_max(Vector3 pos);

    Vector3 get_emitter_velocity() const;
    void set_emitter_velocity(Vector3 pos);
};

}
