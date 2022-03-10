#pragma once
#include "math.hpp"
#include "camera.hpp"
#include <vector>
struct Vertex{
    float3 pos;
    float3 normal;
    float2 uv;
};
struct Triangle{
    Vertex vertices[3];
};

// Ax + By + Cz + D = 0
struct Plane{
  float3 normal;
  float D;
};

enum class BoxVisibility{
  Invisible,
  Intersecting,
  FullyVisible
};

struct Frustum{
  enum PLANE_IDX : uint32_t
  {
    LEFT_PLANE_IDX   = 0,
    RIGHT_PLANE_IDX  = 1,
    BOTTOM_PLANE_IDX = 2,
    TOP_PLANE_IDX    = 3,
    NEAR_PLANE_IDX   = 4,
    FAR_PLANE_IDX    = 5,
    NUM_PLANES       = 6
  };

  Plane left_plane;
  Plane right_plane;
  Plane bottom_plane;
  Plane top_plane;
  Plane near_plane;
  Plane far_plane;

  const Plane& getPlane(PLANE_IDX Idx) const
  {

    const Plane* Planes = reinterpret_cast<const Plane*>(this);
    return Planes[static_cast<size_t>(Idx)];
  }

  Plane& getPlane(PLANE_IDX Idx)
  {
    Plane* Planes = reinterpret_cast<Plane*>(this);
    return Planes[static_cast<size_t>(Idx)];
  }
};

struct BoundBox3D{
    float3 min_p;
    float3 max_p;
};


static BoxVisibility GetBoxVisibilityAgainstPlane(const Plane& plane,
                                                  const BoundBox3D& box){
  const auto& normal = plane.normal;
  float3 max_point{
      (normal.x > 0.f) ? box.max_p.x : box.min_p.x,
      (normal.y > 0.f) ? box.max_p.y : box.min_p.y,
      (normal.z > 0.f) ? box.max_p.z : box.min_p.z
  };
  float d_max = dot(max_point,normal) + plane.D;
  if(d_max < 0.f) return BoxVisibility::Invisible;
  float3 min_point{
      (normal.x > 0.f) ? box.min_p.x : box.max_p.x,
      (normal.y > 0.f) ? box.min_p.y : box.max_p.y,
      (normal.z > 0.f) ? box.min_p.z : box.max_p.z
  };
  float d_min = dot(min_point,normal) + plane.D;
  if(d_min > 0.f) return BoxVisibility::FullyVisible;

  return BoxVisibility::Intersecting;
}

static BoxVisibility GetBoxVisibility(const Frustum& frustum,
                                      const BoundBox3D& box){
  uint32_t num_planes_inside = 0;
  for(uint32_t plane_idx = 0;plane_idx < Frustum::NUM_PLANES; plane_idx++){
    const auto& cur_plane = frustum.getPlane(static_cast<Frustum::PLANE_IDX>(plane_idx));

    auto visibility_against_plane = GetBoxVisibilityAgainstPlane(cur_plane,box);

    if(visibility_against_plane == BoxVisibility::Invisible)
      return BoxVisibility::Invisible;

    if(visibility_against_plane == BoxVisibility::FullyVisible)
      num_planes_inside++;
  }
  return (num_planes_inside == Frustum::NUM_PLANES) ? BoxVisibility::FullyVisible : BoxVisibility::Intersecting ;
}

inline bool FrustumIntersectWithBoundBox(const Frustum& frustum,const BoundBox3D& box){
    return GetBoxVisibility(frustum,box) != BoxVisibility::Invisible;
}
inline void ExtractFrustumFromProjViewMatrix(const mat4& matrix,Frustum& frustum,bool is_OpenGL = true){
  // Left clipping plane
  frustum.left_plane.normal.x = matrix[0][3] + matrix[0][0];
  frustum.left_plane.normal.y = matrix[1][3] + matrix[1][0];
  frustum.left_plane.normal.z = matrix[2][3] + matrix[2][0];
  frustum.left_plane.D = matrix[3][3] + matrix[3][0];

  // Right clipping plane
  frustum.right_plane.normal.x = matrix[0][3] - matrix[0][0];
  frustum.right_plane.normal.y = matrix[1][3] - matrix[1][0];
  frustum.right_plane.normal.z = matrix[2][3] - matrix[2][0];
  frustum.right_plane.D = matrix[3][3] - matrix[3][0];

  // Top clipping plane
  frustum.top_plane.normal.x = matrix[0][3] - matrix[0][1];
  frustum.top_plane.normal.y = matrix[1][3] - matrix[1][1];
  frustum.top_plane.normal.z = matrix[2][3] - matrix[2][1];
  frustum.top_plane.D = matrix[3][3] - matrix[3][1];

  // Bottom clipping plane
  frustum.bottom_plane.normal.x = matrix[0][3] + matrix[0][1];
  frustum.bottom_plane.normal.y = matrix[1][3] + matrix[1][1];
  frustum.bottom_plane.normal.z = matrix[2][3] + matrix[2][1];
  frustum.bottom_plane.D = matrix[3][3] + matrix[3][1];

  // Near clipping plane
  if (is_OpenGL)
  {
    // -w <= z <= w
    frustum.near_plane.normal.x = matrix[0][3] + matrix[0][2];
    frustum.near_plane.normal.y = matrix[1][3] + matrix[1][2];
    frustum.near_plane.normal.z = matrix[2][3] + matrix[2][2];
    frustum.near_plane.D = matrix[3][3] + matrix[3][2];
  }
  else
  {
    // 0 <= z <= w
    frustum.near_plane.normal.x = matrix[0][2];
    frustum.near_plane.normal.y = matrix[1][2];
    frustum.near_plane.normal.z = matrix[2][2];
    frustum.near_plane.D = matrix[3][2];
  }

  // Far clipping plane
  frustum.far_plane.normal.x = matrix[0][3] - matrix[0][2];
  frustum.far_plane.normal.y = matrix[1][3] - matrix[1][3];
  frustum.far_plane.normal.z = matrix[2][3] - matrix[2][2];
  frustum.far_plane.D = matrix[3][3] - matrix[3][2];
}

inline BoundBox3D GetBoundBoxFromCamera(const Camera& camera){
    float tan_fov = tan(radians(camera.zoom * 0.5f));
    float3 near_p = camera.position + camera.front * camera.z_near;
    float3 far_p = camera.position + camera.front * camera.z_far;
    float offset_y = tan_fov * camera.z_far;
    float offset_x = offset_y * camera.aspect;
    std::vector<float3> pts;
    pts.emplace_back(near_p+camera.right*offset_x+camera.up*offset_y);
    pts.emplace_back(near_p+camera.right*offset_x-camera.up*offset_y);
    pts.emplace_back(near_p-camera.right*offset_x+camera.up*offset_y);
    pts.emplace_back(near_p-camera.right*offset_x-camera.up*offset_y);
    pts.emplace_back(far_p+camera.right*offset_x+camera.up*offset_y);
    pts.emplace_back(far_p+camera.right*offset_x-camera.up*offset_y);
    pts.emplace_back(far_p-camera.right*offset_x+camera.up*offset_y);
    pts.emplace_back(far_p-camera.right*offset_x-camera.up*offset_y);
    float3 min_p = {std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
    float3 max_p = {-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
    for(auto &pt:pts){
        min_p.x = std::min(pt.x,min_p.x);
        min_p.y = std::min(pt.y,min_p.y);
        min_p.z = std::min(pt.z,min_p.z);
        max_p.x = std::max(pt.x,max_p.x);
        max_p.y = std::max(pt.y,max_p.y);
        max_p.z = std::max(pt.z,max_p.z);
    }
    return BoundBox3D{
        min_p,max_p
    };
}
