#pragma once
constexpr auto INIT_MONEY = 1000000;
constexpr auto MONEY_BAR = 100000;
constexpr auto USER_SIZE_BAR = 1;

enum Suit {
	spade, heart, diamond, club
};
enum Rank {
	J = 11, Q, K, A
};
struct Card {
	Suit s;
	int r;
};

struct User {
	uv_tcp_t* tcp;
	string id;

	int chip = -1;

	Card hole[2];

	bool operator==(User const& other) const
	{
		return tcp == other.tcp && id == other.id;
	}
	bool operator!=(User const& other) const
	{
		return !(*this == other);
	}
};



struct Room {
	static const int user_lim = 10;

	vector<User> users;

	int gameon = 0;

	int pool = 0;

	vector<Card> deck;
	Card flop[3];
	Card turn;
	Card river;

	void reset()
	{
		gameon = 1;
		pool = 0;
		deck.clear();
		for (int r = 2; r <= A; ++r)
			deck.push_back(Card{ spade,r });
		for (int r = 2; r <= A; ++r)
			deck.push_back(Card{ heart,r });
		for (int r = 2; r <= A; ++r)
			deck.push_back(Card{ diamond,r });
		for (int r = 2; r <= A; ++r)
			deck.push_back(Card{ club,r });
		std::random_shuffle(deck.begin(), deck.end());
		
		for (int i = 0; i < 3; ++i) {
			flop[0] = deck.back();
			deck.pop_back();
		}

		for (auto& user : users) {
			user.hole[0] = deck.back();
			deck.pop_back();
			user.hole[1] = deck.back();
			deck.pop_back();
		}

		turn = deck.back();
		deck.pop_back();

		river = deck.back();
		deck.pop_back();

		cerr << "all card given, " << " remaining deck size is " << deck.size() << '\n';
	}
};


