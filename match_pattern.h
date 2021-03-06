#pragma once
#include <array>
#include <optional>
#include <tuple>
#include "tetris_stuff.h"
#include "ai.h"


struct match_pattern {
	int width = 0;
	int height = 0;
	std::array<std::array<int8_t, 5>, 5> pattern;//0 empty, 1 there,2 doesn't matter
	std::tuple<int, int, int> outcome;

	std::optional<std::tuple<int, int, int>> match(const tetris_board& board, int start_x, int end_x, int start_y, int end_y) const noexcept {
		for (int x = start_x; x < end_x - width; ++x) {
			for (int y = start_y; y < end_y - height; ++y) {
				for (int j = 0; j < height; ++j) {
					for (int i = 0; i < width; ++i) {
						//
						if ((pattern[j][i] == 0 && board.minos[x + i][y + j] != tetris_block::empty) ||
							(pattern[j][i] == 1 && board.minos[x + i][y + j] == tetris_block::empty)) {
							goto break_inner;
						}
					}
				}
				goto wat;
			break_inner:;
				continue;
			wat:;
				const auto [x_s, y_s, o] = outcome;
				return std::tuple(x_s + x, y_s + y, o);
			}
		}

		return std::nullopt;
	}

};

constexpr match_pattern create_match_pattern(int w, int h, std::array<std::array<int8_t, 5>, 5> pattern, std::tuple<int, int, int> o) {
	std::reverse(pattern.begin(), pattern.begin() + h);
	return {w, h, pattern, o};
}

const auto tspin1 = create_match_pattern(
	3, 3,
	{
		{
			{{0, 0, 1}},
			{{0, 0, 0}},
			{{1, 0, 1}},
		}
	},
	{1, 1, 2}
);

const auto tspin2 = create_match_pattern(
	3, 3,
	{
		{
			{{1, 0, 0}},
			{{0, 0, 0}},
			{{1, 0, 1}},
		}
	},
	{1, 1, 2}
);

static inline const std::array<match_pattern, 2> tspindouble_matches = {tspin1, tspin2};

struct anti_match_pattern {
	int width = 0;
	int height = 0;
	std::array<std::array<int8_t, 10>, 10> pattern;//0 empty, 1 there,2 doesn't matter
	std::vector<piece_hard_drop_spot> outcomes;

	std::vector<piece_hard_drop_spot> match(const tetris_board& board, int start_x, int end_x, int start_y, int end_y, std::array<int8_t, 10> heights) const noexcept {
		std::vector<piece_hard_drop_spot> ret;
		for (int x = start_x; x < end_x - width; ++x) {
			for (int y = heights[x] - height; y >= 0 && y < end_y - height; ++y) {
				for (int j = 0; j < height; ++j) {
					for (int i = 0; i < width; ++i) {
						if ((pattern[j][i] == 0 && board.minos[x + i][y + j] != tetris_block::empty) ||
							(pattern[j][i] == 1 && board.minos[x + i][y + j] == tetris_block::empty)) {
							goto break_inner;
						}
					}
				}
				for (const auto& o : outcomes) {
					ret.emplace_back(o.x + x, o.orientation);
				}
			break_inner:;
			}
		}

		return ret;
	}
};

inline anti_match_pattern create_anti_match_pattern(int w, int h, std::array<std::array<int8_t, 10>, 10> pattern, std::vector<piece_hard_drop_spot> o) {
	std::reverse(pattern.begin(), pattern.begin() + h);
	return {w, h, pattern, std::move(o)};
}

static const inline std::vector<anti_match_pattern> bad_L_spots = {
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{0, 0, 1}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 1}, {1, 0}, {1, 2}}
	),
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{1, 0, 0}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 3}, {1, 0}, {1, 2}}
	),

	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{0, 0, 1}},
				{{1, 0, 1}},
				{{2, 1, 1}},
			}
		},
		{{2, 2}}
	),

};

static const inline std::vector<anti_match_pattern> bad_O_spots = {
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{0, 0, 1}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 0}, {0, 0}}
	),
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{1, 0, 0}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{0, 0}, {1, 0}}
	),
	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{0, 0, 0}},
				{{0, 0, 1}},
				{{1, 1, 2}},
			}
		},
		{{1, 0}}
	),
	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{0, 0, 0}},
				{{1, 0, 0}},
				{{2, 1, 1}},
			}
		},
		{{0, 0}}
	),

};
static const inline std::vector<anti_match_pattern> bad_I_spots = {
	create_anti_match_pattern(
		2, 7,
		{
			{
				{{0, 0 }},
				{{2, 0 }},
				{{1, 0 }},
				{{2, 1 }},
				{{2, 0 }},
				{{2, 0 }},
				{{2, 0 }},
			}
		},
		{{1, 1}}
	),
};

static const inline std::vector<anti_match_pattern> bad_J_spots = {
	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{0, 0, 0}},
				{{0, 0, 1}},
				{{1, 1, 1}},
			}
		},
		{{2, 3}, {1, 0}}
	),
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{0, 0, 1}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 1}, {1, 0}, {1, 2}}
	),
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{1, 0, 0}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 3}, {1, 0}, {1, 2}}
	),
	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{1, 0, 0}},
				{{1, 0, 1}},
				{{1, 1, 2}},
			}
		},
		{{0, 2}}
	),
};


static const inline std::vector<anti_match_pattern> bad_T_spots = {
	create_anti_match_pattern(
		3, 2,
		{
			{
				{{0, 0, 0}},
				{{1, 1, 1}},
			}
		},
		//{{0, 1}, {2, 3}}
		{}
	),
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{0, 0, 1}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 3}}
	),
	create_anti_match_pattern(
		3, 3,
		{
			{
				{{1, 0, 0}},
				{{0, 0, 0}},
				{{1, 0, 1}},
			}
		},
		{{1, 1}}
	),
	create_anti_match_pattern(
		4, 3,
		{
			{
				{{0,0, 0, 0}},
				{{0,1, 1, 0}},
				{{0,0, 1, 1}},
			}
		},
		{{0, 1}}
	),
	create_anti_match_pattern(
		4, 3,
		{
			{
				{{0,0, 0, 0}},
				{{0,1, 1, 0}},
				{{1,1, 0, 0}},
			}
		},
		{{3, 3}}
	),

};


static const inline std::vector<anti_match_pattern> bad_S_spots = {
	create_anti_match_pattern(
		3, 2,
		{
			{
				{{0, 0, 0}},
				{{1, 1, 1}},
			}
		},
		//{{0, 1}, {2, 3}}
		{{0, 1}, {2, 3}}
	),
	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{0, 0, 0}},
				{{1, 0, 0}},
				{{2, 1, 1}},
			}
		},
		{{2, 1}, {1, 0}}
	),
};
static const inline std::vector<anti_match_pattern> bad_Z_spots = {
	create_anti_match_pattern(
		3, 2,
		{
			{
				{{0,0,0}},
				{{1, 1, 1}},
			}
		},
		//{{0, 1}, {2, 3}}
		{}
	),
	create_anti_match_pattern(
		3, 4,
		{
			{
				{{0, 0, 0}},
				{{0, 0, 0}},
				{{0, 0, 1}},
				{{1, 1, 2}},
			}
		},
		{{1, 0}, {0, 1}}
	),
};
