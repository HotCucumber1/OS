#pragma once

#include "../Client/Client.h"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Window/Event.hpp"

class App
{
public:
	App(
		int width,
		int height,
		const std::string& title,
		Client& client
		);

	void Run();

private:
	void ChangeColor(const sf::Keyboard::Key& key);

private:
	sf::RenderWindow m_window;
	sf::Vector2i m_lastMousePos;
	sf::Color m_color = sf::Color::Black;
	Client& m_client;
	bool m_isDrawing = false;
};
