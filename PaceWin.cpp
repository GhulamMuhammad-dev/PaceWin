#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cmath>
#include <memory>
#include <iostream>

// Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float GRAVITY = 1000.0f;
const float JUMP_VELOCITY = -500.0f;
const float MIN_SPAWN_INTERVAL = 1.0f;
const float MAX_SPAWN_INTERVAL = 5.0f;
const float MIN_MONSTER_SPEED = 100.0f;
const float MAX_MONSTER_SPEED = 500.0f;
const int SMALL_MONSTER_HEALTH = 5;
const int NUM_SMALL_MONSTERS_FOR_GIANT = 15;
const int PLAYER_HEALTH = 10;
const int PLAYER_BULLETS_TO_KILL = 20;

class Bullet {
public:
    Bullet(sf::Vector2f position, sf::Vector2f velocity) : shape(sf::Vector2f(5, 5)), velocity(velocity) {
        shape.setPosition(position);
        shape.setFillColor(sf::Color::Yellow);
    }

    void move(float deltaTime) {
        shape.move(velocity * deltaTime);
    }

    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
    }

private:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
};

class Gun {
public:
    Gun(sf::Vector2f position) {
        shape.setFillColor(sf::Color::Green);
        shape.setSize(sf::Vector2f(20, 10));
        shape.setOrigin(shape.getSize().x / 2, shape.getSize().y / 2);
        shape.setPosition(position);
    }

    void setRotation(float angle) {
        shape.setRotation(angle);
    }

    void setPosition(sf::Vector2f position) {
        shape.setPosition(position);
    }

    sf::Vector2f getPosition() const {
        return shape.getPosition();
    }

    sf::Vector2f getDirection() const {
        float angleRad = shape.getRotation() * (3.1415926f / 180.0f);
        return sf::Vector2f(std::cos(angleRad), std::sin(angleRad));
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
    }

private:
    sf::RectangleShape shape;
};

class Monster {
public:
    Monster(float x, float y, float speed, int health, sf::Color color) : shape(sf::Vector2f(30, 30)), velocity(-speed, 0), health(health), color(color) {
        shape.setPosition(x, y);
        shape.setFillColor(color);
    }

    void move(float deltaTime) {
        shape.move(velocity * deltaTime);
    }

    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }

    int getHealth() const {
        return health;
    }

    void decreaseHealth(int amount) {
        health -= amount;
        if (health <= 0)
            destroyed = true;
    }

    bool isDestroyed() const {
        return destroyed;
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
    }

    const sf::RectangleShape& getShape() const {
        return shape;
    }

protected:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    int health;
    bool destroyed = false;
    sf::Color color;
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML Game");

    sf::RectangleShape ground(sf::Vector2f(WINDOW_WIDTH, 50));
    ground.setFillColor(sf::Color::Green);
    ground.setPosition(0, WINDOW_HEIGHT - ground.getSize().y);

    sf::RectangleShape player1(sf::Vector2f(50, 50));
    player1.setFillColor(sf::Color::Blue);
    player1.setPosition(50, WINDOW_HEIGHT - ground.getSize().y - player1.getSize().y);

    sf::RectangleShape player2(sf::Vector2f(50, 50));
    player2.setFillColor(sf::Color::Red);
    player2.setPosition(WINDOW_WIDTH - 100, WINDOW_HEIGHT - ground.getSize().y - player2.getSize().y);

    // Health bars for players
    sf::RectangleShape player1HealthBar(sf::Vector2f(100, 10));
    player1HealthBar.setFillColor(sf::Color::Green);
    player1HealthBar.setPosition(10, 10);

    sf::RectangleShape player2HealthBar(sf::Vector2f(100, 10));
    player2HealthBar.setFillColor(sf::Color::Green);
    player2HealthBar.setPosition(WINDOW_WIDTH - 110, 10);

    // Gun attached to player 1
    Gun gun(player1.getPosition() + sf::Vector2f(25, 25));

    // Bullets fired by the gun
    std::vector<Bullet> bullets;

    // AI player
    std::vector<std::unique_ptr<Monster>> monsters;

    // Variables to control player movement
    bool player1Jumping = false;
    float player1Velocity = 0;

    bool player2Jumping = false;
    float player2Velocity = 0;

    // Random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Clock for delta time calculation
    sf::Clock clock;
    sf::Clock spawnTimer;

    // Number of small monsters spawned
    int numSmallMonstersSpawned = 0;

    // Player health
    int player1Health = PLAYER_HEALTH;
    int player2Health = PLAYER_HEALTH;

    // Bullets hit count for player 2
    int player2BulletsHit = 0;

    // Main game loop
    while (window.isOpen())
    {
        // Calculate delta time
        sf::Time deltaTime = clock.restart();

        // Event handling
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            // Jumping controls for player 1
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::W && !player1Jumping)
                {
                    player1Velocity = JUMP_VELOCITY;
                    player1Jumping = true;
                }
            }
            // Jumping controls for player 2
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Up && !player2Jumping)
                {
                    player2Velocity = JUMP_VELOCITY;
                    player2Jumping = true;
                }
            }
            // Handle mouse button press
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    sf::Vector2f mousePosition = sf::Vector2f(event.mouseButton.x, event.mouseButton.y);
                    sf::Vector2f direction = mousePosition - gun.getPosition();
                    float angle = std::atan2(direction.y, direction.x) * (180 / 3.14159265f);
                    gun.setRotation(angle);
                    sf::Vector2f bulletVelocity = gun.getDirection() * 300.0f; // Bullet speed
                    bullets.emplace_back(gun.getPosition(), bulletVelocity);
                }
            }
        }

        // Apply gravity to player 1
        if (player1.getPosition().y + player1.getSize().y < WINDOW_HEIGHT - ground.getSize().y)
        {
            player1Velocity += GRAVITY * deltaTime.asSeconds();
            player1.move(0, player1Velocity * deltaTime.asSeconds());
        }
        else
        {
            player1.setPosition(player1.getPosition().x, WINDOW_HEIGHT - ground.getSize().y - player1.getSize().y);
            player1Jumping = false;
            player1Velocity = 0;
        }

        // Apply gravity to player 2
        if (player2.getPosition().y + player2.getSize().y < WINDOW_HEIGHT - ground.getSize().y)
        {
            player2Velocity += GRAVITY * deltaTime.asSeconds();
            player2.move(0, player2Velocity * deltaTime.asSeconds());
        }
        else
        {
            player2.setPosition(player2.getPosition().x, WINDOW_HEIGHT - ground.getSize().y - player2.getSize().y);
            player2Jumping = false;
            player2Velocity = 0;
        }

        // AI player behavior - Spawn monsters gradually
        if (spawnTimer.getElapsedTime().asSeconds() >= MIN_SPAWN_INTERVAL + static_cast<float>(rand()) / RAND_MAX * (MAX_SPAWN_INTERVAL - MIN_SPAWN_INTERVAL)) {
            // Spawn a monster in front of player 2
            float monsterX = player2.getPosition().x - 50; // Spawn 50 pixels in front of player 2
            float monsterSpeed = MIN_MONSTER_SPEED + static_cast<float>(rand()) / RAND_MAX * (MAX_MONSTER_SPEED - MIN_MONSTER_SPEED); // Random speed between min and max

            float monsterY = WINDOW_HEIGHT - ground.getSize().y - 30; // Set Y position to be on the ground
            monsters.push_back(std::make_unique<Monster>(monsterX, monsterY, monsterSpeed, SMALL_MONSTER_HEALTH, sf::Color::Magenta));

            // Increase the count of small monsters spawned
            numSmallMonstersSpawned++;

            // Reset spawn timer
            spawnTimer.restart();
        }

        // Move monsters towards player 1
        for (auto& monster : monsters) {
            monster->move(deltaTime.asSeconds());

            // Collision detection with player 1
            if (monster->getBounds().intersects(player1.getGlobalBounds())) {

                // Handle collision with player 1
                player1Health--;
                if (player1Health <= 0) {
                    // Game over, player 1 lost
                    std::cout << "Game Over - Player 1 Lost!" << std::endl;
                    window.close();
                }
                // Destroy the monster
                monster->decreaseHealth(SMALL_MONSTER_HEALTH); // Decrease health by remaining health
            }

            // Collision detection with player 2
            if (monster->getBounds().intersects(player2.getGlobalBounds())) {
                // Handle collision with player 2
                player2Health--;
                if (player2Health <= 0) {
                    // Game over, player 2 lost
                    std::cout << "Game Over - Player 2 Lost!" << std::endl;
                    window.close();
                }
                // Destroy the monster
                monster->decreaseHealth(SMALL_MONSTER_HEALTH); // Decrease health by remaining health
            }

            // Collision detection with bullets
            for (auto& bullet : bullets) {
                if (monster->getBounds().intersects(bullet.getBounds())) {
                    monster->decreaseHealth(1); // Decrease health by 1 for each hit
                    bullet = bullets.back(); // Replace current bullet with the last bullet
                    bullets.pop_back(); // Remove last bullet
                    break; // Break to avoid invalid iterator access
                }
            }
        }

        // Remove destroyed monsters
        monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](const std::unique_ptr<Monster>& m) { return m->isDestroyed(); }), monsters.end());

        // Move bullets
        for (auto& bullet : bullets) {
            bullet.move(deltaTime.asSeconds());
        }

        // Collision detection with bullets for player 2
        for (auto& bullet : bullets) {
            if (bullet.getBounds().intersects(player2.getGlobalBounds())) {
                // Bullet hit player 2
                player2BulletsHit++;
                player2Health--;
                bullet = bullets.back(); // Replace current bullet with the last bullet
                bullets.pop_back(); // Remove last bullet

                // Check if player 2 is killed
                if (player2Health <= 0) {
                    // Game over, player 2 lost
                    std::cout << "Game Over - Player 2 Lost!" << std::endl;
                    window.close();
                }
                break; // Break to avoid invalid iterator access
            }
        }

        // Update health bar sizes
        player1HealthBar.setSize(sf::Vector2f(player1Health * 10, 10));
        player2HealthBar.setSize(sf::Vector2f(player2Health * 10, 10));

        // Rendering
        window.clear();
        window.draw(ground);
        window.draw(player1);
        window.draw(player2);
        window.draw(player1HealthBar);
        window.draw(player2HealthBar);
        gun.draw(window);
        for (const auto& bullet : bullets) {
            bullet.draw(window);
        }
        for (const auto& monster : monsters) {
            monster->draw(window);
        }
        window.display();
    }

    return 0;
}
