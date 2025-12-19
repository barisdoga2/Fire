#pragma once

#include "EasyRender.hpp"

class EasyModel;
class EasyAnimator;
class EasyEntity {
public:
    EasyModel* model;
    EasyAnimator* animator{};

    EasyTransform transform;

    EasyEntity(EasyModel* model, EasyTransform transform = EasyTransform{});
    EasyEntity(EasyModel* model, glm::vec3 position, glm::vec3 rotation = glm::vec3(0.f), glm::vec3 scale = glm::vec3(1.f));

    virtual bool Update(double _dt);

    virtual glm::mat4x4 TransformationMatrix() const;
};
