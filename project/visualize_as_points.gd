extends MeshInstance3D

@export
var force_field_node: ForceField

@export_range(0.0, 1.0)
var clip_x: float:
	get():		
		var material: ShaderMaterial = self.material_override
		if material:
			return material.get_shader_parameter("clip_x")
		return 0.0
	set(value):
		var material: ShaderMaterial = self.material_override
		if material:
			material.set_shader_parameter("clip_x", value)

@export
var magnitude_min: float:
	get():		
		var material: ShaderMaterial = self.material_override
		if material:
			return material.get_shader_parameter("mag_min")
		return 0.0
	set(value):
		var material: ShaderMaterial = self.material_override
		if material:
			material.set_shader_parameter("mag_min", value)
			
@export
var magnitude_max: float:
	get():		
		var material: ShaderMaterial = self.material_override
		if material:
			return material.get_shader_parameter("mag_max")
		return 0.3
	set(value):
		var material: ShaderMaterial = self.material_override
		if material:
			material.set_shader_parameter("mag_max", value)
			
@export
var show_pressure: bool:
	get():
		var material: ShaderMaterial = self.material_override
		if material:
			return material.get_shader_parameter("show_pressure")
		return false
	set(value):
		var material: ShaderMaterial = self.material_override
		if material:
			material.set_shader_parameter("show_pressure", value)
		
func _ready() -> void:
	if !force_field_node:
		push_error("No force field node.")
		return
		
	var texture = force_field_node.get("texture")
	var field_size = force_field_node.get("field_size")
	
	if !texture:
		push_error("The force field node is missing a texture.")
		return
		
	var surface_array = []
	surface_array.resize(Mesh.ARRAY_MAX)
	var verts = PackedVector3Array()
	
	var scale_f = Vector3(1.0, 1.0, 1.0) / (Vector3(field_size) - Vector3(1.0, 1.0, 1.0))
	
	for x in range(1.0, field_size.x - 2):
		for y in range(1.0, field_size.y - 2):
			for z in range(1.0, field_size.z - 2):
				verts.append(Vector3(x, y, z) * scale_f)
				
	surface_array[Mesh.ARRAY_VERTEX] = verts
	
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_POINTS, surface_array)
	
	var material: ShaderMaterial = self.material_override
	if material:
		material.set_shader_parameter("force_field", texture)
		material.set_shader_parameter("field_size", field_size)
