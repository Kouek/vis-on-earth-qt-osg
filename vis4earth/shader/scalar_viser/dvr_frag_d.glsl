#version 130

uniform sampler3D volTex;
uniform sampler1D tfTex;
uniform vec3 eyePos;
uniform vec3 sliceCntr;
uniform vec3 sliceDir;
uniform float dt;
uniform float minLatitute;
uniform float maxLatitute;
uniform float minLongtitute;
uniform float maxLongtitute;
uniform float minHeight;
uniform float maxHeight;
uniform int maxStepCnt;
uniform int volStartFromZeroLon;
uniform int useSlice;

varying vec3 vertex;

struct Hit {
	int isHit;
	float tEntry;
};

/*
* 函数: intersectInnerSphere
* 功能: 返回视线与内球相交的位置
* 参数:
* -- p: 视线与外球相交的位置
* -- e2pDir: 视点指向p的方向
*/
Hit intersectInnerSphere(vec3 p, vec3 e2pDir) {
	Hit hit = Hit(0, 0.f);
	vec3 p2c = -p;
	float l = dot(e2pDir, p2c);
	if (l <= 0.f)
		return hit;

	float m2 = maxHeight * maxHeight - l * l;
	float innerR2 = minHeight * minHeight;
	if (m2 >= innerR2)
		return hit;

	float q = sqrt(innerR2 - m2);
	hit.isHit = 1;
	hit.tEntry = l - q;
	return hit;
}

/*
* 函数: anotherIntersectionOuterSphere
* 功能: 返回视线与外球相交的另一个位置
* 参数:
* -- p: 视线与外球相交的第一个位置
* -- e2pDir: 视点指向p的方向
*/
float anotherIntersectionOuterSphere(vec3 p, vec3 e2pDir) {
	vec3 p2c = -p;
	float l = dot(e2pDir, p2c);
	return 2.f * l;
}

Hit intersectSlice(vec3 e2pDir) {
	Hit hit = Hit(0, 0.f);
	
	vec3 surfCntr;
	float lon = minLongtitute + sliceCntr.x * (maxLongtitute - minLongtitute);
	float lat = minLatitute + sliceCntr.y * (maxLatitute - minLatitute);
	float h = minHeight + sliceCntr.z * (maxHeight - minHeight);
	surfCntr.z = h * sin(lat);
	h = h * cos(lat);
	surfCntr.y = h * sin(lon);
	surfCntr.x = h * cos(lon);

	mat3 rotMat;
	vec3 surfN;
	lon = .5f * (maxLongtitute + minLongtitute);
	lat = .5f * (maxLatitute + minLatitute);
	h = .5f * (maxHeight + minHeight);
	surfN.z = h * sin(lat);
	h = h * cos(lat);
	surfN.y = h * sin(lon);
	surfN.x = h * cos(lon);
	surfN = normalize(surfN);
	rotMat[2] = surfN;
	rotMat[0] = cross(vec3(0, 0, 1), surfN);
	rotMat[1] = cross(surfN, rotMat[0]);
	surfN = rotMat * sliceDir;

	float dirDtN = dot(e2pDir, surfN);
	if (dirDtN >= 0.f) return hit;

	hit.isHit = 1;
	hit.tEntry = dot(surfN, surfCntr) - dot(surfN, eyePos);
	hit.tEntry /= dirDtN;
	return hit;
}

#define PI 3.14159
void main() {
	//#define TEST
	// 开启TEST宏，测试剔除功能
#ifdef TEST
	vec3 d = normalize(vertex - eyePos);
	vec3 pos = vertex;
	float r = sqrt(pos.x * pos.x + pos.y * pos.y);
	float lat = atan(pos.z / r);
	float lon = atan(pos.y, pos.x);

	int entryOutOfRng = 0;
	if (lat < minLatitute)
		entryOutOfRng |= 1;
	if (lat > maxLatitute)
		entryOutOfRng |= 2;
	if (lon < minLongtitute)
		entryOutOfRng |= 4;
	if (lon > maxLongtitute)
		entryOutOfRng |= 8;

	float tExit = anotherIntersectionOuterSphere(vertex, d);
	Hit hitInner = intersectInnerSphere(vertex, d);
	if (hitInner.isHit != 0)
		tExit = hitInner.tEntry;

	pos = vertex + tExit * d;
	r = sqrt(pos.x * pos.x + pos.y * pos.y);
	lat = atan(pos.z / r);
	lon = atan(pos.y, pos.x);
	if ((entryOutOfRng & 1) != 0 && lat < minLatitute) {
		gl_FragColor = vec4(1.f, 0.f, 0.f, .3f);
		return;
	}
	if ((entryOutOfRng & 2) != 0 && lat > maxLatitute) {
		gl_FragColor = vec4(0.f, 1.f, 0.f, .3f);
		return;
	}
	if ((entryOutOfRng & 4) != 0 && lon < minLongtitute) {
		gl_FragColor = vec4(0.f, 0.f, 1.f, .3f);
		return;
	}
	if ((entryOutOfRng & 8) != 0 && lon > maxLongtitute) {
		gl_FragColor = vec4(.5f, .5f, .5f, .3f);
		return;
	}

	float hDlt = maxHeight - minHeight;
	float latDlt = maxLatitute - minLatitute;
	float lonDlt = maxLongtitute - minLongtitute;

	if (useSlice != 0) {
		Hit hit = intersectSlice(d);
		if (hit.isHit != 0) {
			vec3 pos = eyePos + hit.tEntry * d;
			float r = sqrt(pos.x * pos.x + pos.y * pos.y);
			float lat = atan(pos.z / r);
			r = length(pos);
			float lon = atan(pos.y, pos.x);

			if (lat < minLatitute || lat > maxLatitute || lon < minLongtitute || lon > maxLongtitute) {}
			else {
				r = (r - minHeight) / hDlt;
				lat = (lat - minLatitute) / latDlt;
				lon = (lon - minLongtitute) / lonDlt;
				gl_FragColor = vec4(lon, lat, r, 1.f);
				return;
			}
		}
	}

	pos = vertex;
	r = sqrt(pos.x * pos.x + pos.y * pos.y);
	lat = atan(pos.z / r);
	r = length(pos);
	lon = atan(pos.y, pos.x);
	r = (r - minHeight) / hDlt;
	lat = (lat - minLatitute) / latDlt;
	lon = (lon - minLongtitute) / lonDlt;
	gl_FragColor = vec4(lon, lat, r, .75f);
#else
	vec3 d = normalize(vertex - eyePos);
	vec3 pos = vertex;
	float r = sqrt(pos.x * pos.x + pos.y * pos.y);
	float lat = atan(pos.z / r);
	r = length(pos);
	float lon = atan(pos.y, pos.x);
	// 判断视线与外球第一个交点（即进入体的位置）所在象限
	int entryOutOfRng = 0;
	if (lat < minLatitute)
		entryOutOfRng |= 1;
	if (lat > maxLatitute)
		entryOutOfRng |= 2;
	if (lon < minLongtitute)
		entryOutOfRng |= 4;
	if (lon > maxLongtitute)
		entryOutOfRng |= 8;
	float tExit = anotherIntersectionOuterSphere(vertex, d);
	Hit hit = intersectInnerSphere(vertex, d);
	if (hit.isHit != 0)
		tExit = hit.tEntry;
	// 判断视线离开体的位置所在象限
	pos = vertex + tExit * d;
	r = sqrt(pos.x * pos.x + pos.y * pos.y);
	lat = atan(pos.z / r);
	lon = atan(pos.y, pos.x);
	// 若两个位置均不在范围内，且所在象限相同，则不需要计算该视线
	if ((entryOutOfRng & 1) != 0 && lat < minLatitute)
		discard;
	if ((entryOutOfRng & 2) != 0 && lat > maxLatitute)
		discard;
	if ((entryOutOfRng & 4) != 0 && lon < minLongtitute)
		discard;
	if ((entryOutOfRng & 8) != 0 && lon > maxLongtitute)
		discard;
	float hDlt = maxHeight - minHeight;
	float latDlt = maxLatitute - minLatitute;
	float lonDlt = maxLongtitute - minLongtitute;
	vec4 color = vec4(0, 0, 0, 0);
	vec3 entry2Exit = pos - vertex;
	float tMax = length(entry2Exit);
	float tAcc = 0.f;
	pos = vertex;
	int stepCnt = 0;
	// 处理切面
	if (useSlice != 0) {
		hit = intersectSlice(d);
		if (hit.isHit != 0) {
			vec3 pos = eyePos + hit.tEntry * d;
			float r = sqrt(pos.x * pos.x + pos.y * pos.y);
			float lat = atan(pos.z / r);
			r = length(pos);
			float lon = atan(pos.y, pos.x);

			if (lat < minLatitute || lat > maxLatitute
				|| lon < minLongtitute || lon > maxLongtitute
				|| r < minHeight || r > maxHeight
				) {}
			else {
				r = (r - minHeight) / hDlt;
				lat = (lat - minLatitute) / latDlt;
				lon = (lon - minLongtitute) / lonDlt;
				if (volStartFromZeroLon != 0)
					if (lon < .5f) lon += .5f;
					else lon -= .5f;

				float scalar = texture(volTex, vec3(lon, lat, r)).r;
				gl_FragColor = texture(tfTex, scalar);
				gl_FragColor.a = 1.f;
				return;
			}
		}
	}
	// 执行光线传播算法
	do {
		r = sqrt(pos.x * pos.x + pos.y * pos.y);
		lat = atan(pos.z / r);
		r = length(pos);
		lon = atan(pos.y, pos.x);

		if (lat < minLatitute || lat > maxLatitute || lon < minLongtitute || lon > maxLongtitute) {}
		else {
			r = (r - minHeight) / hDlt;
			lat = (lat - minLatitute) / latDlt;
			lon = (lon - minLongtitute) / lonDlt;
			if (volStartFromZeroLon != 0)
				if (lon < .5f) lon += .5f;
				else lon -= .5f;

			float scalar = texture(volTex, vec3(lon, lat, r)).r;
			vec4 tfCol = texture(tfTex, scalar);
			color.rgb = color.rgb + (1.f - color.a) * tfCol.a * tfCol.rgb;
			color.a = color.a + (1.f - color.a) * tfCol.a;
			if (color.a > .95f)
				break;
		}

		pos += dt * d;
		tAcc += dt;
		++stepCnt;
		if (stepCnt > maxStepCnt) break;
	} while (tAcc < tMax);

	gl_FragColor = color;
#endif
}