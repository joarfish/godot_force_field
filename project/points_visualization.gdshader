shader_type spatial;
render_mode unshaded, fog_disabled;

uniform sampler3D force_field;
uniform ivec3 field_size;
uniform float clip_x;
uniform float mag_min;
uniform float mag_max;
uniform bool show_pressure;

varying vec3 point_color;

const vec4 prisma_scale[8] = vec4[8](
    vec4(0.0,0.05038205347059877,0.029801736499741757,0.5279751010495176),
    vec4(0.14285714285714285,0.32784692303604196,0.0066313933705768055,0.6402853293744383),
    vec4(0.2857142857142857,0.5453608398097519,0.03836817688235455,0.6472432548304646),
    vec4(0.42857142857142855,0.7246542772727967,0.1974236709187686,0.5379281037132716),
    vec4(0.5714285714285714,0.8588363515132411,0.35929521887338184,0.407891799954962),
    vec4(0.7142857142857142,0.9557564842476064,0.5338287173328614,0.2850080723374925),
    vec4(0.8571428571428571,0.9945257260387773,0.7382691276441445,0.16745985897148677),
    vec4(1.0,0.9400151278782742,0.9751557856205376,0.131325887773911)
);

vec3 toColor(float v, float min_v, float max_v) {
    v = clamp(v, min_v, max_v - 0.00001);
    float v_n = (v - min_v) / (max_v - min_v);
    int idx_limit = 0;

    for (int i=0; i < prisma_scale.length(); i++) {
        idx_limit = v_n > prisma_scale[i].x ? i : idx_limit;
    }

    vec4 low = prisma_scale[idx_limit];
    vec4 high = prisma_scale[idx_limit + 1];
    float v_nn = (v_n - low.x) / (high.x - low.x);

    return mix(low.yzw, high.yzw, v_nn);
}


void vertex() {
	vec4 texel = texelFetch(force_field, ivec3(floor(VERTEX * vec3(field_size))), 0).xyzw;
	float magnitude = length(texel.xyz);

	if (VERTEX.x < clip_x) {
		VERTEX.x = 10000.0;
	}

	POINT_SIZE = 15.0;

	if (show_pressure) {
		point_color = toColor(texel.w, -340.0, 90.0);
	} else {
		point_color = toColor(magnitude, mag_min, mag_max);
	}
}

void fragment() {
	ALBEDO = point_color;
}
