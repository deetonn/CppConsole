#pragma once

#include <string>
#include <bitset>
#include <format>

#include <D3D11.h>
#include <Windows.h>

#include "RenderBackend.hpp"

#include "Forward.hpp" // For forward declaration of Console.

#define INPUT_BUFFER_SIZE 512

void cc_tooltip(const char* text);
bool cc_button(const char* text, const char* tooltip = NULL);
void cc_checkbox(const char* text, bool* checked, const char* tooltip = NULL);
void cc_int_input(
	const char* text, 
	int* value,
	const bool disabled,
	const char* tooltip = NULL
);
void cc_int2_input(
	const char* text, 
	int values[2], 
	const bool disabled, 
	const char* tooltip = NULL
);

void core_render_function(ImGuiIO& io, Renderer& renderer, OpenClicker& context);

enum class ClickType {
	// Just one single key press
	SingleClick,
	// The same as above, but twice.
	DoubleClick,

	SizeOfClickTypeDontTouch,
};

constexpr std::size_t click_type_count =
	static_cast<std::size_t>(ClickType::SizeOfClickTypeDontTouch);

enum class MouseButton {
	// The left mouse button.
	LeftClick,
	// The right mouse button.
	RightClick,
	// The top side button.
	TopSideButton,
	// The bottom side button.
	BottomSideButton,

	SizeOfMouseButtonDontTouch,
};

constexpr std::size_t mouse_button_count =
	static_cast<std::size_t>(MouseButton::SizeOfMouseButtonDontTouch);

enum class InputWidget {
	MillisecondBetweenClick,
	LaunchDelay,
	Coordinates,
	ClickType,
	MouseButtonSelection,
	TimeScaleSelection,

	CountOfWidgetsDontMove
};

constexpr std::size_t widget_count =
	static_cast<std::size_t>(InputWidget::CountOfWidgetsDontMove);

enum class Button : uint8_t {
	Start, 
	Stop,
	CoordinatesEnabled,
	CountOfButtonsDontMove
};

constexpr std::size_t button_count = 
	static_cast<std::size_t>(Button::CountOfButtonsDontMove);

enum class State {
	Clickable,
	Unclickable
};

enum class TimeScale {
	Milliseconds,
	Seconds,
	Minutes,
	Hours,

	CountOfTimeScaleDontMove,
};

constexpr std::size_t time_scale_count =
	static_cast<std::size_t>(TimeScale::CountOfTimeScaleDontMove);

struct TimeScaleInfo {
	const char* title_text;
	const char* tooltip_text;
	TimeScale scale;
};

struct Stats {
	std::size_t total_clicks{ 0 };
	std::size_t total_left_clicks{ 0 };
	std::size_t total_right_clicks{ 0 };
	std::size_t total_top_mb_clicks{ 0 };
	std::size_t total_bottom_mb_clicks{ 0 };
};

/*
* This struct contains all definitions that the core rendering depends on.
*
* This contains things such as if windows are opened, or if options are checked.
* It must exist before the rendering loop begins, so we don't reset state.
*
* This will be passed into core_render_function via the renderer.
*
* NOTE: add default values where possible.
*/

struct RenderingContext {
	std::bitset<button_count> button_states;
	std::bitset<widget_count> widget_states;

	HANDLE clicking_thread{ INVALID_HANDLE_VALUE };
	// Set to true to stop clicking_thread.
	bool stop_click_threadf{ false };
	bool prev_waiting_for_thread_exit{ false };
	bool waiting_for_thread_exit{ false };

	bool first_render_call{ true };
	std::int32_t time_between_click{ 1 };
	std::int32_t launch_delay{ 1 };

	// Corresponding to the below array. (Defaults to SingleClick)
	int selected_click_type = 0;
	// MUST be in this order.
	ClickType all_click_types[2] = { ClickType::SingleClick, ClickType::DoubleClick };

	// Correspoding to the below array. (Default to LeftMouse)
	int selected_mouse_button = 0;
	// MUST be in this order.
	MouseButton all_mouse_buttons[4] = { MouseButton::LeftClick, MouseButton::RightClick, MouseButton::TopSideButton, MouseButton::BottomSideButton };

	bool coords_enabled{ false };
	int coords[2] = { 0, 0 };
	bool capturing_mouse_coords{ false };

	std::string log_text{"This is the log text, useful information will live here.\n"};

	Stats stats{};

	bool use_diff_time_scale{ false };
	TimeScaleInfo time_scales[4] {
		TimeScaleInfo {
			.title_text = "Number of milliseconds between clicks",
			.tooltip_text = "The amount of milliseconds that will pass between clicks.",
			.scale = TimeScale::Milliseconds
		},
		TimeScaleInfo {
			.title_text = "Number of seconds between clicks",
			.tooltip_text = "The amount of seconds that will pass between clicks.",
			.scale = TimeScale::Seconds
		},
		TimeScaleInfo {
			.title_text = "Number of minutes between clicks",
			.tooltip_text = "The amount of minutes that will pass between clicks.",
			.scale = TimeScale::Minutes
		},
		TimeScaleInfo {
			.title_text = "Number of hours between clicks",
			.tooltip_text = "The amount of hours between clicks.",
			.scale = TimeScale::Hours
		}
	};
	int selected_time_scale{ 0 };

	template <class... Ts>
	inline void logln(const std::format_string<Ts...> fmt, Ts&&... args) noexcept {
		auto formatted_text = std::format(fmt, std::forward<Ts>(args)...);
		log_text += formatted_text;
		log_text += "\n";
	}

	inline State get_button_state(Button button) const noexcept {
		if (button == Button::CountOfButtonsDontMove)
			return State::Unclickable;
		return static_cast<State>(
			(bool)button_states[static_cast<std::size_t>(button)]
		);
	}
	inline void set_button_state(Button button, State state) noexcept {
		if (button == Button::CountOfButtonsDontMove)
			return;
		button_states.set(static_cast<size_t>(button), static_cast<bool>(state));
	}

	inline State get_widget_state(InputWidget widget) noexcept {
		if (widget == InputWidget::CountOfWidgetsDontMove)
			return State::Unclickable;
		return static_cast<State>(
			(bool)widget_states[static_cast<std::size_t>(widget)]
		);
	}

	inline void set_widget_state(InputWidget widget, State state) noexcept {
		if (widget == InputWidget::CountOfWidgetsDontMove)
			return;
		widget_states.set(static_cast<size_t>(widget), static_cast<bool>(state));
	}
};

class Renderer {
private:
	RenderBackend m_backend;
	RenderingContext m_render_context;
public:
	Renderer(const std::string_view title,
		const std::uint32_t x,
		const std::uint32_t y
	)
		: m_backend(title, x, y)
	{}

	void begin_render_loop(OpenClicker& context, Renderer& renderer) noexcept;
	RenderingContext& render_context() noexcept { return m_render_context; }

	void resize_window(std::size_t x, std::size_t y, UINT flags = 0);

	HWND window() noexcept { return m_backend.window_handle(); }
};