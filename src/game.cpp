#include "../include/game.h"

Game::Game(const GameSettings& settings) {
    settings_ = settings;

    sf::ContextSettings window_settings;
    window_settings.antialiasingLevel = settings_.antialiasing;

    if (!settings_.font.loadFromFile("arial.ttf")) {
        std::clog << "snake: error loading font file" << std::endl;
    }

    window_.create(sf::VideoMode(settings_.width, settings_.height), "snake",
                   sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize,
                   window_settings);

    move_status_ = MoveStatus::LEFT;

    timestamp_ = std::chrono::system_clock::now().time_since_epoch() /
                 std::chrono::seconds(1);

    // fill the board with empty tiles
    for (auto i = 0; i < settings.grid_size; i++) {
        std::vector<TileObject> row;
        for (auto j = 0; j < settings.grid_size; j++) {
            row.push_back(TileObject::Empty);
        }
        state_.push_back(row);
    }

    // place snake head on the board
    auto snake_head_position =
        sf::Vector2i(state_.size() / 2, state_.size() / 2);

    // Always update these two state variables when updating snake position.
    snake_positions_.push_back(snake_head_position);
    state_[snake_positions_.front().x][snake_positions_.front().y] =
        TileObject::Snake;

    // place the first point on the board
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> random(0, settings.grid_size - 1);
    point_pos_ = sf::Vector2i(random(generator), random(generator));
    state_[point_pos_.x][point_pos_.y] = TileObject::Pickup;

    // load high score
    std::ifstream input_score_file("high_score.txt");
    if (input_score_file.fail()) {
        high_score_ = 0;
    } else {
        std::stringstream buffer;
        buffer << input_score_file.rdbuf();
        std::string high_score = buffer.str();
        try {
            high_score_ = std::stoi(high_score);
        } catch (const std::invalid_argument& e) {
            high_score_ = 0;
        }
    }
}

void Game::move_snake() {
    std::vector<sf::Vector2i> new_snake_positions =
        std::vector(snake_positions_);

    switch (move_status_) {
        case MoveStatus::LEFT: {
            new_snake_positions[0] =
                sf::Vector2i(snake_positions_[0].x - 1, snake_positions_[0].y);

            for (int i = 1; i < snake_positions_.size(); i++) {
                new_snake_positions[i] = snake_positions_[i - 1];
            }
            break;
        }
        case MoveStatus::UP: {
            new_snake_positions[0] =
                sf::Vector2i(snake_positions_[0].x, snake_positions_[0].y - 1);
            for (int i = 1; i < snake_positions_.size(); i++) {
                new_snake_positions[i] = snake_positions_[i - 1];
            }
            break;
        }
        case MoveStatus::RIGHT: {
            new_snake_positions[0] =
                sf::Vector2i(snake_positions_[0].x + 1, snake_positions_[0].y);
            for (int i = 1; i < snake_positions_.size(); i++) {
                new_snake_positions[i] = snake_positions_[i - 1];
            }
            break;
        }
        case MoveStatus::DOWN: {
            new_snake_positions[0] =
                sf::Vector2i(snake_positions_[0].x, snake_positions_[0].y + 1);
            for (int i = 1; i < snake_positions_.size(); i++) {
                new_snake_positions[i] = snake_positions_[i - 1];
            }
            break;
        }
    }

    // LOST: out of bounds
    if (new_snake_positions[0].x < 0 || new_snake_positions[0].y < 0 ||
        new_snake_positions[0].x > settings_.grid_size - 1 ||
        new_snake_positions[0].y > settings_.grid_size - 1) {
        std::cout << "LOST (OUT OF BOUNDS)" << std::endl;
        settings_.running = false;
        return;
    }

    // LOST: snake hit itself
    for (int i = 1; i < new_snake_positions.size(); i++) {
        auto segment = new_snake_positions[i];
        auto head = new_snake_positions.front();

        if (head.x == segment.x && head.y == segment.y) {
            std::cout << "LOST (SNAKE HIT ITSELF)" << std::endl;
            settings_.running = false;
            return;
        }
    }

    // collected crystal
    if (new_snake_positions[0].x == point_pos_.x &&
        new_snake_positions[0].y == point_pos_.y) {
        std::cout << "COLLECT" << std::endl;

        score_++;
        if (score_ > high_score_) {
            high_score_ = score_;
        }

        // add new segment in place of the last one in the old snake_positions
        sf::Vector2i new_segment_position = snake_positions_.back();

        new_snake_positions.push_back(new_segment_position);

        new_crystal();
    }

    // clear old snake
    for (auto& old_snake_pos : snake_positions_) {
        state_[old_snake_pos.x][old_snake_pos.y] = TileObject::Empty;
    }

    // update state with new snake
    snake_positions_ = new_snake_positions;
    for (auto& snake_pos : snake_positions_) {
        state_[snake_pos.x][snake_pos.y] = TileObject::Snake;
    }
}

void Game::new_crystal() {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> random(0, settings_.grid_size - 1);

    unsigned int x;
    unsigned int y;

    do {
        x = random(generator);
        y = random(generator);
    } while (std::find(snake_positions_.begin(), snake_positions_.end(),
                       sf::Vector2i(x, y)) != snake_positions_.end());

    state_[x][y] = TileObject::Pickup;
    point_pos_ = sf::Vector2i(x, y);
}

bool Game::update() {
    if (!settings_.running) {
        window_.close();
        return false;
    }

    window_.clear(settings_.color_background);

    handle_logic();
    handle_input();
    render_grid();
    render_ui();

    window_.display();

    return true;
}

void Game::handle_logic() {
    auto new_timestamp = std::chrono::system_clock::now().time_since_epoch() /
                         std::chrono::milliseconds(settings_.interval_ms);

    if (new_timestamp > timestamp_) {
        move_snake();
    }

    // post-update
    timestamp_ = new_timestamp;
}

void Game::handle_input() {
    sf::Event event;
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Resized) {
            settings_.width = event.size.width;
            settings_.height = event.size.height;
            sf::FloatRect visible_area(0, 0, event.size.width,
                                       event.size.height);
            window_.setView(sf::View(visible_area));
            std::cout << "snake: window resized: " << event.size.width << "x"
                      << event.size.height << std::endl;
        }
        if (event.type == sf::Event::Closed)
            settings_.running = false;
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Left)
                move_status_ = MoveStatus::LEFT;
            if (event.key.code == sf::Keyboard::Up)
                move_status_ = MoveStatus::UP;
            if (event.key.code == sf::Keyboard::Right)
                move_status_ = MoveStatus::RIGHT;
            if (event.key.code == sf::Keyboard::Down)
                move_status_ = MoveStatus::DOWN;
        }
    }
}

void Game::render_grid() {
    double width = settings_.width / settings_.grid_size;
    double height = settings_.height / settings_.grid_size;

    for (int i = 0; i < settings_.grid_size; i++) {
        for (int j = 0; j < settings_.grid_size; j++) {
            auto x = width * i;
            auto y = height * j;
            auto tileObject = state_[i][j];

            Color color = settings_.color_background;
            if (tileObject == TileObject::Snake) {
                color = settings_.color_snake;
            }
            if (tileObject == TileObject::Pickup) {
                color = settings_.color_pickup;
            }

            auto tile = sf::RectangleShape(sf::Vector2f(width, height));
            tile.setPosition(x, y);
            tile.setFillColor(color);
            window_.draw(tile);
        }
    }
}

void Game::render_ui() {
    // render score text
    sf::Text score_text;

    score_text.setFont(settings_.font);

    std::string score;
    score.append("Score: ");
    score.append(std::to_string(score_));
    score_text.setString(score);

    score_text.setCharacterSize(24);
    score_text.setFillColor(settings_.color_ui);
    score_text.setStyle(sf::Text::Bold);
    score_text.setPosition(32, 32);
    window_.draw(score_text);

    // render high score text
    sf::Text high_score_text;

    high_score_text.setFont(settings_.font);

    std::string high_score;
    high_score.append("High score: ");
    high_score.append(std::to_string(high_score_));
    high_score_text.setString(high_score);

    high_score_text.setCharacterSize(24);
    high_score_text.setFillColor(settings_.color_ui);
    high_score_text.setStyle(sf::Text::Bold);
    high_score_text.setPosition(256, 32);
    window_.draw(high_score_text);
}
