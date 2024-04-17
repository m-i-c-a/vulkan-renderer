#ifndef CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <limits>

namespace Camera
{
    glm::mat4 proj_mat { 1.0 };
    glm::mat4 view_mat { 1.0 };

    glm::vec3 xyz_dir { 0.0f, 0.0f, 0.0f };

    float sensitivity_xyz = 0.1f;
    float sensitivity_hpr = 0.1f;

    bool proj_dirty = true;
    bool view_dirty = true;

    void update_xyz_dir(const glm::vec3& xyz)
    {
        xyz_dir += xyz;
        view_dirty = xyz_dir != glm::vec3(0.0f);
    }

    void update_xyz()
    {
        view_mat[3] += sensitivity_xyz * glm::vec4(xyz_dir, 0.0f);
    }

    void update_hpr(const glm::vec3& delta_rotation)
    {

    }
};

#endif // CAMERA_HPP