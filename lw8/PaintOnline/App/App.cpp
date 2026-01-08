#include "App.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Event.hpp>


constexpr int FRAME_LIMIT = 60;
constexpr float THICKNESS = 3;

sf::Color GetSfColor(uint32_t color);

App::App(
	const int width,
	const int height,
	const std::string& title,
	Client& client)
	: m_window(
		  sf::VideoMode(width, height),
		  title)
	, m_client(client)
{
	m_window.setFramerateLimit(FRAME_LIMIT);
}

void App::Run()
{
	while (m_window.isOpen())
	{
		sf::Event event{};
		while (m_window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				m_window.close();
				break;
			case sf::Event::MouseButtonPressed:
				m_isDrawing = true;
				m_lastMousePos = sf::Mouse::getPosition(m_window);
				break;
			case sf::Event::MouseButtonReleased:
				m_isDrawing = false;
				break;
			case sf::Event::MouseMoved:
				if (m_isDrawing)
				{
					const sf::Vector2i endPos = sf::Mouse::getPosition(m_window);
					m_client.SendSegment(
						m_lastMousePos.x,
						m_lastMousePos.y,
						endPos.x,
						endPos.y,
						m_color.toInteger());
					m_lastMousePos = endPos;
				}
				break;
			case sf::Event::KeyPressed:
				ChangeColor(event.key.code);
				break;
			default:
				break;
			}
		}
		m_window.clear(sf::Color::White);
		const auto segments = m_client.GetData();
		for (const auto& s : segments)
		{
			const auto color = GetSfColor(s.color);
			const sf::Vertex line[] = {
				sf::Vertex(sf::Vector2f(s.x1, s.y1), color),
				sf::Vertex(sf::Vector2f(s.x2, s.y2), color)
			};

			m_window.draw(line, 2, sf::Lines);
		}
		m_window.display();
	}
}

sf::Color GetSfColor(const uint32_t color)
{
	sf::Color lineColor;
	lineColor.r = static_cast<sf::Uint8>((color >> 24) & 0xFF);
	lineColor.g = static_cast<sf::Uint8>((color >> 16) & 0xFF);
	lineColor.b = static_cast<sf::Uint8>((color >> 8) & 0xFF);
	lineColor.a = 255;
	return lineColor;
}

void App::ChangeColor(const sf::Keyboard::Key& key)
{
	switch (key)
	{
	case sf::Keyboard::R:
		m_color = sf::Color::Red;
		break;
	case sf::Keyboard::G:
		m_color = sf::Color::Green;
		break;
	case sf::Keyboard::B:
		m_color = sf::Color::Blue;
		break;
	default:
		break;
	}
}
