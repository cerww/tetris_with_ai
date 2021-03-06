#pragma once
#include "start_screen.h"
#include "game_data.h"

struct dead_solo_state {
	explicit dead_solo_state(all_game_data& g,tetris_game_update last_update_since_dead,std::optional<std::vector<tetris_piece>> bag = std::nullopt) :
		m_custom_bag(std::move(bag)),
		m_data(g),
		m_last_update_since_dead(std::move(last_update_since_dead)) {	}

	std::optional<screen_thingy> update(event_handler_t& event_handler, game_keybinds& settings);

private:
	std::optional<std::vector<tetris_piece>> m_custom_bag = std::nullopt;
	all_game_data& m_data;
	tetris_game_update m_last_update_since_dead;

	std::chrono::milliseconds m_time_till_not_dead = 3000ms;

};

struct playing_solo_state{
	explicit playing_solo_state(all_game_data& g) :m_data(g),
		player_ptr(std::make_unique<tetris_game_keyboard_player>(g.game_settings)) {

		sf::Text back_button_text;
		back_button_text.setFont(g.default_font);
		back_button_text.setString("back");
		m_back_button.set_text(std::move(back_button_text));
		player_ptr->start_doing_stuff_now();
	}

	explicit playing_solo_state(all_game_data& g,std::vector<tetris_piece> bag) :
		m_custom_bag(bag),
		m_data(g),
		player_ptr(std::make_unique<tetris_game_keyboard_player>(g.game_settings,std::move(bag))) {

		sf::Text back_button_text;
		back_button_text.setFont(g.default_font);
		back_button_text.setString("back");
		m_back_button.set_text(std::move(back_button_text));
		player_ptr->start_doing_stuff_now();
	}


	std::optional<screen_thingy> update(event_handler_t& event_handler, game_keybinds& settings) {
		if (m_back_button.update(event_handler, event_handler.time_since_last_poll())) {
			return start_screen(m_data);
		}

		auto& window = event_handler.window();
		auto& player = *player_ptr;

		const auto [actions_this_frame, new_actions_this_frame] = next_actions_pressed(actions, event_handler, settings);

		actions = actions_this_frame;

		auto game_update = player.get_update();

		if (game_update.died) {
			return dead_solo_state(m_data,std::move(game_update),std::move(m_custom_bag));
		}

		player.recieve_update({ {actions}, {new_actions_this_frame}, event_handler.time_since_last_poll() }, 0);

		window.clear(sf::Color(100, 100, 100));
		draw_tetris_board(window, game_update, 200, 600);
		m_back_button.draw(window);

		return std::nullopt;
	}


private:
	std::optional<std::vector<tetris_piece>> m_custom_bag = std::nullopt;
	all_game_data& m_data;
	std::array<bool, (int)action::size> actions = {};

	std::unique_ptr<tetris_game_keyboard_player> player_ptr;
	boring_button m_back_button = boring_button(0, 10, 200, 100);

};

inline std::optional<screen_thingy> dead_solo_state::update(event_handler_t& event_handler, game_keybinds& settings) {
	const auto time_since_last_update = event_handler.time_since_last_poll();
	auto& window = event_handler.window();
	m_time_till_not_dead -= time_since_last_update;
	if (m_time_till_not_dead <= 0ms) {
		if(m_custom_bag.has_value()) {
			return playing_solo_state(m_data, std::move(m_custom_bag.value()));
		} else {
			return playing_solo_state(m_data);
		}
	}

	window.clear(sf::Color(100, 100, 100));
	draw_tetris_board(window, m_last_update_since_dead, 200, 600);

	return std::nullopt;
}
