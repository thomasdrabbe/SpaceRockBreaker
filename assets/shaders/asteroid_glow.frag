// Fragment-only (SFML default vertex). Additieve gloed op basis van texture-alpha.
// Zelfde stijl als SFML-voorbeelden: gl_TexCoord / texture2D / gl_FragColor.
uniform sampler2D texture;
uniform vec3 glow_rgb;
uniform vec2 texel_size;
uniform float glow_strength;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec4 base = texture2D(texture, uv) * gl_Color;
    float a0 = base.a;

    float spread = a0;
    vec2 ts = texel_size;

    // Morphologische dilatie in twee ringen (max-alpha in buurt).
    spread = max(spread, texture2D(texture, uv + vec2(ts.x, 0.0)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv - vec2(ts.x, 0.0)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv + vec2(0.0, ts.y)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv - vec2(0.0, ts.y)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv + ts).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv - ts).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv + vec2(ts.x, -ts.y)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv + vec2(-ts.x, ts.y)).a * gl_Color.a);

    vec2 ts2 = ts * 2.0;
    spread = max(spread, texture2D(texture, uv + vec2(ts2.x, 0.0)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv - vec2(ts2.x, 0.0)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv + vec2(0.0, ts2.y)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv - vec2(0.0, ts2.y)).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv + ts2).a * gl_Color.a);
    spread = max(spread, texture2D(texture, uv - ts2).a * gl_Color.a);

    float halo = clamp(spread - a0, 0.0, 1.0);
    vec3 rgb = glow_rgb * halo * glow_strength;
    float alpha = halo * glow_strength * 0.92;
    gl_FragColor = vec4(rgb, alpha);
}
