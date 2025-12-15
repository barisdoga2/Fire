#include "pch.h"
#include "EasyEntity.hpp"
#include "EasyModel.hpp"
#include "EasyAnimation.hpp"
#include "EasyAnimator.hpp"



EasyEntity::EasyEntity(EasyModel* model, EasyTransform transform) : model(model), transform(transform)
{

}

EasyEntity::EasyEntity(EasyModel* model, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) : model(model), transform({ position,rotation,scale })
{

}

bool EasyEntity::Update(double _dt)
{
    if (animator)
        animator->UpdateAnimation(_dt);
    else if (!animator && model->isRawDataLoadedToGPU && model->animations.size() > 0u)
        animator = new EasyAnimator(model->animations.at(0));
    return true;
}
