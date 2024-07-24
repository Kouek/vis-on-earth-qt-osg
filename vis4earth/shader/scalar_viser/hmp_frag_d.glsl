#version 130

uniform sampler3D volTex;
uniform sampler1D colTblTex;
uniform float minLatitute;
uniform float maxLatitute;
uniform float minLongtitute;
uniform float maxLongtitute;
uniform float minHeight;
uniform float maxHeight;
uniform int volStartFromZeroLon;

varying vec3 vertex;

void main() {
    float r = sqrt(vertex.x * vertex.x + vertex.y * vertex.y);
    float lat = atan(vertex.z / r);
    r = length(vertex);
    float lon = atan(vertex.y, vertex.x);
    if (lat < minLatitute || lat > maxLatitute || lon < minLongtitute || lon > maxLongtitute)
        discard;

    float hDlt = maxHeight - minHeight;
    float latDlt = maxLatitute - minLatitute;
    float lonDlt = maxLongtitute - minLongtitute;

    r = (r - minHeight) / hDlt;
    lat = (lat - minLatitute) / latDlt;
    lon = (lon - minLongtitute) / lonDlt;
    if (volStartFromZeroLon != 0)
        if (lon < .5f) lon += .5f;
        else lon -= .5f;

    float scalar = texture(volTex, vec3(lon, lat, r)).r;
    vec4 col = texture(colTblTex, scalar);
    gl_FragColor.rgb = col.rgb;
    gl_FragColor.a = 1.f;
}