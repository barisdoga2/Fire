#include "pch.h"
#include "EasyEntity.hpp"
#include "EasyModel.hpp"
#include "EasyAnimation.hpp"
#include "EasyAnimator.hpp"



EasyEntity::EasyEntity(EasyModel* model, EasyTransform transform) : model(model), transform(transform)
{
    
}

EasyEntity::EasyEntity(EasyModel* model, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) : model(model), transform( position,glm::quat(glm::radians(rotation)),scale)
{
    
}

bool EasyEntity::Update(double _dt)
{
    if (!assetReady)
    {
        if (model->isRawDataLoadedToGPU)
        {
            assetReady = true;
            AssetReadyCallback();
        }
    }

    return true;
}

void EasyEntity::AssetReadyCallback()
{
    if (!animator && model->animations.size() > 0u)
        animator = new EasyAnimator();
}

glm::mat4x4 EasyEntity::TransformationMatrix() const
{
    return CreateTransformMatrix(transform.position, transform.rotationQuat, transform.scale);
}
