#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <iostream>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/containers/vector.h>
#include <ozz/options/options.h>

#include "Model.hpp"
#include "Bone.hpp"
#include "PlaybackController.hpp"

class AnimatorCallback {
public:
	virtual void AnimationEnded() = 0;
};

class Animator
{
public:
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation* m_CurrentAnimation;
	float m_CurrentTime;
	float m_DeltaTime;

	// OZZ
	PlaybackController controller_;
	ozz::animation::SamplingJob::Context context_;
	ozz::vector<ozz::math::SoaTransform> locals_;
	ozz::vector<ozz::math::Float4x4> models_;
	ozz::animation::Animation* m_CurrentOZZAnimation;

	Animator(Animation* animation, ozz::animation::Animation* ozzAnimation)
	{
		m_DeltaTime = 0.0;
		m_CurrentTime = 0.0;
		m_CurrentAnimation = animation;
		m_CurrentOZZAnimation = ozzAnimation;

		m_FinalBoneMatrices.reserve(200);

		for (int i = 0; i < 200; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));

		// OZZ
		{
			const int num_soa_joints = animation->model->skeleton->num_soa_joints();
			locals_.resize(num_soa_joints);
			const int num_joints = animation->model->skeleton->num_joints();
			models_.resize(num_joints);

			context_.Resize(num_joints);
		}
	}

	bool UpdateAnimation(float dt)
	{
		m_DeltaTime = dt;
		if (m_CurrentAnimation)
		{
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
			m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
			CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
		}

		// OZZ
		{
			// Updates current animation time.
			controller_.Update(m_CurrentOZZAnimation, dt);

			// Samples optimized animation at t = animation_time_.
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = m_CurrentOZZAnimation;
			sampling_job.context = &context_;
			sampling_job.ratio = controller_.time_ratio();
			sampling_job.output = make_span(locals_);
			if (!sampling_job.Run())
			{
				return false;
			}

			// Converts from local space to model space matrices.
			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = m_CurrentAnimation->model->skeleton;
			ltm_job.input = make_span(locals_);
			ltm_job.output = make_span(models_);
			if (!ltm_job.Run())
			{
				return false;
			}
		}

		return true;
	}

	void PlayAnimation(Animation* pAnimation)
	{
		if (m_CurrentAnimation != pAnimation)
		{
			m_CurrentAnimation = pAnimation;
			m_CurrentTime = 0.0f;
		}
	}

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)//+
	{
		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;

		Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

		if (Bone)
		{
			Bone->Update(m_CurrentTime);
			nodeTransform = Bone->GetLocalTransform();
		}

		glm::mat4 globalTransformation = parentTransform * nodeTransform;

		auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			int index = boneInfoMap[nodeName].id;
			glm::mat4 offset = boneInfoMap[nodeName].offset;
			m_FinalBoneMatrices[index] = globalTransformation * offset;
		}

		for (int i = 0; i < node->childrenCount; i++)
			CalculateBoneTransform(&node->children[i], globalTransformation);
	}

	const std::vector<glm::mat4>& GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}

};
