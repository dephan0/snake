#include "../include/app.h"
#include <algorithm>

App::App(std::vector<std::string>& args) {
    running_ = true;
    if (!init(args)) {
        running_ = false;
    }
}

App::~App() = default;

int App::run() {
    while (running_) {
        // ..and visualize it in the GUI
        if (running_ && gui_) {
            bool ok = gui_->update();
            std::cout << "update" << std::endl;
            if (!ok) {
                running_ = false;
            }
        }
    }

    return 0;
}

void App::print_help() {
    std::cout
        << "-----------------------------------------------------------\n"
        << "snake -- simple snake game written in SFML \n"
        << "-----------------------------------------------------------\n"
        << "Source: https://github.com/bartekpacia/snake\n"
        << "\n"
        << "\tGUI options:\n"
        << "\t-H  --height [val]             window height (defualt: 1280)\n"
        << "\t-W  --width [val]              window width (defualt: 720)\n";
}

bool App::is_flag_present(std::vector<std::string>& all_args,
                          const std::string& short_arg,
                          const std::string& long_arg) {
    auto it = std::find_if(all_args.begin(), all_args.end(),
                           [short_arg, long_arg](const std::string& s) {
                               return s == short_arg || s == long_arg;
                           });

    if (it == all_args.end()) {
        return false;
    }

    all_args.erase(it);
    return true;
}

std::string App::get_flag_value(std::vector<std::string>& all_args,
                                const std::string& short_arg,
                                const std::string& long_arg,
                                const std::string& default_value) {
    auto it = std::find_if(all_args.begin(), all_args.end(),
                           [short_arg, long_arg](const std::string& s) {
                               return s == short_arg || s == long_arg;
                           });

    std::string value = default_value;
    if (it == all_args.end()) {
    } else if (it + 1 == all_args.end()) {
        all_args.erase(it);
    } else {
        value = *(it + 1);
        all_args.erase(it, it + 2);
    }
    return value;
}

bool App::init(std::vector<std::string>& args) {
    // print help
    if (is_flag_present(args, "-h", "--help")) {
        print_help();
        return false;
    }

    // Initialize the GUI
    GUISettings sfml_settings;

    std::stringstream(get_flag_value(args, "-W", "--width")) >>
        sfml_settings.width;
    std::stringstream(get_flag_value(args, "-H", "--height")) >>
        sfml_settings.height;

    gui_ = std::make_unique<GUI>(sfml_settings);

    std::cout << "init completed" << std::endl;

    return true;
}
