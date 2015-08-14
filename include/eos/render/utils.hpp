/*
 * Eos - A 3D Morphable Model fitting library written in modern C++11/14.
 *
 * File: include/eos/render/utils.hpp
 *
 * Copyright 2014, 2015 Patrik Huber
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#ifndef RENDER_UTILS_HPP_
#define RENDER_UTILS_HPP_

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace eos {
	namespace render {

/**
 * Transforms a point from clip space ([-1, 1] x [-1, 1]) to
 * image (screen) coordinates, i.e. the window transform.
 * Note that the y-coordinate is flipped because the image origin
 * is top-left while in clip space top is +1 and bottom is -1.
 * No z-division is performed.
 * Note: It should rather be called from NDC to screen space?
 *
 * Exactly conforming to the OpenGL viewport transform, except that
 * we flip y at the end.
 * Qt: Origin top-left. OpenGL: bottom-left. OCV: top-left.
 *
 * @param[in] clip_coordinates A point in clip coordinates.
 * @param[in] screen_width Width of the screen or window.
 * @param[in] screen_height Height of the screen or window.
 * @return A vector with x and y coordinates transformed to screen space.
 */
inline cv::Vec2f clip_to_screen_space(const cv::Vec2f& clip_coordinates, int screen_width, int screen_height)
{
	// Window transform:
	const float x_ss = (clip_coordinates[0] + 1.0f) * (screen_width / 2.0f);
	const float y_ss = screen_height - (clip_coordinates[1] + 1.0f) * (screen_height / 2.0f); // also flip y; Qt: Origin top-left. OpenGL: bottom-left.
	return cv::Vec2f(x_ss, y_ss);
	/* Note: What we do here is equivalent to
	   x_w = (x *  vW/2) + vW/2;
	   However, Shirley says we should do:
	   x_w = (x *  vW/2) + (vW-1)/2;
	   (analogous for y)
	   Todo: Check the consequences.
	*/
};

/**
 * Transforms a point from image (screen) coordinates to
 * clip space ([-1, 1] x [-1, 1]).
 * Note that the y-coordinate is flipped because the image origin
 * is top-left while in clip space top is +1 and bottom is -1.
 *
 * @param[in] screen_coordinates A point in screen coordinates.
 * @param[in] screen_width Width of the screen or window.
 * @param[in] screen_height Height of the screen or window.
 * @return A vector with x and y coordinates transformed to clip space.
 */
inline cv::Vec2f screen_to_clip_space(const cv::Vec2f& screen_coordinates, int screen_width, int screen_height)
{
	const float x_cs = screen_coordinates[0] / (screen_width / 2.0f) - 1.0f;
	float y_cs = screen_coordinates[1] / (screen_height / 2.0f) - 1.0f;
	y_cs *= -1.0f;
	return cv::Vec2f(x_cs, y_cs);
};

// TODO: Should go to detail:: namespace, or texturing/utils or whatever.
unsigned int get_max_possible_mipmaps_num(unsigned int width, unsigned int height)
{
	unsigned int mipmapsNum = 1;
	unsigned int size = std::max(width, height);

	if (size == 1)
		return 1;

	do {
		size >>= 1;
		mipmapsNum++;
	} while (size != 1);

	return mipmapsNum;
};

inline bool is_power_of_two(int x)
{
	return !(x & (x - 1));
};

class Texture
{
public:
	// Todo: This whole class needs a major overhaul and documentation.

	// throws: ocv exc,  runtime_ex
	void create_mipmapped_texture(cv::Mat image, unsigned int mipmapsNum = 0) {

		this->mipmaps_num = (mipmapsNum == 0 ? get_max_possible_mipmaps_num(image.cols, image.rows) : mipmapsNum);
		/*if (mipmapsNum == 0)
		{
		uchar mmn = render::utils::MatrixUtils::getMaxPossibleMipmapsNum(image.cols, image.rows);
		this->mipmapsNum = mmn;
		} else
		{
		this->mipmapsNum = mipmapsNum;
		}*/

		if (this->mipmaps_num > 1)
		{
			if (!is_power_of_two(image.cols) || !is_power_of_two(image.rows))
			{
				throw std::runtime_error("Error: Couldn't generate mipmaps, width or height not power of two.");
			}
		}
		image.convertTo(image, CV_8UC4);	// Most often, the input img is CV_8UC3. Img is BGR. Add an alpha channel
		cv::cvtColor(image, image, CV_BGR2BGRA);

		int currWidth = image.cols;
		int currHeight = image.rows;
		for (int i = 0; i < this->mipmaps_num; i++)
		{
			if (i == 0) {
				mipmaps.push_back(image);
			}
			else {
				cv::Mat currMipMap(currHeight, currWidth, CV_8UC4);
				cv::resize(mipmaps[i - 1], currMipMap, currMipMap.size());
				mipmaps.push_back(currMipMap);
			}

			if (currWidth > 1)
				currWidth >>= 1;
			if (currHeight > 1)
				currHeight >>= 1;
		}
		this->widthLog = (uchar)(std::log(mipmaps[0].cols) / CV_LOG2 + 0.0001f); // std::epsilon or something? or why 0.0001f here?
		this->heightLog = (uchar)(std::log(mipmaps[0].rows) / CV_LOG2 + 0.0001f); // Changed std::logf to std::log because it doesnt compile in linux (gcc 4.8). CHECK THAT
	};

	std::vector<cv::Mat> mipmaps;	// make Texture a friend class of renderer, then move this to private?
	unsigned char widthLog, heightLog; // log2 of width and height of the base mip-level

private:
	//std::string filename;
	unsigned int mipmaps_num;
};
	} /* namespace render */
} /* namespace eos */

#endif /* RENDER_UTILS_HPP_ */
