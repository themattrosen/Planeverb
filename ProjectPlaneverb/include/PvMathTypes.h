#pragma once
#include <cmath>

// Enable different bit-depth for floating point numbers
#define Real float

namespace Planeverb
{
	struct vec3
	{
		Real x, y, z;

		vec3(Real _x = 0.f, Real _y = 0.f, Real _z = 0.f) : x(_x), y(_y), z(_z) {}
		vec3& operator=(const vec3&) = default;
		vec3(const vec3& rhs) = default;
		~vec3() = default;
		vec3 operator-(const vec3& rhs) { return vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
		vec3 operator+(const vec3& rhs) { return vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
		vec3 operator*(Real scalar) { return vec3(x * scalar, y * scalar, z * scalar); }

		inline Real SquareLength() const { return x * x + y * y + z * z; }
		inline Real Length() const { return std::sqrt(SquareLength()); }
		inline Real SquareDistance(const vec3& rhs) { return vec3(*this - rhs).SquareLength(); }
		inline Real Distance(const vec3& rhs) { return std::sqrt(SquareDistance(rhs)); }
	};

	struct vec2
	{
		union
		{
			struct{	Real x, y; };
			Real m[2];
		};
		vec2(Real _x = 0.f, Real _y = 0.f) : x(_x), y(_y) {}
		vec2& operator=(const vec2&) = default;
		vec2(const vec2& rhs) = default;
		~vec2() = default;
		vec2 operator-(const vec2& rhs) const { return vec2(x - rhs.x, y - rhs.y); }
		vec2 operator+(const vec2& rhs) const { return vec2(x + rhs.x, y + rhs.y); }
		vec2 operator*(Real scalar) const { return vec2(x * scalar, y * scalar); }

		inline Real SquareLength() const { return x * x + y * y; }
		inline Real Length() const { return std::sqrt(SquareLength()); }
		inline Real SquareDistance(const vec2& rhs) const { return vec2(*this - rhs).SquareLength(); }
		inline Real Distance(const vec2& rhs) const { return std::sqrt(SquareDistance(rhs)); }
		inline void Normalize() { Real len(Length()); if (len > 0) { x /= len; y /= len; } }
		inline vec2 Normalized() const { vec2 ret(*this); ret.Normalize(); return ret; }
	};

	struct AABB
	{
		vec2 position = { 0, 0 };
		Real width = Real(0);
		Real height = Real(0);
		Real absorption = Real(0);

		/*
			 _________________
			|        |        | }
			|________p________| }\ = height
			|        |        | }/
			|________|________| }

			|_________________|
					^
					width
		*/
	};

	// absorption parameter R, defined as sqrt(1-absorption)
#define PV_ABSORPTION_FREE_SPACE				((Real)(0.000000000))
#define PV_ABSORPTION_DEFAULT					((Real)(0.989949494))
#define PV_ABSORPTION_BRICK_UNGLAZED			((Real)(0.979795897))
#define PV_ABSORPTION_BRICK_PAINTED				((Real)(0.989949494))
#define PV_ABSORPTION_CONCRETE_ROUGH			((Real)(0.969535971))
#define PV_ABSORPTION_CONCRETE_BLOCK_PAINTED	((Real)(0.964365076))
#define PV_ABSORPTION_GLASS_HEAVY				((Real)(0.984885780))
#define PV_ABSORPTION_GLASS_WINDOW				((Real)(0.938083152))
#define PV_ABSORPTION_TILE_GLAZED				((Real)(0.994987437))
#define PV_ABSORPTION_PLASTER_BRICK				((Real)(0.984885780))
#define PV_ABSORPTION_PLASTER_CONCRETE_BLOCK	((Real)(0.974679434))
#define PV_ABSORPTION_WOOD_PLYWOOD_PANEL		((Real)(0.948683298))
#define PV_ABSORPTION_STEEL						((Real)(0.948683298))
#define PV_ABSORPTION_WOOD_PANEL				((Real)(0.953939201))
#define PV_ABSORPTION_CONCRETE_BLOCK_COARSE		((Real)(0.806225775))
#define PV_ABSORPTION_DRAPERY_LIGHT				((Real)(0.921954446))
#define PV_ABSORPTION_DRAPERY_MEDIUM			((Real)(0.670820393))
#define PV_ABSORPTION_DRAPERY_HEAVY				((Real)(0.632455532))
#define PV_ABSORPTION_FIBERBOARD_SHREDDED_WOOD	((Real)(0.632455532))
#define PV_ABSORPTION_CONCRETE_PAINTED			((Real)(0.989949494))
#define PV_ABSORPTION_WOOD						((Real)(0.964365076))
#define PV_ABSORPTION_WOOD_VARNISHED			((Real)(0.984885780))
#define PV_ABSORPTION_CARPET_HEAVY				((Real)(0.806225775))
#define PV_ABSORPTION_GRAVEL					((Real)(0.547722558))
#define PV_ABSORPTION_GRASS						((Real)(0.547722558))
#define PV_ABSORPTION_SNOW_FRESH				((Real)(0.316227766))
#define PV_ABSORPTION_SOIL_ROUGH				((Real)(0.741619849))
#define PV_ABSORPTION_WOOD_TREE					((Real)(0.911043358))
#define PV_ABSORPTION_WATER_SURFACE				((Real)(0.994987437))
#define PV_ABSORPTION_CONCRETE					((Real)(0.979795897))
#define PV_ABSORPTION_GLASS						((Real)(0.969535971))
#define PV_ABSORPTION_MARBLE					((Real)(0.994987437))
#define PV_ABSORPTION_DRAPERY					((Real)(0.921954446))
#define PV_ABSORPTION_CLOTH						((Real)(0.921954446))
#define PV_ABSORPTION_AWNING					((Real)(0.921954446))
#define PV_ABSORPTION_FOLIAGE					((Real)(0.911043358))
#define PV_ABSORPTION_METAL						((Real)(0.948683298))
#define PV_ABSORPTION_ICE						((Real)(0.994987437))
#define PV_ABSORPTION_SNOW_PACKED				((Real)(0.994987437))

} // namespace Planeverb
