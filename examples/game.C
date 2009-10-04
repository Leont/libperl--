#include "perl++.h"
#include <string>
#include <list>
#include <utility>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

using namespace perl;
using namespace std;

class player {
	string name;
	pair<int, int> position;
	double strength;
	public:
	player(string initial_name) : name(initial_name) {
	}
	string get_name() const {
		return name;
	}
	pair<int, int> get_position() const {
		return position;
	}
	void set_position(pair<int, int> new_position) {
		position = new_position;
	}

	double get_strength() const {
		return strength;
	}
	void set_strength(double new_strength) {
		strength = new_strength;
	}
	void die(string) {
		// XXX
	}
};

struct game_exception : public std::exception {
	string message;
	game_exception(const std::string& _message) : message(_message) {
	}
	const char* what() {
		return message.c_str();
	}
	~game_exception() throw() {
	}
};

using namespace boost::lambda;

class game {
	list<player> players;
	public:
	void add_player(const string& name) {
		players.push_back(player(name));
	}
	player& get_player(const string& name) {
		list<player>::iterator player_itr = find_if(players.begin(), players.end(), bind(&player::get_name, _1) == name);
		if (player_itr == players.end())
			throw game_exception("No such player");
		return *player_itr;
	}
	const list<player>& get_players() const {
		return players;
	}
};

namespace perl {
	namespace typecast {
		template<> struct typemap<player> : public exported_type<player> {
		};
	}
}

int main() {
	static Interpreter universe;

	Class<player> player_class = universe.add_class("Player");

	player_class.add("get_name",     &player::get_name);
	player_class.add("get_position", &player::get_position);
	player_class.add("set_position", &player::set_position);
	player_class.add("get_strength", &player::get_strength);
	player_class.add("set_strength", &player::set_strength);

	Class<game> gamer_class = universe.add_class("Game");

	gamer_class.add("add_player", &game::add_player);
	gamer_class.add("get_player", &game::get_player);

	return 0;
}
