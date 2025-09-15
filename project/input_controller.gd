extends MarginContainer

@export
var points_node: MeshInstance3D

@export
var force_field: ForceField

@export
var clip_x_slider: Slider

@export
var vel_min_slider: Slider

@export
var vel_max_slider: Slider

@export
var emitter_pos_z_slider: Slider

@export
var emitter_magnitude_slider: Slider

@export
var show_pressure_checkbox: CheckBox

func _ready() -> void:
	clip_x_slider.value_changed.connect(handle_clip_x_changed)
	vel_min_slider.value_changed.connect(handle_vel_min_changed)
	vel_max_slider.value_changed.connect(handle_vel_max_changed)
	emitter_pos_z_slider.value_changed.connect(handle_emitter_pos_z_changed)
	emitter_magnitude_slider.value_changed.connect(handle_emitter_velocity_changed)
	show_pressure_checkbox.toggled.connect(handle_show_pressure_changed)
	
	var clip_x: float = points_node.get("clip_x")
	clip_x_slider.set_value_no_signal(clip_x)
	
	var mag_min: float = points_node.get("magnitude_min")
	vel_min_slider.set_value_no_signal(mag_min)
	
	var mag_max: float = points_node.get("magnitude_max")
	vel_max_slider.set_value_no_signal(mag_max)
	
	var emitter_pos_z: float = force_field.emitter_pos_min.z
	emitter_pos_z_slider.set_value_no_signal(emitter_pos_z)
	
	var emitter_magnitude: float = force_field.emitter_velocity.z
	emitter_magnitude_slider.set_value_no_signal(emitter_magnitude)
	
	var show_pressure: bool = points_node.get("show_pressure")
	show_pressure_checkbox.set_pressed_no_signal(show_pressure)
	
func handle_clip_x_changed(val: float):
	points_node.set("clip_x", val)
	
func handle_vel_min_changed(val: float):
	points_node.set("magnitude_min", val)

func handle_vel_max_changed(val: float):
	points_node.set("magnitude_max", val)
	
func handle_emitter_pos_z_changed(val: float):
	force_field.emitter_pos_max.z = val + 0.1
	force_field.emitter_pos_min.z = val
	
func handle_emitter_velocity_changed(val: float):
	force_field.emitter_velocity.z = val
	
func handle_show_pressure_changed(val: bool):
	points_node.set("show_pressure", val)
