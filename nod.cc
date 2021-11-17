#include <iostream>
#include <regex>

using car_plate_t = std::string;

using line_number_t = uint_fast32_t;
using line_content_t = std::string;
using road_text_t = std::string;
using intersection_point_t = int_fast32_t;

using road_type_t = char;
using road_number_t = uint_fast16_t;

using car_dist_t = int_fast32_t;
using road_dist_t = int_fast64_t;

using record_type_t = std::tuple<line_number_t, line_content_t, road_text_t, intersection_point_t>;
using road_name_t = std::pair<road_type_t, road_number_t>;
using car_data_t = std::pair<car_dist_t, car_dist_t>;
using input_type_t = std::pair<line_number_t, line_content_t>;

namespace {
    /*
     * Struct with function used to compare two roads.
     */
    struct road_name_comparator {
        bool operator() (const road_name_t &road1, const road_name_t &road2) const {
            const auto &[road_type1, road_number1] = road1;
            const auto &[road_type2, road_number2] = road2;
            if (road_number1 == road_number2)
                return road_type1 < road_type2;
            return road_number1 < road_number2;
        }
    };

    std::map<car_plate_t, record_type_t> cars_on_road;
    std::map<road_name_t, road_dist_t, road_name_comparator> roads_data;
    std::map<car_plate_t, car_data_t> cars_data;

    constexpr char motorway = 'A';
    constexpr int_fast32_t default_dist = -1;
}

namespace get {
    inline car_plate_t get_car_plate(const std::smatch &words) {
        return words.str(1);
    }

    inline road_text_t get_road_text(const std::smatch &words) {
        return words.str(2);
    }

    inline std::string get_distance(const std::smatch &words) {
        return words.str(3);
    }

    inline std::string get_query_key(const std::smatch &words) {
        return words.str(1);
    }

    inline line_number_t get_line_number(const record_type_t &record) {
        return std::get<0>(record);
    }

    inline line_content_t get_line_content(const record_type_t &record) {
        return std::get<1>(record);
    }

    inline road_text_t get_road(const record_type_t &record) {
        return std::get<2>(record);
    }

    inline intersection_point_t get_intersection_point(const record_type_t &record) {
        return std::get<3>(record);
    }
}

namespace convert {
    /*
     * Splits a string containing information about road into two elements: road type and road
     * number.
     */
    inline road_name_t convert_string_to_road(const road_text_t &text) {
        road_number_t number = std::stoi(text.substr(1), nullptr, 10);
        return std::make_pair(text.at(0), number);
    }

    /*
     * Converts ABC,D format strings to ABCD format integer number. Notice: integer number is
     * 10 times larger than in original string representation to ensure correctness of calculations.
     */
    inline car_dist_t convert_string_to_number(const std::string &number) {
        car_dist_t value = 10 * std::stoi(number.substr(0, number.length() - 2), nullptr, 10);
        value += std::stoi(number.substr(number.length() - 1), nullptr, 10);
        return value;
    }
}

namespace update {
    /*
     * Adds a car to the map of cars currently on roads.
     */
    void add_car(const std::smatch &words, const std::pair<line_number_t, line_content_t> &input) {
        const auto &[line_number, line_content] = input;
        car_dist_t number = convert::convert_string_to_number(get::get_distance(words));
        record_type_t new_data = std::make_tuple(line_number, line_content,
                                                 get::get_road_text(words), number);
        cars_on_road.emplace(get::get_car_plate(words), new_data);
    }

    /*
     * Counts total number of km passed by car and adds it with car plate to the map.
     */
    void insert_car_data(car_data_t &car_data, const std::smatch &words) {
        auto &[dist_A, dist_S] = car_data;
        car_dist_t dist1 = get::get_intersection_point(cars_on_road.at(get::get_car_plate(words)));
        car_dist_t dist2 = convert::convert_string_to_number(get::get_distance(words));

        if (get::get_road_text(words).at(0) == motorway) {
            if (dist_A == default_dist) ++dist_A;
            dist_A += std::abs(dist2 - dist1);
        } else {
            if (dist_S == default_dist) ++dist_S;
            dist_S += std::abs(dist2 - dist1);
        }
        cars_data.emplace(get::get_car_plate(words), car_data);
    }

    /*
     * Counts total number of km passed by all cars on road and adds it with road name to the map.
     */
    void insert_road_data(road_dist_t &road_dist, const std::smatch &words) {
        road_dist_t dist1 = get::get_intersection_point(cars_on_road.at(get::get_car_plate(words)));
        road_dist_t dist2 = convert::convert_string_to_number(get::get_distance(words));
        if (road_dist == default_dist) {
            ++road_dist;
        }
        road_dist += std::abs(dist2 - dist1);
        roads_data.emplace(convert::convert_string_to_road(get::get_road_text(words)), road_dist);
    }

    /*
     * Updates information about car, acquiring old data if it was already in map. In case of a new
     * car, it just inserts it.
     */
    void acquire_car_data(const std::smatch &words) {
        car_data_t car_data = std::make_pair(default_dist, default_dist);
        if (cars_data.find(get::get_car_plate(words)) == cars_data.end()) {
            insert_car_data(car_data, words);
        } else {
            car_data = cars_data.at(get::get_car_plate(words));
            cars_data.erase(cars_data.find(get::get_car_plate(words)));
            insert_car_data(car_data, words);
        }
    }

    /*
     * Updates information about road, acquiring old data if it was already in map. In case of a new
     * road, it just inserts it.
     */
    void acquire_road_data(const std::smatch &words) {
        road_dist_t road_dist = default_dist;
        auto road = convert::convert_string_to_road(get::get_road_text(words));
        if (roads_data.find(road) == roads_data.end()) {
            insert_road_data(road_dist, words);
        } else {
            road_dist = roads_data.at(road);
            roads_data.erase(roads_data.find(road));
            insert_road_data(road_dist, words);
        }
    }

    /*
     * Updates information in maps containing data when car leaves the road.
     */
    void end_of_journey(const std::smatch &words) {
        acquire_car_data(words);
        acquire_road_data(words);
        cars_on_road.erase(cars_on_road.find(get::get_car_plate(words)));
    }

    /*
     * Updates information about cars and roads stored in maps. In case of new objects, it adds
     * them to the right maps.
     */
    void update_database(const std::smatch &words,
                         const std::pair<line_number_t, line_content_t> &input) {
        if (cars_on_road.find(get::get_car_plate(words)) == cars_on_road.end()) {
            add_car(words, input);
        } else if (get::get_road(cars_on_road.at(get::get_car_plate(words)))
                   != (get::get_road_text(words))) {

            std::cerr << "Error in line " << get::get_line_number(cars_on_road.at
                (get::get_car_plate(words))) << ": " << get::get_line_content(cars_on_road.at
                (get::get_car_plate(words))) << "\n";

            cars_on_road.erase(cars_on_road.find(get::get_car_plate(words)));
            add_car(words, input);
        } else {
            end_of_journey(words);
        }
    }
}

namespace query {
    /*
     * Prints information about car.
     * Notice: earlier for convenience the number of km was multiplied by 10 (see
     * convert_string_to_number function). The number of km is converted back to the proper format.
     */
    inline void print_car_data(const car_plate_t &plate, const car_data_t &car_data) {
        const auto &[dist_A, dist_S] = car_data;
        std::cout << plate;

        if (dist_A > default_dist) {
            std::cout << " A " << dist_A / 10 << "," << dist_A % 10;
        }
        if (dist_S > default_dist) {
            std::cout << " S " << dist_S / 10 << "," << dist_S % 10;
        }
        std::cout << "\n";
    }

    /*
     * Prints information about road.
     * Notice: earlier for convenience the number of km was multiplied by 10 (see
     * convert_string_to_number function). The number of km is converted back to the proper format.
     */
    inline void print_road_data(const road_name_t &road, const road_dist_t dist) {
        const auto &[road_type, road_number] = road;
        std::cout << road_type << road_number << " " << dist / 10 << "," << dist % 10 << "\n";
    }

    /*
     * Handles a query asking for all cars and roads.
     */
    void all_data_query() {
        for (const auto &car : cars_data) {
            const auto &[car_plate, car_dists] = car;
            print_car_data(car_plate, car_dists);
        }
        for (const auto &road : roads_data) {
            const auto &[road_name, road_dist] = road;
            print_road_data(road_name, road_dist);
        }
    }

    /*
     * Handles a query asking for a specific car.
     */
    void car_query(const std::smatch &words) {
        if (cars_data.find(get::get_query_key(words)) != cars_data.end()) {
            const auto p = cars_data.at(get::get_query_key(words));
            print_car_data(get::get_query_key(words), p);
        }
    }

    /*
     * Handles a query asking for a specific road.
     */
    void road_query(const std::smatch &words) {
        if (roads_data.find(convert::convert_string_to_road(get::get_query_key(words)))
            != roads_data.end()) {
            const auto p = roads_data.at(convert::convert_string_to_road(get::get_query_key(words)));
            print_road_data(convert::convert_string_to_road(get::get_query_key(words)), p);
        }
    }
}

namespace nod {
    /*
     * Reads input line by line and calls for an update, a query, or writes an error, depending on
     * what has been read.
     */
    void invoke(const std::regex regexes_table[4]) {

        input_type_t input;
        auto &[line_number, line_content] = input;
        line_number = 0;
        std::smatch words;

        while (std::getline(std::cin, line_content)) {
            ++line_number;
            if (line_content.empty()) continue;

            if (std::regex_match(line_content, words, regexes_table[0])) {
                update::update_database(words, input);
            } else if (std::regex_match(line_content, regexes_table[1])) {
                query::all_data_query();
            } else {
                bool right_query = false;
                if (std::regex_match(line_content, words, regexes_table[2])) {
                    query::car_query(words);
                    right_query = true;
                }
                if (std::regex_match(line_content, words, regexes_table[3])) {
                    query::road_query(words);
                    right_query = true;
                }
                if (!right_query) {
                    std::cerr << "Error in line " << line_number << ": " << line_content << "\n";
                }
            }
        }
    }
}

/*
 * Creates regexes from strings and invokes a function to process incoming input from user.
 */
int main() {
    std::regex data_regex(
        R"(^\s*([A-Za-z0-9]{3,11})\s+([AS][1-9]\d{0,2})\s+(0,\d|[1-9]\d{0,7},\d)\s*$)");
    std::regex all_query_regex(R"(^\s*[?]\s*$)");
    std::regex car_query_regex(R"(^\s*[?]\s*([a-zA-Z0-9]{3,11})\s*$)");
    std::regex road_query_regex(R"(^\s*[?]\s*([AS][1-9]\d{0,2})\s*$)");

    std::regex regexes_table[4] = {data_regex, all_query_regex, car_query_regex, road_query_regex};
    nod::invoke(regexes_table);

    return 0;
}
