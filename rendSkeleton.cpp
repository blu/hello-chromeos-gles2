#include <stdint.h>
#include <stdio.h>
#include "cmath_fix"
#include <iostream>
#include <iomanip>

#include "scoped.hpp"
#include "vectsimd.hpp"
#include "rendSkeleton.hpp"

using util::scoped_ptr;
using util::scoped_functor;

namespace util {

template <>
class scoped_functor< FILE >
{
public:
	void operator()(FILE* arg)
	{
		fclose(arg);
	}
};

} // namespace util

namespace rend
{

using namespace simd;

void
initBoneMatx(
	const unsigned count,
	dense_matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx)
{
	assert(256 > count);
	assert(bone_mat);
	assert(bone);
	assert(bone_idx < count);

	if (bone[bone_idx].matx_valid)
		return;

	const vect3 pos = bone[bone_idx].position;
	const quat  ori = bone[bone_idx].orientation;
	const vect3 sca = bone[bone_idx].scale;

	const matx4 orientation(ori);
	bone[bone_idx].to_model.set(0, vect4().mul(orientation[0], sca[0]));
	bone[bone_idx].to_model.set(1, vect4().mul(orientation[1], sca[1]));
	bone[bone_idx].to_model.set(2, vect4().mul(orientation[2], sca[2]));
	bone[bone_idx].to_model.set(3, vect4(
		pos[0],
		pos[1],
		pos[2],
		1.f));

	const unsigned parent_idx = bone[bone_idx].parent_idx;

	if (255 != parent_idx) {
		assert(parent_idx < bone_idx);
		assert(bone[parent_idx].matx_valid);

		bone[bone_idx].to_model.mulr(bone[parent_idx].to_model);
	}

	bone[bone_idx].to_local.inverse(bone[bone_idx].to_model);
	bone[bone_idx].matx_valid = true;

	bone_mat[bone_idx] = dense_matx4(
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f);
}


void
updateBoneMatx(
	const unsigned count,
	dense_matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx)
{
	assert(256 > count);
	assert(bone_mat);
	assert(bone);
	assert(bone_idx < count);

	if (bone[bone_idx].matx_valid)
		return;

	const vect3 pos = bone[bone_idx].position;
	const quat  ori = bone[bone_idx].orientation;
	const vect3 sca = bone[bone_idx].scale;

	const matx4 orientation(ori);
	bone[bone_idx].to_model.set(0, vect4().mul(orientation[0], sca[0]));
	bone[bone_idx].to_model.set(1, vect4().mul(orientation[1], sca[1]));
	bone[bone_idx].to_model.set(2, vect4().mul(orientation[2], sca[2]));
	bone[bone_idx].to_model.set(3, vect4(
		pos[0],
		pos[1],
		pos[2],
		1.f));

	const unsigned parent_idx = bone[bone_idx].parent_idx;

	if (255 != parent_idx) {
		assert(parent_idx < bone_idx);
		assert(bone[parent_idx].matx_valid);

		bone[bone_idx].to_model.mulr(bone[parent_idx].to_model);
	}

	bone[bone_idx].matx_valid = true;

	const matx4 b = matx4().mul(bone[bone_idx].to_local, bone[bone_idx].to_model);

	bone_mat[bone_idx] = dense_matx4(
		b[0][0], b[0][1], b[0][2], b[0][3],
		b[1][0], b[1][1], b[1][2], b[1][3],
		b[2][0], b[2][1], b[2][2], b[2][3],
		b[3][0], b[3][1], b[3][2], b[3][3]);
}


void
updateRoot(
	Bone* root)
{
	assert(root);

	if (root->matx_valid)
		return;

	const matx4 orientation(root->orientation);
	root->to_model.set(0, vect4().mul(orientation[0], root->scale[0]));
	root->to_model.set(1, vect4().mul(orientation[1], root->scale[1]));
	root->to_model.set(2, vect4().mul(orientation[2], root->scale[2]));
	root->to_model.set(3, vect4(
		root->position[0],
		root->position[1],
		root->position[2],
		1.f));

	root->matx_valid = true;
}


void
invalidateBoneMatx(
	const unsigned bone_count,
	Bone* bone,
	const unsigned bone_idx)
{
	assert(256 > bone_count);
	assert(bone);
	assert(bone_idx < bone_count);

	// unless already invalid, a bone is as valid as its parent
	if (!bone[bone_idx].matx_valid)
		return;

	const unsigned parent_idx = bone[bone_idx].parent_idx;

	if (255 != parent_idx) {
		assert(parent_idx < bone_idx);

		bone[bone_idx].matx_valid = bone[parent_idx].matx_valid;
	}
}


void
animateSkeleton(
	const unsigned count,
	dense_matx4* bone_mat,
	Bone* bone,
	const std::vector< Track >& skeletal_animation,
	const float anim_time,
	Bone* root)
{
	assert(256 > count);
	assert(bone_mat);
	assert(bone);

	if (skeletal_animation.empty())
		return;

	bool updates = false;

	for (std::vector< Track >::const_iterator it = skeletal_animation.begin(); it != skeletal_animation.end(); ++it) {

		if (0 == root && 255 == it->bone_idx)
			continue;

		Bone* const bone_alias = bone;
		Bone& bone = 255 == it->bone_idx
			? *root
			: bone_alias[it->bone_idx];

		Track::BonePositionKeys::const_iterator    jt0 = it->position_key.begin()    + it->position_last_key_idx;
		Track::BoneOrientationKeys::const_iterator jt1 = it->orientation_key.begin() + it->orientation_last_key_idx;
		Track::BoneScaleKeys::const_iterator       jt2 = it->scale_key.begin()       + it->scale_last_key_idx;

		for (Track::BonePositionKeys::const_iterator kt = it->position_key.end(); jt0 != it->position_key.end(); ++jt0) {
			if (jt0->time < anim_time) {
				kt = jt0;
				it->position_last_key_idx = jt0 - it->position_key.begin();
				continue;
			}

			if (jt0->time > anim_time) {
				if (kt == it->position_key.end()) {
					jt0 = kt;
					break;
				}

				const float w1 = (anim_time - kt->time) / (jt0->time - kt->time);
				const float w0 = 1.f - w1;

				bone.position.wsum(kt->value, jt0->value, w0, w1);
			}
			else
				bone.position = jt0->value;

			bone.matx_valid = false;
			updates = true;
			break;
		}

		for (Track::BoneOrientationKeys::const_iterator kt = it->orientation_key.end(); jt1 != it->orientation_key.end(); ++jt1) {
			if (jt1->time < anim_time) {
				kt = jt1;
				it->orientation_last_key_idx = jt1 - it->orientation_key.begin();
				continue;
			}

			if (jt1->time > anim_time) {
				if (kt == it->orientation_key.end()) {
					jt1 = kt;
					break;
				}

				const float w1 = (anim_time - kt->time) / (jt1->time - kt->time);
				const float w0 = 0.f > kt->value.dot(jt1->value) ? w1 - 1.f : 1.f - w1;

				bone.orientation.wsum(kt->value, jt1->value, w0, w1);
				bone.orientation.normalise();
			}
			else
				bone.orientation = jt1->value;

			bone.matx_valid = false;
			updates = true;
			break;
		}

		for (Track::BoneScaleKeys::const_iterator kt = it->scale_key.end(); jt2 != it->scale_key.end(); ++jt2) {
			if (jt2->time < anim_time) {
				kt = jt2;
				it->scale_last_key_idx = jt2 - it->scale_key.begin();
				continue;
			}

			if (jt2->time > anim_time) {
				if (kt == it->scale_key.end()) {
					jt2 = kt;
					break;
				}

				const float w1 = (anim_time - kt->time) / (jt2->time - kt->time);
				const float w0 = 1.f - w1;

				bone.scale.wsum(kt->value, jt2->value, w0, w1);
			}
			else
				bone.scale = jt2->value;

			bone.matx_valid = false;
			updates = true;
			break;
		}
	}

	if (updates) {
		for (unsigned i = 0; i < count; ++i)
			invalidateBoneMatx(count, bone, i);

		for (unsigned i = 0; i < count; ++i)
			updateBoneMatx(count, bone_mat, bone, i);

		if (0 != root)
			updateRoot(root);
	}
}


void
resetSkeletonAnimProgress(
	const std::vector< Track >& skeletal_animation)
{
	for (std::vector< Track >::const_iterator it = skeletal_animation.begin(); it != skeletal_animation.end(); ++it) {
		it->position_last_key_idx = 0;
		it->orientation_last_key_idx = 0;
		it->scale_last_key_idx = 0;
	}
}


bool
loadSkeletonAnimationABE(
	const char* const filename,
	unsigned* count,
	dense_matx4* bone_mat,
	Bone* bone,
	std::vector< std::vector< Track > >& animations,
	std::vector< float >& durations)
{
	assert(filename);
	assert(count);
	assert(bone_mat);
	assert(bone);

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file()) {
		std::cerr << __FUNCTION__ << " failed to open " << filename << std::endl;
		return false;
	}

	uint32_t magic;
	if (1 != fread(&magic, sizeof(magic), 1, file()))
		return false;

	uint32_t version;
	if (1 != fread(&version, sizeof(version), 1, file()))
		return false;

	std::cout << "skeleton magic, version: 0x" << std::hex << std::setw(8) << std::setfill('0') <<
		magic << std::dec << ", " << version << std::endl;

	static const uint32_t sMagic = 0x6c656b53;
	static const uint32_t sVersion = 100;

	if (magic != sMagic || version != sVersion)
		return false;

	uint16_t n_bones;
	if (1 != fread(&n_bones, sizeof(n_bones), 1, file()))
		return false;

	if (*count < n_bones) {
		std::cerr << __FUNCTION__ << " failed to load a skeleton with too many bones" << std::endl;
		return false;
	}

	for (uint16_t i = 0; i < n_bones; ++i) {
		float position[3];
		if (1 != fread(&position, sizeof(position), 1, file()))
			return false;

		float ori_swiz[4];
		if (1 != fread(&ori_swiz, sizeof(ori_swiz), 1, file()))
			return false;

		float scale[3];
		if (1 != fread(&scale, sizeof(scale), 1, file()))
			return false;

		uint8_t parent;
		if (1 != fread(&parent, sizeof(parent), 1, file()))
			return false;

		uint16_t name_len;
		char name[1024];

		if (1 != fread(&name_len, sizeof(name_len), 1, file()) || name_len >= sizeof(name))
			return false;

		if (1 != fread(name, sizeof(name[0]) * name_len, 1, file()))
			return false;

		name[name_len] = '\0';

		bone[i].position = vect3(
			position[0],
			position[1],
			position[2]);
		bone[i].orientation = quat(
			ori_swiz[1],
			ori_swiz[2],
			ori_swiz[3],
			ori_swiz[0]).conj(); // reverse rotation (conjugate)
		bone[i].scale = vect3(
			scale[0],
			scale[1],
			scale[2]);
		bone[i].parent_idx = parent;
		bone[i].name = name;

#if VERBOSE_READ
		std::cout <<
			"bone " << i << ": " << name <<
			"\n\tpos: " <<
			bone[i].position[0] << ", " <<
			bone[i].position[1] << ", " <<
			bone[i].position[2] <<
			"\n\tori: " <<
			bone[i].orientation[0] << ", " <<
			bone[i].orientation[1] << ", " <<
			bone[i].orientation[2] << ", " <<
			bone[i].orientation[3] <<
			"\n\tsca: " <<
			bone[i].scale[0] << ", " <<
			bone[i].scale[1] << ", " <<
			bone[i].scale[2] <<
			"\n\tpar: " <<
			unsigned(bone[i].parent_idx) << std::endl;

#endif
	}

	uint16_t n_anims;
	if (1 != fread(&n_anims, sizeof(n_anims), 1, file()))
		return false;

#if VERBOSE_READ
	std::cout << "animations: " << n_anims << std::endl;

#endif
	for (uint16_t i = 0; i < n_anims; ++i) {
		uint16_t name_len;
		char name[1024];

		if (1 != fread(&name_len, sizeof(name_len), 1, file()) || name_len >= sizeof(name))
			return false;

		if (1 != fread(name, sizeof(name[0]) * name_len, 1, file()))
			return false;

		name[name_len] = '\0';

		uint32_t duration[2];
		if (1 != fread(duration, sizeof(duration), 1, file()))
			return false;

		if (0 == duration[1])
			std::cout << "warning: animation '" << name << "' has zero duration\n";

		const float scaledDuration = (1.f / 512.f) * duration[1];
		durations.push_back(scaledDuration);

		uint16_t n_tracks;
		if (1 != fread(&n_tracks, sizeof(n_tracks), 1, file()))
			return false;

#if VERBOSE_READ
		std::cout << "animation, tracks: " << name << ", " << unsigned(n_tracks) << std::endl;

#endif
		std::vector< Track >& skeletal_animation = *animations.insert(animations.end(), std::vector< Track >());

		for (uint16_t i = 0; i < n_tracks; ++i) {
			uint8_t bone_idx_and_stuff[2];
			if (1 != fread(bone_idx_and_stuff, sizeof(bone_idx_and_stuff), 1, file()))
				return false;

			assert(!bone_idx_and_stuff[1]);

#if VERBOSE_READ
			std::cout << "track, bone: " << unsigned(i) << ", " << unsigned(bone_idx_and_stuff[0]) << std::endl;

#endif
			Track& track = *skeletal_animation.insert(skeletal_animation.end(), Track());

			track.bone_idx = bone_idx_and_stuff[0];

			uint32_t n_pos_keys;
			if (1 != fread(&n_pos_keys, sizeof(n_pos_keys), 1, file()))
				return false;

#if VERBOSE_READ
			std::cout << "\tposition keys: " << n_pos_keys << std::endl;

#endif
			for (uint32_t i = 0; i < n_pos_keys; ++i) {
				float time;
				if (1 != fread(&time, sizeof(time), 1, file()))
					return false;

				float position[3];
				if (1 != fread(&position, sizeof(position), 1, file()))
					return false;

				BonePositionKey& pos = *track.position_key.insert(track.position_key.end(), BonePositionKey());
				pos.time = time * scaledDuration;
				pos.value = vect3(
					position[0],
					position[1],
					position[2]);
			}

			uint32_t n_ori_keys;
			if (1 != fread(&n_ori_keys, sizeof(n_ori_keys), 1, file()))
				return false;

#if VERBOSE_READ
			std::cout << "\torientation keys: " << n_ori_keys << std::endl;

#endif
			for (uint32_t i = 0; i < n_ori_keys; ++i) {
				float time;
				if (1 != fread(&time, sizeof(time), 1, file()))
					return false;

				float ori_swiz[4];
				if (1 != fread(&ori_swiz, sizeof(ori_swiz), 1, file()))
					return false;

				BoneOrientationKey& ori = *track.orientation_key.insert(track.orientation_key.end(), BoneOrientationKey());
				ori.time = time * scaledDuration;
				ori.value = quat(
					ori_swiz[1],
					ori_swiz[2],
					ori_swiz[3],
					ori_swiz[0]).conj(); // reverse rotation (conjugate)
			}

			uint32_t n_sca_keys;
			if (1 != fread(&n_sca_keys, sizeof(n_sca_keys), 1, file()))
				return false;

#if VERBOSE_READ
			std::cout << "\tscale keys: " << n_sca_keys << std::endl;

#endif
			for (uint32_t i = 0; i < n_sca_keys; ++i) {
				float time;
				if (1 != fread(&time, sizeof(time), 1, file()))
					return false;

				float scale[3];
				if (1 != fread(&scale, sizeof(scale), 1, file()))
					return false;

				BoneScaleKey& sca = *track.scale_key.insert(track.scale_key.end(), BoneScaleKey());
				sca.time = time * scaledDuration;
				sca.value = vect3(
					scale[0],
					scale[1],
					scale[2]);
			}

			track.position_last_key_idx = 0;
			track.orientation_last_key_idx = 0;
			track.scale_last_key_idx = 0;
		}
	}

	*count = n_bones;

	for (uint16_t i = 0; i < n_bones; ++i)
		initBoneMatx(unsigned(n_bones), bone_mat, bone, unsigned(i));

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// OgreAnimation deserializer from Serializer_v1.10

static bool loadBoneOgre(
	FILE* const file,
	Bone& bone,
	const unsigned boneCount)
{
	uint16_t bone_chunk_id;

	if (1 != fread(&bone_chunk_id, sizeof(bone_chunk_id), 1, file) || 0x2000 != bone_chunk_id) {
		fprintf(stderr, "%s encountered non-bone\n", __FUNCTION__);
		return false;
	}

	uint32_t bone_chunk_size;
	if (1 != fread(&bone_chunk_size, sizeof(bone_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	size_t name_len = 0;
	char name[1024];
	while (name_len < sizeof(name) / sizeof(name[0])) {
		if (1 != fread(name + name_len, sizeof(name[0]), 1, file)) {
			fprintf(stderr, "%s could not read name\n", __FUNCTION__);
			return false;
		}
		if (name[name_len] == 0xa) {
			name[name_len] = '\0';
			break;
		}
		++name_len;
	}

	if (name_len == sizeof(name) / sizeof(name[0])) {
		fprintf(stderr, "%s encountered too long name\n", __FUNCTION__);
		return false;
	}

	uint16_t boneIdx;
	if (1 != fread(&boneIdx, sizeof(boneIdx), 1, file)) {
		fprintf(stderr, "%s could not read bone idx\n", __FUNCTION__);
		return false;
	}

	if (unsigned(boneIdx) != boneCount) {
		fprintf(stderr, "%s encountered bone index mismatch\n", __FUNCTION__);
		return false;
	}

	float position[3];
	if (1 != fread(position, sizeof(position), 1, file)) {
		fprintf(stderr, "%s could not read position\n", __FUNCTION__);
		return false;
	}

	float ori_swiz[4];
	if (1 != fread(ori_swiz, sizeof(ori_swiz), 1, file)) {
		fprintf(stderr, "%s could not read orientation\n", __FUNCTION__);
		return false;
	}

	bone.name = std::string(name);
	bone.position = vect3(
		position[0],
		position[1],
		position[2]);
	bone.orientation = quat(
		ori_swiz[0],
		ori_swiz[1],
		ori_swiz[2],
		ori_swiz[3]);

	// size of bone chunk does not take into account the name
	if (bone_chunk_size == sizeof(bone_chunk_id) + sizeof(bone_chunk_size) + sizeof(boneIdx) + sizeof(position) + sizeof(ori_swiz))
		return true;

	float scale[3];
	if (1 != fread(scale, sizeof(scale), 1, file)) {
		fprintf(stderr, "%s could not read scale\n", __FUNCTION__);
		return false;
	}

	bone.scale = vect3(
		scale[0],
		scale[1],
		scale[2]);

	return bone_chunk_size == sizeof(bone_chunk_id) + sizeof(bone_chunk_size) + sizeof(boneIdx) + sizeof(position) + sizeof(ori_swiz) + sizeof(scale);
}

static bool loadBoneParentOgre(
	FILE* const file,
	Bone* bone,
	const unsigned boneCount)
{
	uint16_t parent_chunk_id;

	if (1 != fread(&parent_chunk_id, sizeof(parent_chunk_id), 1, file) || 0x3000 != parent_chunk_id) {
		fprintf(stderr, "%s encountered non-bone-parent\n", __FUNCTION__);
		return false;
	}

	uint32_t parent_chunk_size;
	if (1 != fread(&parent_chunk_size, sizeof(parent_chunk_size), 1, file)) {
		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	uint16_t child;
	if (1 != fread(&child, sizeof(child), 1, file)) {
		fprintf(stderr, "%s could not read child index\n", __FUNCTION__);
		return false;
	}

	if (unsigned(child) >= boneCount) {
		fprintf(stderr, "%s encountered child bone index mismatch\n", __FUNCTION__);
		return false;
	}

	uint16_t parent;
	if (1 != fread(&parent, sizeof(parent), 1, file)) {
		fprintf(stderr, "%s could not read parent index\n", __FUNCTION__);
		return false;
	}

	if (unsigned(parent) >= boneCount) {
		fprintf(stderr, "%s encountered parent bone index mismatch\n", __FUNCTION__);
		return false;
	}

	if (parent == child) {
		fprintf(stderr, "%s encountered self-parented bone\n", __FUNCTION__);
		return false;
	}

	bone[child].parent_idx = parent;

	return parent_chunk_size == sizeof(parent_chunk_id) + sizeof(parent_chunk_size) + sizeof(child) + sizeof(parent);
}

static bool loadKeyFrameOgre(
	FILE* const file,
	uint32_t& chunk_size, // remaining size of the chunk
	Track& track)
{
	uint16_t keyframe_chunk_id;
	if (1 != fread(&keyframe_chunk_id, sizeof(keyframe_chunk_id), 1, file) || 0x4110 != keyframe_chunk_id) {
		fprintf(stderr, "%s encountered non-keyframe\n", __FUNCTION__);
		return false;
	}

	uint32_t keyframe_chunk_size;
	if (1 != fread(&keyframe_chunk_size, sizeof(keyframe_chunk_size), 1, file) ||
		sizeof(keyframe_chunk_id) + sizeof(keyframe_chunk_size) >= keyframe_chunk_size) {

		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (keyframe_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= keyframe_chunk_size;

	float time;
	if (1 != fread(&time, sizeof(time), 1, file)) {
		fprintf(stderr, "%s could not read time\n", __FUNCTION__);
		return false;
	}

	float ori_swiz[4];
	if (1 != fread(ori_swiz, sizeof(ori_swiz), 1, file)) {
		fprintf(stderr, "%s could not read orientation\n", __FUNCTION__);
		return false;
	}

	float position[3];
	if (1 != fread(position, sizeof(position), 1, file)) {
		fprintf(stderr, "%s could not read position\n", __FUNCTION__);
		return false;
	}

	BonePositionKey& pos = *track.position_key.insert(track.position_key.end(), BonePositionKey());
	pos.time = time;
	pos.value = vect3(
		position[0],
		position[1],
		position[2]);

	BoneOrientationKey& ori = *track.orientation_key.insert(track.orientation_key.end(), BoneOrientationKey());
	ori.time = time;
	ori.value = quat(
		ori_swiz[0],
		ori_swiz[1],
		ori_swiz[2],
		ori_swiz[3]);

	if (keyframe_chunk_size == sizeof(keyframe_chunk_id) + sizeof(keyframe_chunk_size) + sizeof(time) + sizeof(ori_swiz) + sizeof(position))
		return true;

	float scale[3];
	if (1 != fread(scale, sizeof(scale), 1, file)) {
		fprintf(stderr, "%s could not read scale\n", __FUNCTION__);
		return false;
	}

	BoneScaleKey& sca = *track.scale_key.insert(track.scale_key.end(), BoneScaleKey());
	sca.time = time;
	sca.value = vect3(
		scale[0],
		scale[1],
		scale[2]);

	return keyframe_chunk_size == sizeof(keyframe_chunk_id) + sizeof(keyframe_chunk_size) + sizeof(time) + sizeof(ori_swiz) + sizeof(position) + sizeof(scale);
}

static bool loadTrackOgre(
	FILE* const file,
	uint32_t& chunk_size, // remaining size of the chunk
	std::vector< Track >& tracks)
{
	uint16_t track_chunk_id;
	if (1 != fread(&track_chunk_id, sizeof(track_chunk_id), 1, file) || 0x4100 != track_chunk_id) {
		fprintf(stderr, "%s encountered non-track\n", __FUNCTION__);
		return false;
	}

	uint32_t track_chunk_size;
	if (1 != fread(&track_chunk_size, sizeof(track_chunk_size), 1, file) ||
		sizeof(track_chunk_id) + sizeof(track_chunk_size) >= track_chunk_size) {

		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	if (track_chunk_size > chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	chunk_size -= track_chunk_size;

	uint16_t bone_idx;
	if (1 != fread(&bone_idx, sizeof(bone_idx), 1, file)) {
		fprintf(stderr, "%s could not read bone index\n", __FUNCTION__);
		return false;
	}

	const uint32_t chunk_dec = sizeof(track_chunk_id) + sizeof(track_chunk_size) + sizeof(bone_idx);
	if (chunk_dec > track_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	track_chunk_size -= chunk_dec;
	Track& track = *tracks.insert(tracks.end(), Track());

	while (track_chunk_size) {
		if (!loadKeyFrameOgre(file, track_chunk_size, track))
			return false;
	}

	track.bone_idx = bone_idx;
	track.position_last_key_idx = 0;
	track.orientation_last_key_idx = 0;
	track.scale_last_key_idx = 0;

#if VERBOSE_READ
	std::cout << "track, bone: " << unsigned(tracks.size() - 1) << ", " << unsigned(bone_idx) << std::endl;
	const Track& t = tracks.back();
	std::cout << "\tposition keys: " << t.position_key.size() << std::endl;
	std::cout << "\torientation keys: " << t.orientation_key.size() << std::endl;
	std::cout << "\tscale keys: " << t.scale_key.size() << std::endl;

#endif
	return true;
}

static bool loadAnimationOgre(
	FILE* const file,
	std::vector< std::vector< Track > >& animations,
	std::vector< float >& durations)
{
	uint16_t anim_chunk_id;

	if (1 != fread(&anim_chunk_id, sizeof(anim_chunk_id), 1, file) || 0x4000 != anim_chunk_id) {
		fprintf(stderr, "%s encountered non-animation\n", __FUNCTION__);
		return false;
	}

	uint32_t anim_chunk_size;
	if (1 != fread(&anim_chunk_size, sizeof(anim_chunk_size), 1, file) ||
		sizeof(anim_chunk_id) + sizeof(anim_chunk_size) >= anim_chunk_size) {

		fprintf(stderr, "%s could not read chunk size\n", __FUNCTION__);
		return false;
	}

	size_t name_len = 0;
	char name[1024];
	while (name_len < sizeof(name) / sizeof(name[0])) {
		if (1 != fread(name + name_len, sizeof(name[0]), 1, file)) {
			fprintf(stderr, "%s could not read name\n", __FUNCTION__);
			return false;
		}
		if (name[name_len] == 0xa) {
			name[name_len] = '\0';
			break;
		}
		++name_len;
	}

	if (name_len == sizeof(name) / sizeof(name[0])) {
		fprintf(stderr, "%s encountered too long name\n", __FUNCTION__);
		return false;
	}

	float duration;
	if (1 != fread(&duration, sizeof(duration), 1, file)) {
		fprintf(stderr, "%s could not read duration\n", __FUNCTION__);
		return false;
	}

	if (0.f == duration)
		fprintf(stdout, "warning: animation '%s' has zero duration\n", name);

	durations.push_back(duration);

#if VERBOSE_READ
	fprintf(stderr, "animation \"%s\" of duration %f\n", name, duration);

#endif
	const uint32_t chunk_dec = sizeof(anim_chunk_id) + sizeof(anim_chunk_size) + name_len + 1 + sizeof(duration);
	if (chunk_dec > anim_chunk_size) {
		fprintf(stderr, "%s chunk size mismatch\n", __FUNCTION__);
		return false;
	}

	anim_chunk_size -= chunk_dec;
	std::vector< Track >& skeletal_animation = *animations.insert(animations.end(), std::vector< Track >());

	while (anim_chunk_size) {
		if (!loadTrackOgre(file, anim_chunk_size, skeletal_animation))
			return false;
	}

	return true;
}


static void repaBone(
	Bone* const bone,
	const size_t count,
	const uint8_t index0,
	const uint8_t index1)
{
	for (size_t i = 0; i < count; ++i) {
		if (bone[i].parent_idx == index0)
			bone[i].parent_idx = index1;
		else
		if (bone[i].parent_idx == index1)
			bone[i].parent_idx = index0;
	}
}


static void repaTrack(
	std::vector< std::vector< Track > >& animations,
	const uint8_t index0,
	const uint8_t index1)
{
	for (std::vector< std::vector< Track > >::iterator it = animations.begin(); it != animations.end(); ++it) { 
		const std::vector< Track >::iterator it_end = it->end();
		for (std::vector< Track >::iterator jt = it->begin(); jt != it_end; ++jt) {
			if (jt->bone_idx == index0)
				jt->bone_idx = index1;
			else
			if (jt->bone_idx == index1)
				jt->bone_idx = index0;
		}
	}
}


bool
loadSkeletonAnimationOgre(
	const char* const filename,
	unsigned* count,
	dense_matx4* bone_mat,
	Bone* bone,
	std::vector< std::vector< Track > >& animations,
	std::vector< float >& durations)
{
	assert(filename);
	assert(count);
	assert(bone_mat);
	assert(bone);

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file()) {
		fprintf(stderr, "%s failed at fopen\n", __FUNCTION__);
		return false;
	}

	const unsigned boneCapacity = *count;
	unsigned boneCount = 0;
	bool done = false;

	while (!done) {
		uint16_t id;
		if (1 != fread(&id, sizeof(id), 1, file())) {
			if (!feof(file())) {
				fprintf(stderr, "%s failed to read chunk id\n", __FUNCTION__);
				return false;
			}
			fprintf(stderr, "%s done\n", __FUNCTION__);
			break;
		}

		switch (id) {
		case 0x2000: // SKELETON_BONE
			if (boneCount == boneCapacity) {
				fprintf(stderr, "%s encountered too many bones\n", __FUNCTION__);
				return false;
			}
			bone[boneCount] = Bone();
			if (!loadBoneOgre(file(), bone[boneCount], boneCount))
				return false;
			++boneCount;
			break;

		case 0x3000: // SKELETON_BONE_PARENT
			if (!loadBoneParentOgre(file(), bone, boneCount))
				return false;
			break;

		case 0x4000: // SKELETON_ANIMATION
			if (!loadAnimationOgre(file(), animations, durations))
				return false;
			break;

		default:
			fprintf(stderr, "%s encountered unhandled chunk id 0x%04hx; done\n", __FUNCTION__, id);
			done = true;
			break;
		}
	}

	// Ogre keyframes are cumulative to bone's binding pose - correct that
	for (std::vector< std::vector< Track > >::iterator it = animations.begin(); it != animations.end(); ++it) { 
		const std::vector< Track >::iterator it_end = it->end();

		for (std::vector< Track >::iterator jt = it->begin(); jt != it_end; ++jt) {
			Track& track = *jt;
			const Bone& track_bone = bone[track.bone_idx];

			for (Track::BonePositionKeys::iterator pt = track.position_key.begin(); pt != track.position_key.end(); ++pt)
				pt->value.add(track_bone.position);

			for (Track::BoneOrientationKeys::iterator ot = track.orientation_key.begin(); ot != track.orientation_key.end(); ++ot)
				ot->value.qmull(track_bone.orientation);

			for (Track::BoneScaleKeys::iterator st = track.scale_key.begin(); st != track.scale_key.end(); ++st)
				st->value.mul(track_bone.scale);
		}
	}

	// fix up mis-ordered skeleton trees
	bool rescan;
	do {
		rescan = false;
		for (size_t i = 0; i < boneCount; ++i) {
			const int8_t parent_i = bone[i].parent_idx;

			if (parent_i > int8_t(i)) {
				const size_t j = uint8_t(parent_i);
				const Bone bone_i(bone[i]);
				bone[i] = bone[j];
				bone[j] = bone_i;

				repaBone(bone, boneCount, parent_i, int8_t(i));
				repaTrack(animations, parent_i, int8_t(i));
				rescan = true;
			}
		}
	}
	while (rescan);

	*count = boneCount;

	for (unsigned i = 0; i < boneCount; ++i) {

#if VERBOSE_READ
		std::cout <<
			"bone " << i << ": " << bone[i].name <<
			"\n\tpos: " <<
			bone[i].position[0] << ", " <<
			bone[i].position[1] << ", " <<
			bone[i].position[2] <<
			"\n\tori: " <<
			bone[i].orientation[0] << ", " <<
			bone[i].orientation[1] << ", " <<
			bone[i].orientation[2] << ", " <<
			bone[i].orientation[3] <<
			"\n\tsca: " <<
			bone[i].scale[0] << ", " <<
			bone[i].scale[1] << ", " <<
			bone[i].scale[2] <<
			"\n\tpar: " <<
			unsigned(bone[i].parent_idx) << std::endl;

#endif
		initBoneMatx(boneCount, bone_mat, bone, i);
	}

	return true;
}

} // namespace rend
